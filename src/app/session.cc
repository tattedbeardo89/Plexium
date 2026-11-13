// SPDX-License-Identifier: GPL-3.0-or-later
#include "session.h"
#include "runtime/state.h"
#include "browse.h"
#include "plex/navigation.h"
#include "plex/audio.h"
#include "plex/free_tv.h"
#include "plex/episodes.h"
#include <chrono>
#include <fstream>
#include <random>
#include <sys/stat.h>
#include <unistd.h>
namespace plexium {
std::vector<Destination> navigation(const std::vector<plex::Item>& sections) {
 std::vector<Destination> out={{"home","Home"},{"search","Search"},{"live-tv","Live TV"}};
 std::set<std::string> seen;
 for(const auto &s:sections) if(!s.id.empty() && seen.insert(s.id).second)out.push_back({"library:"+s.id,s.title});
 out.push_back({"free-tv","Free Live TV (Beta)"});out.push_back({"profiles","Switch profile"});out.push_back({"servers","Change server"});out.push_back({"settings","Settings"});return out;
}
std::string poster(const plex::Item &item) {
 if(item.type=="episode")return item.grandparent_thumb;
 return item.thumb;
}
double Session::now()const{if(clock)return clock();return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
Session::Session(plex::Transport t,const std::string &path,PlaybackPort &p,std::function<double()> monotonic_clock):client(std::move(t),"pending"),clock(std::move(monotonic_clock)),root(path),player(p){}
void Session::fail(const char *stage,int status){model.message=std::string(stage)+(status?" (HTTP "+std::to_string(status)+")":"")+". Retry or return with Circle.";}
bool Session::private_root(){
 struct stat s{};
 if(mkdir(root.c_str(),0700)!=0 && errno!=EEXIST)return false;
 if(lstat(root.c_str(),&s)!=0 || !S_ISDIR(s.st_mode) || s.st_uid!=geteuid() || (s.st_mode&077))return false;
#ifdef PLEX_PS5
 if(root==PLEX_STATE_ROOT && !migrate_state(PLEX_LEGACY_STATE_ROOT,root))return false;
#endif
 return true;
}
void Session::prepare_next(){
 model.upcoming.clear();model.autoplay_remaining=0;next_deadline=0;
 if(model.detail.type!="episode")return;
 auto bounded=client.bounded(800,1500);auto result=plex::next_episode(bounded,base,token,model.detail);
 if(!result){model.message="Next episode is unavailable; choose an episode from the season.";return;}
 auto values=plex::items(result.data);if(values.size()!=1||values[0].id==model.detail.id)return;
 model.upcoming.push_back(std::move(values[0]));
 if(model.autoplay){next_deadline=now()+10;model.autoplay_remaining=10;}
}
bool Session::play_next(){
 next_deadline=0;model.autoplay_remaining=0;
 if(model.upcoming.size()!=1)return false;
 auto id=model.upcoming.front().id;model.upcoming.clear();
 // Resolve fresh Plex metadata/resume state after the prior session released.
 auto response=client.bounded(800,2000).browse(base,token,"/library/metadata/"+plex::encode(id)+"?includeMarkers=1",0,1);
 if(!response){fail("Next episode metadata is unavailable",response.status);return false;}
 auto values=plex::items(response.data);if(values.size()!=1||values[0].id!=id||values[0].type!="episode"||values[0].watched){fail("Next episode is no longer available or already watched");return false;}
 model.detail=std::move(values[0]);model.media_version=0;load_tracks();return open_playback();
}
void Session::save_preferences(){
 if(track_preferences_scope.empty())return;
 plex::Json data;if(!read_private_json(root+"/playback-preferences.json",data)){struct stat existing{};if(lstat((root+"/playback-preferences.json").c_str(),&existing)==0){model.message="Playback preferences could not be read safely.";return;}data=plex::Json{{"schema",1},{"profiles",plex::Json::object()}};}
 auto profiles=data.find("profiles");if(plex::number(data,"schema")!=1||profiles==data.end()||!profiles->is_object()||profiles->size()>64){model.message="Playback preferences have an unsupported schema.";return;}
 if(!profiles->contains(track_preferences_scope)&&profiles->size()>=64){model.message="Playback preference scope limit reached.";return;}
 (*profiles)[track_preferences_scope]={{"audio_language",audio_language},{"subtitle_language",subtitle_language},{"subtitles",prefer_subtitles},{"autoplay",model.autoplay},{"quality",int(model.quality)}};
 if(!write_private_json(root+"/playback-preferences.json",data))model.message="Playback preferences could not be saved.";
}
void Session::load_tracks(){
 model.audio_tracks=plex::media_tracks(model.detail,model.media_version,false);model.subtitle_tracks=plex::media_tracks(model.detail,model.media_version,true);model.tracks={};
 for(const auto &track:model.audio_tracks)if(!audio_language.empty()&&subtitle_language_matches(track.language,audio_language)){model.tracks.audio=track.id;break;}
 if(model.tracks.audio.empty())for(const auto &track:model.audio_tracks)if(track.selected){model.tracks.audio=track.id;break;}
 if(prefer_subtitles&&!subtitle_language.empty())for(const auto &track:model.subtitle_tracks)if(subtitle_language_matches(track.language,subtitle_language)){model.tracks.subtitle=track.id;break;}
}
void Session::apply_tracks(){
 if(!tracks_pending)return;
 bool ready=true;
 auto apply=[&](bool subtitle,const std::vector<plex::MediaTrack>&tracks,const std::string&id){
  if(id.empty()){if(subtitle&&!player.select_native_track(true,-1))ready=false;return;}
  const auto *track=plex::selected_track(tracks,id);if(!track){ready=false;return;}
  // A negotiated server output may have new ES identities or burnt subtitles.
  if(model.playback_mode!=plex::Mode::DirectPlay){if(subtitle)player.select_native_track(true,-1);return;}
  if(subtitle&&!track->key.empty()){
   plex::Json selected={{"streamType",3},{"key",track->key},{"codec",track->codec}};auto url=plex::external_subtitle(base,token,selected);
   if(!external_applied){if(url.empty()||!player.add_external_subtitle(url))ready=false;else external_applied=true;}return;
  }
  auto id_native=plex::native_track_match(*track,player.native_tracks(subtitle));if(id_native<0||!player.select_native_track(subtitle,id_native))ready=false;
 };
 apply(false,model.audio_tracks,model.tracks.audio);apply(true,model.subtitle_tracks,model.tracks.subtitle);
 if(ready)tracks_pending=false;
 else model.message="Waiting for an unambiguous native track match. Choose another track or server quality if unavailable.";
}
void Session::persist_progress(){
 if(!journal.save(outbox,std::time(nullptr),now()))model.message="Playback progress could not be saved safely; keep the application open to retry.";
}
void Session::flush_progress(){
 if(base.empty()||token.empty())return;
 size_t budget=2;
 for(size_t n=0;n<outbox.entries.size()&&budget;){
  const auto u=outbox.entries[n];if(u.blocked||u.due>now()){++n;continue;}
  auto bounded=client.bounded(500,1000);
  if(u.recovered||u.attempts>0){
   --budget;auto metadata=bounded.browse(base,token,"/library/metadata/"+plex::encode(u.item.id),0,1);
   if(!metadata){if(metadata.status==401||metadata.status==403)outbox.entries[n].blocked=true;else outbox.failed(u.item.id,u.sequence,now());persist_progress();++n;continue;}
   auto values=plex::items(metadata.data);auto verdict=values.size()==1?plex::reconcile_progress(u,values[0]):plex::Reconcile::Conflict;
   if(verdict!=plex::Reconcile::Send){if(verdict==plex::Reconcile::AlreadyApplied){last_ack_session=u.session;last_ack_id=u.item.id;last_ack_state=u.state;last_ack_position=u.position;}outbox.ack(u.item.id,u.sequence);persist_progress();continue;}
   if(!budget){++n;continue;}
  }
  --budget;auto result=bounded.timeline(base,token,u.item,u.state,u.position,u.session);
  if(result){last_ack_session=u.session;last_ack_id=u.item.id;last_ack_state=u.state;last_ack_position=u.position;if(u.session==playback_session&&u.item.id==model.detail.id)playback_baseline=u.position;outbox.ack(u.item.id,u.sequence);}
  else{outbox.failed(u.item.id,u.sequence,now());if(result.status==401||result.status==403)outbox.entries[n].blocked=true;++n;}
  persist_progress();
 }
}
void Session::report_progress(const std::string &state,int64_t position){
 if(model.detail.type=="channel"||base.empty()||token.empty())return;
 if(last_ack_session==playback_session&&last_ack_id==model.detail.id&&last_ack_state==state&&last_ack_position==position){flush_progress();return;}
 if(!outbox.put(model.detail,state,position,std::time(nullptr),playback_session)){model.message="Progress journal capacity reached; previous updates were preserved.";return;}
 for(auto &u:outbox.entries)if(u.item.id==model.detail.id){u.baseline=playback_baseline;u.baseline_viewed=playback_viewed;}
 persist_progress();flush_progress();
}
void Session::shutdown(){
 if((model.screen==Screen::Playback||(model.screen==Screen::Tracks&&model.tracks_during_playback))&&playback_started){outbox.put(model.detail,"stopped",model.detail.offset,std::time(nullptr),playback_session);for(auto &u:outbox.entries)if(u.item.id==model.detail.id){u.baseline=playback_baseline;u.baseline_viewed=playback_viewed;}persist_progress();}
 close_stream();
}
bool Session::close_stream(){
 if(stream.session.empty()){stream={};stream_base.clear();stream_token.clear();return true;}
 auto bounded=client.bounded(500,1000);
 auto result=plex::stream_control(bounded,stream_base,stream_token,stream.session,true);
 if(!result){model.message="Plex streaming session release failed; retry Stop before opening another title.";return false;}
 stream={};stream_base.clear();stream_token.clear();return true;
}
bool Session::open_playback(bool force){
 if(!close_stream())return false;
 model.playback_state=PlaybackState::Resolving;if(progress)progress(model);
 auto bounded=client.bounded(1200,5000);
 playback_baseline=model.detail.offset;playback_viewed=model.detail.last_played;
 auto session=device+"-"+std::to_string(++playback_serial)+"-"+std::to_string(static_cast<uint64_t>(now()*1000000));
 playback_session=session;
 stream=model.detail.type=="track"?plex::StreamPlan{plex::audio_direct_play(base,token,model.detail,model.media_version),"","Direct Play"}:
  plex::negotiate_video(bounded,base,token,model.detail,model.media_version,model.quality,local_connection,session,force,model.tracks);
 if(stream.playback.mode==plex::Mode::Unsupported){model.playback_state=PlaybackState::Error;fail(stream.playback.error.c_str());return false;}
 stream_base=base;stream_token=token;model.playback_mode=stream.playback.mode;
 model.playback_state=PlaybackState::Opening;if(progress)progress(model);
 player.prepare_audio(plex::selected_track(model.audio_tracks,model.tracks.audio));
 if(!player.open(stream.playback.url,model.detail.offset)){
  bool direct=stream.playback.mode==plex::Mode::DirectPlay;
  if(!close_stream()){model.playback_state=PlaybackState::Error;return false;}
  if(direct&&!force&&model.detail.type!="track")return open_playback(true);
  model.playback_state=PlaybackState::Error;fail("Player failed to open negotiated media");return false;
 }
 external_applied=false;tracks_pending=true;playback_started=false;heartbeat_at=now()+10;model.paused=false;model.screen=Screen::Playback;
 // Opening is acceptance of an asynchronous request, never proof of decoding.
 return true;
}
void Session::clear_context(){
 if((model.screen==Screen::Playback||(model.screen==Screen::Tracks&&model.tracks_during_playback))&&playback_started){outbox.put(model.detail,"stopped",model.detail.offset,std::time(nullptr),playback_session);persist_progress();}
 player.stop();close_stream();journal.release();outbox.entries.clear();playback_started=false;model.playback_state=PlaybackState::Released;current_profile.clear();model.profile_name.clear();model.profile_avatar.clear();active.clear();base.clear();token.clear();server_id.clear();resources.clear();history.clear();cached_home.clear();pages.clear();metadata.clear();channels.clear();live_parts.clear();channels_loaded=0;personal_channels=plex::Json::array();personal_loaded=0;model.guide={};home_loaded=0;
 model.audio_tracks.clear();model.subtitle_tracks.clear();model.tracks={};model.upcoming.clear();model.autoplay_remaining=0;next_deadline=0;
 model.sections.clear();model.entries.clear();model.rails.clear();model.navigation.clear();model.detail={};model.detail_rails.clear();model.library_id.clear();model.library_path.clear();model.server_name.clear();model.servers.clear();
}
bool Session::profiles(bool restore_saved){
 model.profiles.clear();model.screen=Screen::Profiles;
 auto r=client.home_users(account);if(!r){fail("Unable to load Plex Home profiles",r.status);return false;}
 model.profiles=plex::profiles(r.data);model.screen=Screen::Profiles;
 if(model.profiles.empty()){fail("Plex returned no selectable Home profiles");return false;}
 if(restore_saved&&!account_id.empty()){
  std::string owner,selected;
  if(plex::load_credentials(root+"/selected-profile.json",owner,selected)&&owner==account_id){
   auto found=std::find_if(model.profiles.begin(),model.profiles.end(),[&](const auto &profile){return profile.id==selected;});
   // Permissions and protection come only from this fresh authorized response.
   if(found!=model.profiles.end()&&!found->protected_user)return select_profile(selected,"");
  }
 }
 return true;
}
bool Session::select_profile(const std::string &id,const std::string &pin){
  auto p=std::find_if(model.profiles.begin(),model.profiles.end(),[&](const auto&p){return p.id==id;});
  if(p==model.profiles.end()){fail("Unknown profile");return false;}
  // Wipe all old authorized content before PIN entry or switching.
  clear_context();
  if(p->protected_user&&!plex::valid_pin(pin)){model.screen=Screen::Pin;return false;}
  auto r=client.switch_home(account,id,pin);
  if(!r){model.screen=Screen::Profiles;fail("Profile authorization failed",r.status);return false;}
  active=plex::text(r.data,"authenticationToken",plex::text(r.data,"authToken"));
  if(active.empty()){model.screen=Screen::Profiles;fail("Profile response has no authorization");return false;}
  current_profile=id;model.profile_name=p->name;model.profile_avatar=p->avatar;
  if(!account_id.empty()&&!plex::save_credentials(root+"/selected-profile.json",account_id,id))model.message="Profile selected, but the preference could not be saved.";
  return discover();
}
bool Session::discover(){
 auto r=client.resources(active);if(!r){fail("Server discovery failed",r.status);return false;}
 resources=plex::servers(r.data);model.servers.clear();
 for(const auto&s:resources)model.servers.push_back({s.id,s.name});
 model.screen=Screen::Servers;
 if(resources.empty()){fail("No authorized Plex servers found");return false;}
 std::string profile,remembered;
 if(!current_profile.empty()&&plex::load_credentials(root+"/server-"+current_profile+".json",profile,remembered)&&profile==current_profile&&
    std::any_of(resources.begin(),resources.end(),[&](const auto&s){return s.id==remembered;}))return connect(remembered);
 if(resources.size()==1)return connect(resources.front().id);
 return true;
}
bool Session::connect(const std::string&id){
 auto found=std::find_if(resources.begin(),resources.end(),[&](const auto&s){return s.id==id;});
 if(found==resources.end()){fail("Unknown server selection");return false;}
 if((model.screen==Screen::Playback||(model.screen==Screen::Tracks&&model.tracks_during_playback))&&playback_started)report_progress("stopped",model.detail.offset);
 player.stop();if(!close_stream())return false;outbox.entries.clear();playback_started=false;
 base.clear();token.clear();server_id.clear();history.clear();cached_home.clear();pages.clear();metadata.clear();channels.clear();live_parts.clear();channels_loaded=0;personal_channels=plex::Json::array();personal_loaded=0;model.guide={};home_loaded=0;model.sections.clear();model.rails.clear();model.entries.clear();model.navigation.clear();model.detail={};model.screen=Screen::Servers;
 if(found->token.empty())found->token=active;
 std::string selected;
 // Secure default: never silently downgrade to HTTP, even for local discovery.
 std::string route_id,preferred;
 if(!current_profile.empty()&&plex::load_credentials(root+"/route-"+current_profile+".json",route_id,preferred)&&route_id!=id)preferred.clear();
 auto r=client.bounded(1200,3500).select(*found,false,selected,nullptr,preferred);if(!r){fail("Cannot verify server identity and authorized libraries",r.status);return false;}
 local_connection=std::any_of(found->connections.begin(),found->connections.end(),[&](const auto &c){return c.uri==selected&&c.local;});
 base=selected;token=found->token.empty()?active:found->token;server_id=id;
 audio_language.clear();subtitle_language.clear();prefer_subtitles=false;model.autoplay=true;
 track_preferences_scope=account_id+"/"+current_profile+"/"+server_id;
 if(!journal.bind(root,track_preferences_scope,outbox,std::time(nullptr),now()))model.message="Saved progress is unavailable or has an unsupported schema; it was not overwritten.";
 plex::Json preferences;if(read_private_json(root+"/playback-preferences.json",preferences)&&plex::number(preferences,"schema")==1){auto all=preferences.find("profiles");if(all!=preferences.end()&&all->is_object()){auto settings=all->find(track_preferences_scope);if(settings!=all->end()){audio_language=plex::text(*settings,"audio_language");subtitle_language=plex::text(*settings,"subtitle_language");prefer_subtitles=plex::flag(*settings,"subtitles");model.autoplay=plex::flag(*settings,"autoplay",true);model.quality=static_cast<plex::Quality>(std::clamp<int64_t>(plex::number(*settings,"quality"),0,6));}}}
 flush_progress();
 model.server_name=found->name;model.sections=plex::items(r.data,false);
 model.navigation=navigation(model.sections);
 if(!current_profile.empty())plex::save_credentials(root+"/route-"+current_profile+".json",id,selected);
 if(!current_profile.empty())plex::save_credentials(root+"/server-"+current_profile+".json",current_profile,server_id);return home();
}
bool Session::home(bool refresh){
 model.screen=Screen::Home;model.entries.clear();model.library_id.clear();model.library_tab.clear();model.books=false;
 if(base.empty()){fail("Select a server first");return false;}
 if(!refresh&&!cached_home.empty()&&now()-home_loaded<60){model.rails=cached_home;return true;}
 model.rails=cached_home;if(progress)progress(model);
 auto bounded=client.bounded(1200,2200);auto deadline=now()+10;
 auto r=plex::home_feed(bounded,base,token,model.sections,[&]{return now()<deadline;},[&](const plex::Json&data){
  model.rails=plex::home_rows(data);if(progress)progress(model);
 },true);
 if(r)model.rails=plex::home_rows(r.data);
 if(!model.rails.empty()){cached_home=model.rails;home_loaded=now();return true;}
 if(!r){fail("Home is temporarily unavailable; open a library or retry",r.status);return false;}
 return true;
}
bool Session::listing(const std::string &path,int start){
 auto r=client.bounded(1500,5000).browse(base,token,path,start,60);if(!r){fail("Media request failed",r.status);return false;}
 model.entries=plex::items(r.data,path.rfind("/library/metadata/",0)==0&&path.find("/children")==std::string::npos);model.page_start=start;
 const auto container=r.data.is_object()?r.data.value("MediaContainer",plex::Json::object()):plex::Json::object();
 model.total=std::max<int64_t>(start+model.entries.size(),plex::number(container,"totalSize",start+model.entries.size()));
 model.library_path=path;return true;
}
bool Session::library(const std::string &id,const std::string &requested){
 auto section=std::find_if(model.sections.begin(),model.sections.end(),[&](const auto &i){return i.id==id;});
 if(section==model.sections.end()){fail("Library is not authorized");return false;}
 auto tab=requested.empty()?"recommended":requested;
 auto tabs=library_tabs(section->type,audiobook_library(section->title));
 if(std::none_of(tabs.begin(),tabs.end(),[&](const auto &t){return t.first==tab;})){fail("Unknown library view");return false;}
 auto key=id+":"+tab;auto cached=pages.find(key);
 if(cached!=pages.end()&&now()-cached->second.loaded<180){model=cached->second.value;return true;}
 model.screen=Screen::Library;model.library_id=id;model.library_type=section->type;model.library_title=section->title;model.library_tab=tab;
 model.books=audiobook_library(section->title);model.entries.clear();model.rails.clear();model.page_start=0;model.total=0;model.library_path.clear();
 if(progress)progress(model);
 auto bounded=client.bounded(1200,2500);auto path="/library/sections/"+plex::encode(id);
 if(tab=="recommended"){
  auto r=bounded.call("GET",base,"/hubs/sections/"+plex::encode(id)+"?count=12&includeEmpty=0",token);
  if(r)model.rails=recommendation_rows(r.data,id);
  if(model.rails.empty()){
   bool music=section->type=="artist";
   for(const auto &query:std::vector<std::pair<std::string,std::string>>{
       {"Recently Added",path+"/all?sort=addedAt:desc&type="+(music?"9":section->type=="show"?"2":"1")},
       {music?(model.books?"Authors":"Artists"):"Top Rated",path+"/all?sort="+(music?"titleSort:asc&type=8":"rating:desc")} }){
    auto response=bounded.browse(base,token,query.second,0,12);
    if(response){auto entries=plex::items(response.data,false);if(!entries.empty())model.rails.push_back({query.first,query.first,false,std::move(entries)});}
    if(progress)progress(model);
   }
  }
 }else if(tab=="genres"){
  auto response=bounded.call("GET",base,path+"/genre"+(section->type=="artist"?"?type=9":""),token);
  if(!response){fail("Genres unavailable",response.status);return false;}
  auto mc=response.data.find("MediaContainer");
  if(mc!=response.data.end()&&mc->contains("Directory")&&(*mc)["Directory"].is_array())for(const auto &genre:(*mc)["Directory"]){
   auto value=plex::text(genre,"key");if(value.empty())continue;
   plex::Item item;item.id="genre";item.type="category";item.title=plex::text(genre,"title",plex::text(genre,"tag"));
   item.key=path+"/all?"+(section->type=="artist"?"type=9&":"")+"genre="+plex::encode(value);model.entries.push_back(std::move(item));if(model.entries.size()==100)break;
  }
 }else{
  std::string query=path+(tab=="collections"?"/collections":"/all?sort="+std::string(tab=="recent"?"addedAt:desc":"titleSort:asc"));
  if(tab=="unwatched")query+="&unwatched=1";
  if(tab=="artists"||tab=="albums"||tab=="tracks")query+="&type="+std::string(tab=="artists"?"8":tab=="albums"?"9":"10");
  if(!listing(query))return false;
 }
 if(pages.size()>=12)pages.erase(pages.begin());pages[key]={model,now()};return true;
}
MediaContext Session::media_context()const{
 bool live=(model.screen==Screen::Library&&model.library_tab=="live")||(model.screen==Screen::Playback&&model.detail.type=="channel");
 return {live?plex::free_tv_origin:base,live?active:token,account+"/"+current_profile+"/"+server_id+(live?"/live":""),account};
}
void Session::channel_page(int start){
 model.page_start=std::clamp(start,0,std::max(0,int(channels.size())-1));model.total=int(channels.size());model.entries.clear();
 for(int i=model.page_start;i<std::min(model.total,model.page_start+60);++i)model.entries.push_back(channels[i]);
}
bool Session::live_tv(int offset,int64_t begins,bool refresh){
 model.screen=Screen::Guide;model.message.clear();
 auto previous_guide=model.guide;model.guide.channels.clear();model.guide.stale=false;
 if(base.empty() || token.empty()){model.guide={};fail("Select an authorized Plex server first");return false;}
 const auto wall=int64_t(std::time(nullptr));
 auto retain_cached=[&]{if(previous_guide.refreshed>0&&wall>=previous_guide.refreshed&&wall-previous_guide.refreshed<=900&&!previous_guide.channels.empty()){model.guide=previous_guide;model.guide.stale=true;model.message="Plex guide is unavailable; showing the last successful cached page. Options retries.";}};
 begins=std::clamp<int64_t>(begins,wall-24*3600,wall+7*24*3600);begins-=begins%1800;
 model.guide.begins=begins;model.guide.ends=begins+7200;
 auto bounded=client.bounded(800,1500);auto deadline=now()+8;bool guide_ok=true;
 if(refresh || personal_loaded==0 || now()-personal_loaded>120){
  personal_loaded=0;personal_channels=plex::Json::array();model.guide.configured=false;model.guide.total=0;
  auto response=plex::live_channels(bounded,base,token);
  if(!response){fail("Plex Live TV discovery unavailable",response.status);
   if(response.status==401||response.status==403)model.guide={};
   else retain_cached();return false;
  }
  model.guide.configured=plex::flag(response.data,"configured");
  auto channels=response.data.find("channels");if(channels!=response.data.end()&&channels->is_array())personal_channels=*channels;
  std::stable_sort(personal_channels.begin(),personal_channels.end(),[](const auto&a,const auto&b){auto an=plex::channel_order(plex::text(a,"number")),bn=plex::channel_order(plex::text(b,"number"));return an==bn?plex::text(a,"title")<plex::text(b,"title"):an<bn;});
  personal_loaded=now();
 }
 model.guide.total=int(personal_channels.size());
 if(!model.guide.configured){model.message="No Live TV & DVR configuration was found on this Plex server. Set up your tuner and TV guide through Plex Web.";return true;}
 offset=std::clamp(offset,0,std::max(0,model.guide.total-1));offset-=offset%6;model.guide.offset=offset;
 if(progress)progress(model);
 for(int i=offset;i<std::min(offset+6,model.guide.total);++i){
  const auto &source=personal_channels[i];plex::GuideChannel channel;
  channel.id=plex::text(source,"id");channel.title=plex::text(source,"title");channel.number=plex::text(source,"number");channel.thumb=plex::text(source,"thumb");
  plex::Json programs=plex::Json::array();
  auto first=plex::guide_date(begins),last=plex::guide_date(model.guide.ends-1);
  for(const auto &date:std::vector<std::string>{first,first==last?"":last}){
   if(date.empty())continue;
   if(now()>=deadline){guide_ok=false;continue;}
   auto response=plex::live_guide(bounded,base,token,source,date,begins,model.guide.ends);
   if(response){auto entries=response.data.find("programs");if(entries!=response.data.end()&&entries->is_array())for(const auto &program:*entries)if(programs.size()<200)programs.push_back(program);}
   else {guide_ok=false;
    if(response.status==401||response.status==403){model.guide={};personal_channels=plex::Json::array();personal_loaded=0;fail("Plex guide access is no longer authorized",response.status);return false;}
    model.message="Some Plex guide information is unavailable. Options refreshes the guide.";
   }
  }
  channel.programs=plex::guide_programs(plex::Json{{"programs",std::move(programs)}},begins,model.guide.ends);
  model.guide.channels.push_back(std::move(channel));if(progress)progress(model);
 }
 if(guide_ok)model.guide.refreshed=wall;
 else if(model.message.empty())model.message="Guide loading reached its time budget. Options retries the current page.";
 if(!guide_ok)retain_cached();
 if(model.guide.total==0)model.message="Plex DVR is configured, but no accessible channels were returned.";
 return true;
}
bool Session::free_tv(bool refresh){
 if(active.empty()){fail("Select a Plex profile before opening Free Live TV");return false;}
 model.screen=Screen::Library;model.library_id.clear();model.library_tab="live";model.library_type="channel";model.library_title="Plex Free Live TV · Experimental";
 model.library_path.clear();model.rails.clear();model.books=false;
 if(refresh||channels.empty()||now()-channels_loaded>120){
  auto response=client.bounded(2000,6000).call("GET",plex::free_tv_origin,"/lineups/plex/channels?includeMetadata=1",active);
  if(!response){model.entries.clear();model.total=0;fail("Plex Free Live TV is unavailable for this profile, region or client",response.status);return false;}
  channels.clear();live_parts.clear();
  for(auto &channel:plex::free_channels(response.data)){live_parts[channel.item.id]=std::move(channel.part);channels.push_back(std::move(channel.item));}
  channels_loaded=now();
 }
 channel_page(0);
 if(channels.empty())model.message="Plex returned no supported non-DRM channels for this profile and region.";
 return true;
}
void Session::remember_position(int64_t position){
 auto update=[&](plex::Item &i){if(i.id==model.detail.id)i.offset=position;};
 auto rows=[&](std::vector<plex::Hub> &values){for(auto &row:values){for(auto &i:row.entries)update(i);if(row.continuing)row.entries.erase(std::remove_if(row.entries.begin(),row.entries.end(),[](const auto &i){return i.duration>0&&i.offset>=i.duration-1000;}),row.entries.end());}};
 rows(cached_home);cached_home.erase(std::remove_if(cached_home.begin(),cached_home.end(),[](const auto &r){return r.entries.empty();}),cached_home.end());
 for(auto &page:pages){for(auto &i:page.second.value.entries)update(i);rows(page.second.value.rails);rows(page.second.value.detail_rails);}
}
Model Session::dispatch(const Command &c){return dispatch_current(c);}
// Runtime borrows this worker-owned view only until its locked copy completes.
__attribute__((noinline)) const Model &Session::dispatch_current(const Command &c){
 model.message.clear();
 switch(c.action){
 case Action::Restore:{
  clear_context();account.clear();account_id.clear();model=Model{};
  if(!private_root()){fail("Private app storage is unavailable");break;}
  std::string ignored;
  if(!plex::load_credentials(root+"/device.json",device,ignored)){
   std::ifstream random("/dev/urandom",std::ios::binary);unsigned char bytes[16]{};
   if(!random.read(reinterpret_cast<char*>(bytes),16)){fail("Secure device identity unavailable");break;}
   const char *hex="0123456789abcdef";for(auto b:bytes){device+=hex[b>>4];device+=hex[b&15];}
   if(!plex::save_credentials(root+"/device.json",device,"")){fail("Cannot save device identity");break;}
  }
  // Keep the same transport while activating the persisted device identifier.
  client.set_client_id(device);
  if(plex::load_credentials(root+"/account.json",ignored,account)&&!account.empty()){
   auto r=client.validate_account(account);if(r){account_id=plex::profile_id(r.data);profiles(true);}else{account.clear();fail("Saved account needs sign-in",r.status);}
  }
  break;}
 case Action::Link:
  if(device.empty()){fail("Initialize secure storage before linking");break;}
  auth.begin(now());auth.created(client.create_pin(),now());model.screen=Screen::Linking;model.code=auth.current.code;
  model.expires=auth.remaining(now());if(auth.state==plex::AuthState::Failed)fail("Device linking failed");break;
 case Action::Poll:
  if(auth.poll_due(now()))auth.polled(client.poll_pin(auth.current.id,auth.current.code),now());
  model.expires=auth.remaining(now());
  if(auth.state==plex::AuthState::Authorized){
   account=auth.current.token;model.code.clear();auth.cancel();
   auto identity=client.validate_account(account);
   if(!identity){account.clear();fail("Linked account could not be validated",identity.status);break;}
   account_id=plex::profile_id(identity.data);
   if(!plex::save_credentials(root+"/account.json",device,account)){account.clear();fail("Cannot persist account securely");break;}profiles();
  }else if(auth.state==plex::AuthState::Failed||auth.state==plex::AuthState::Expired){model.code.clear();fail("Activation expired or failed; press Cross to retry");}
  break;
 case Action::Cancel:auth.cancel();model.code.clear();model.screen=Screen::Welcome;break;
 case Action::Profile:select_profile(c.id,c.text);break;
 case Action::Server:if(c.id.empty()){model.screen=Screen::Servers;}else connect(c.id);break;
 case Action::Home:if(c.id=="profiles"){clear_context();profiles();}else home(c.text=="refresh");break;
 case Action::Library:{
  if(!c.id.empty()&&c.id!=model.library_id){if(history.size()>=16)history.erase(history.begin());history.push_back(model);}
  library(c.id,c.text);break;}
 case Action::Browse:{
  const auto prefix="/library/sections/"+plex::encode(model.library_id)+"/";
  if(model.library_id.empty()||c.text.rfind(prefix,0)!=0||c.text.find("..")!=std::string::npos){fail("Invalid library category");break;}
  if(history.size()>=16)history.erase(history.begin());history.push_back(model);auto category=std::find_if(model.entries.begin(),model.entries.end(),[&](const auto &i){return i.key==c.text;});if(category!=model.entries.end())model.library_title+=" · "+category->title;model.library_tab="category";listing(c.text);break;}
 case Action::LiveTV:live_tv(0,std::time(nullptr),c.text=="refresh");break;
 case Action::GuidePage:{
  if(model.screen!=Screen::Guide)break;
  auto offset=model.guide.offset;auto begins=model.guide.begins;
  if(c.id=="next")offset+=6;else if(c.id=="previous")offset-=6;
  else if(c.id=="later")begins+=3600;else if(c.id=="earlier")begins-=3600;
  else if(c.id=="now")begins=std::time(nullptr);
  live_tv(offset,begins,c.id=="refresh");break;
 }
 case Action::TuneChannel:model.message="Personal Live TV playback is pending verified Plex session lifecycle support.";break;
 case Action::FreeTV:free_tv(c.text=="refresh");break;
 case Action::PlayLive:{
  if(now()-channels_loaded>120&&!free_tv(true))break;
  auto selected=std::find_if(channels.begin(),channels.end(),[&](const auto &i){return i.id==c.id;});
  if(selected==channels.end()||!live_parts.count(c.id)){fail("Channel is no longer available");break;}
  auto path=live_parts.at(c.id);auto url=std::string(plex::free_tv_origin)+path+(path.find('?')==std::string::npos?"?":"&")+"X-Plex-Token="+plex::encode(active)+"&X-Plex-Client-Identifier="+plex::encode(device)+"&X-Plex-Product=Plexium";
  if(!player.open(url,0)){fail("The Plex live stream could not be opened");break;}
  model.detail=*selected;model.paused=false;model.screen=Screen::Playback;break;}
 case Action::Page:
  if(model.library_tab=="live"){channel_page(model.page_start+(c.id=="previous"?-60:60));break;}
  if(!model.library_path.empty()){int start=std::max(0,model.page_start+(c.id=="previous"?-60:60));if(start<model.total)listing(model.library_path,start);}break;
 case Action::Search:
  model.screen=Screen::Search;model.query=c.text;model.entries.clear();model.library_title=c.text.empty()?"Search results":"Results for: "+c.text;model.books=false;model.library_id.clear();model.library_tab.clear();model.rails.clear();if(!c.text.empty()){listing("/search?query="+plex::encode(c.text));model.screen=Screen::Library;}break;
 case Action::Details:
  next_deadline=0;model.autoplay_remaining=0;
  if(c.id.empty()||c.id.find_first_not_of("0123456789")!=std::string::npos){fail("Invalid media identity");break;}
  if(history.size()>=16)history.erase(history.begin());history.push_back(model);
  {auto cached=pages.find("detail:"+c.id);if(cached!=pages.end()&&now()-cached->second.loaded<180){model=cached->second.value;load_tracks();break;}}
  if(listing((c.text=="collection"?"/library/collections/":c.text=="playlist"?"/playlists/":"/library/metadata/")+c.id+(c.text=="collection"||c.text=="playlist"?"":"?includeMarkers=1"))&&!model.entries.empty()){
   model.detail=model.entries.front();model.media_version=0;load_tracks();model.entries.clear();model.detail_rails.clear();model.screen=Screen::Details;if(progress)progress(model);
   if(metadata.size()>=32)metadata.erase(metadata.begin());metadata[c.id]={model.detail,now()};
   if(model.detail.type=="episode"&&(!model.detail.grandparent_id.empty())&&(model.detail.credits.size()<3||model.detail.grandparent_thumb.empty())){
    auto parent=model.detail.grandparent_id;
    if(parent.find_first_not_of("0123456789")==std::string::npos){
     auto found=metadata.find(parent);
     if(found==metadata.end()||now()-found->second.second>300){
      auto response=client.bounded(1000,2000).browse(base,token,"/library/metadata/"+parent,0,1);
      if(response){auto values=plex::items(response.data,false);if(!values.empty()){if(metadata.size()>=32)metadata.erase(metadata.begin());metadata[parent]={values.front(),now()};}}
     }
     found=metadata.find(parent);if(found!=metadata.end()){
      const auto &show=found->second.first;
      if(model.detail.series.empty())model.detail.series=show.title;
      if(model.detail.grandparent_thumb.empty())model.detail.grandparent_thumb=show.thumb;
      if(model.detail.grandparent_art.empty())model.detail.grandparent_art=show.art;
      for(const auto &credit:show.credits)if(model.detail.credits.size()<32&&std::none_of(model.detail.credits.begin(),model.detail.credits.end(),[&](const auto &c){return c.name==credit.name;}))model.detail.credits.push_back(credit);
      for(const auto &name:show.cast)if(model.detail.cast.size()<32&&std::find(model.detail.cast.begin(),model.detail.cast.end(),name)==model.detail.cast.end())model.detail.cast.push_back(name);
     }
     if(progress)progress(model);
    }
   }
   auto bounded=client.bounded(1000,2000);auto type=model.detail.type;
   if(type=="show"||type=="season"||type=="artist"||type=="album"||type=="collection"||type=="playlist"){
    std::string path=type=="collection"?"/library/collections/":type=="playlist"?"/playlists/":"/library/metadata/";
    auto response=bounded.browse(base,token,path+c.id+(type=="playlist"?"/items":"/children"),0,12);
    if(response){auto children=plex::items(response.data,false);if(!children.empty())model.detail_rails.push_back({"children",type=="show"?"Seasons":type=="season"?"Episodes":type=="artist"?(model.books?"Books":"Albums"):(type=="album"||type=="playlist")?(model.books?"Chapters":"Tracks"):"Collection",false,std::move(children)});}
    if(progress)progress(model);
   }
   if(type=="movie"||type=="episode"||type=="show"){
    auto response=bounded.call("GET",base,"/library/metadata/"+c.id+"/related?count=12",token);
    if(response){auto rows=recommendation_rows(response.data);for(auto &r:rows)model.detail_rails.push_back(std::move(r));}
   }
   if(pages.size()>=12)pages.erase(pages.begin());pages["detail:"+c.id]={model,now()};
  }break;
 case Action::Children:
  if(c.id.empty()||c.id.find_first_not_of("0123456789")!=std::string::npos){fail("Invalid parent identity");break;}
  if(history.size()>=16)history.erase(history.begin());history.push_back(model);model.screen=Screen::Library;model.library_title=model.detail.title;model.library_id.clear();model.library_tab.clear();model.rails.clear();
  listing((model.detail.type=="collection"?"/library/collections/":model.detail.type=="playlist"?"/playlists/":"/library/metadata/")+c.id+(model.detail.type=="playlist"?"/items":"/children"));break;
 case Action::Version:
  if(model.screen==Screen::Details&&model.detail.media.is_array()&&!model.detail.media.empty()){model.media_version=(model.media_version+1)%model.detail.media.size();load_tracks();}break;
 case Action::Quality:{auto next=(static_cast<int>(model.quality)+1)%7;if(next==2)next=3;model.quality=static_cast<plex::Quality>(next);save_preferences();break;}
 case Action::Play:{
  next_deadline=0;model.autoplay_remaining=0;model.upcoming.clear();
  if(model.detail.type!="movie"&&model.detail.type!="episode"&&model.detail.type!="track"){fail("Select a playable item");break;}
  open_playback();break;}
 case Action::Stop:{
  if(model.screen!=Screen::Playback&&stream.session.empty())break;
  model.playback_state=PlaybackState::Stopping;
  pages.erase("detail:"+model.detail.id);auto position=player.position();
  if(!c.text.empty())position=std::clamp<int64_t>(plex::number(plex::Json{{"at",c.text}},"at",position),0,model.detail.duration>0?model.detail.duration:INT64_MAX);
  const auto observation=player.observe();if(c.id=="ended"&&observation.ended&&!observation.failed&&model.detail.duration>0)position=model.detail.duration;player.stop();
  if(c.id=="ended"&&observation.failed&&!playback_started&&model.playback_mode==plex::Mode::DirectPlay&&model.detail.type!="track"&&model.detail.type!="channel"){
   open_playback(true);break;
  }
  if(model.detail.type!="channel"){
   if(playback_started||!observation.known){remember_position(position);report_progress("stopped",position);}
   else position=model.detail.offset;
  }
  bool released=close_stream();model.playback_state=released?PlaybackState::Stopped:PlaybackState::Error;
  model.detail.offset=position;model.screen=model.detail.type=="channel"?Screen::Library:Screen::Details;
  if(released&&c.id=="ended"&&observation.ended&&!observation.failed&&playback_started&&model.detail.type=="episode")prepare_next();
  if(model.detail.type=="channel"&&c.id=="ended"){channels_loaded=0;model.message="The live stream ended or could not be decoded. Select the channel again to reconnect.";}
  break;}
 case Action::Pause:
  if(model.screen!=Screen::Playback)break;
  player.pause();model.paused=player.paused();model.playback_state=model.paused?PlaybackState::Paused:PlaybackState::Playing;
  if(playback_started||!player.observe().known)report_progress(model.paused?"paused":"playing",player.position());break;
 case Action::Seek:
  if(model.screen!=Screen::Playback||model.detail.type=="channel")break;
  model.playback_state=PlaybackState::Seeking;player.seek(std::clamp<int64_t>(player.position()+(c.id=="back"?-10000:10000),0,model.detail.duration>0?model.detail.duration:INT64_MAX));
  if(playback_started||!player.observe().known)report_progress(player.paused()?"paused":"playing",player.position());break;
 case Action::Audio:case Action::Subtitle:case Action::Tracks:
  if(model.screen==Screen::Details||model.screen==Screen::Playback){model.tracks_during_playback=model.screen==Screen::Playback;next_deadline=0;model.autoplay_remaining=0;model.screen=Screen::Tracks;}break;
 case Action::SelectAudio:case Action::SelectSubtitle:{
  if(model.screen!=Screen::Tracks)break;bool subtitle=c.action==Action::SelectSubtitle;
  const auto &list=subtitle?model.subtitle_tracks:model.audio_tracks;const auto *track=plex::selected_track(list,c.id);
  if(!track&&!(subtitle&&c.id=="off")){fail("Selected stream is no longer available");break;}
  if(subtitle){external_applied=false;model.tracks.subtitle=track?track->id:"";prefer_subtitles=track!=nullptr;subtitle_language=track?track->language:"";}
  else{model.tracks.audio=track->id;audio_language=track->language;}
  save_preferences();
  if(model.tracks_during_playback){
   model.detail.offset=player.position();
   bool server=model.playback_mode!=plex::Mode::DirectPlay||(track&&((subtitle&&!plex::text_subtitle(*track))||(!subtitle&&!plex::VideoCapabilities{}.audio(track->codec))));
   if(server){report_progress("paused",model.detail.offset);player.stop();model.screen=Screen::Playback;open_playback(true);}
   else{tracks_pending=true;apply_tracks();}
  }break;
 }
 case Action::AutoPlay:model.autoplay=!model.autoplay;if(!model.autoplay){next_deadline=0;model.autoplay_remaining=0;}save_preferences();break;
 case Action::CancelNext:next_deadline=0;model.autoplay_remaining=0;break;
 case Action::PlayNext:play_next();break;
 case Action::SkipMarker:
  if(model.screen==Screen::Playback)for(const auto &marker:model.detail.markers)if(model.detail.offset>=marker.begins&&model.detail.offset<marker.ends){player.seek(marker.ends);model.detail.offset=marker.ends;report_progress("playing",marker.ends);break;}break;
 case Action::Tick:
  if(next_deadline>0){model.autoplay_remaining=int(std::max(0.,std::ceil(next_deadline-now())));if(now()>=next_deadline){play_next();break;}}
  if(model.screen!=Screen::Playback&&!(model.screen==Screen::Tracks&&model.tracks_during_playback))flush_progress();
  if(model.screen==Screen::Playback||(model.screen==Screen::Tracks&&model.tracks_during_playback)){
   apply_tracks();
   model.detail.offset=player.position();model.paused=player.paused();auto observed=player.observe();
   if(observed.ended&&!observed.failed){model.screen=Screen::Playback;return dispatch_current({Action::Stop,"ended",{}});}
   if(observed.failed){model.playback_state=PlaybackState::Error;model.message="Player decoding failed; stop and retry with a different quality.";}
   else if(observed.buffering)model.playback_state=PlaybackState::Buffering;
   else if(observed.playing||observed.paused){playback_started=true;model.playback_state=observed.paused?PlaybackState::Paused:PlaybackState::Playing;}
   if(!observed.failed&&(playback_started||!observed.known))report_progress(model.paused?"paused":"playing",model.detail.offset);else flush_progress();
   if(!stream.session.empty()&&now()>=heartbeat_at){auto bounded=client.bounded(500,1000);if(!plex::stream_control(bounded,stream_base,stream_token,stream.session,false))model.message="Plex streaming heartbeat failed; playback may need to reconnect.";heartbeat_at=now()+10;}
  }break;
 case Action::Settings:if(history.size()>=16)history.erase(history.begin());history.push_back(model);model.screen=Screen::Settings;break;
 case Action::SignOut:
  clear_context();account.clear();account_id.clear();auth.cancel();unlink((root+"/account.json").c_str());unlink((root+"/selected-profile.json").c_str());model=Model{};break;
 case Action::Back:
  next_deadline=0;model.autoplay_remaining=0;
  if(model.screen==Screen::Tracks){model.screen=model.tracks_during_playback?Screen::Playback:Screen::Details;break;}
  if(!history.empty()){model=history.back();history.pop_back();}
  else if(model.screen==Screen::Pin)model.screen=Screen::Profiles;
  else if(model.screen==Screen::Linking){auth.cancel();model.code.clear();model.screen=Screen::Welcome;}
  else if(!base.empty())home();else model.screen=Screen::Welcome;
  break;
 }
 return model;
}
Runtime::Runtime(std::unique_ptr<Session>s):session(std::move(s)){
 worker_error=worker.start([this]{run();},Worker::session_stack);
 if(worker_error)model.message="Session worker could not start";
}
Runtime::~Runtime(){close();}
bool Runtime::close(std::chrono::milliseconds timeout){
 {std::lock_guard<std::mutex> l(mutex);closing=true;++generation;queue.clear();}
 cv.notify_all();
 {std::lock_guard<std::mutex> l(mutex);model=Model{};context={};}
 return worker.wait(timeout);
}
bool Runtime::send(Command command){
 std::lock_guard<std::mutex> l(mutex);
 if(closing || worker_error)return false;
 if(command.action==Action::Tick)for(const auto &pending:queue)if(pending.second.action==Action::Tick)return false;
 if((command.action==Action::Back||command.action==Action::Stop)&&model.busy){++generation;queue.clear();}
 else if(command.action==Action::Cancel||command.action==Action::SignOut){++generation;queue.clear();model=Model{};context={};}
 else if(model.busy || queue.size()>=16)return false;
 if(command.action!=Action::Tick)model.busy=true;queue.push_back({generation,std::move(command)});cv.notify_one();return true;
}
Model Runtime::snapshot()const{std::lock_guard<std::mutex> l(mutex);return model;}
void Runtime::snapshot_into(Model &destination,MediaContext &access)const{std::lock_guard<std::mutex> l(mutex);destination=model;access=context;}
MediaContext Runtime::media_context()const{std::lock_guard<std::mutex> l(mutex);return context;}
void Runtime::run(){
 for(;;){std::pair<uint64_t,Command> next;
  {std::unique_lock<std::mutex> l(mutex);cv.wait(l,[&]{return closing||!queue.empty();});if(closing){l.unlock();session->shutdown();return;}next=std::move(queue.front());queue.pop_front();}
  session->progress=[this,epoch=next.first](const Model &updated){
   std::lock_guard<std::mutex> l(mutex);if(epoch!=generation)return;
   model=updated;model.busy=!queue.empty()||updated.screen!=Screen::Details;context=session->media_context();
  };
  const auto &updated=session->dispatch_current(next.second);session->progress={};
  {std::lock_guard<std::mutex> l(mutex);if(next.first==generation){model=updated;context=session->media_context();model.busy=!queue.empty();}}
 }
}
}
