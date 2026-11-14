// SPDX-License-Identifier: GPL-3.0-or-later
#include "session.h"
#include "catalog.h"
#include "plex/episodes.h"
#include "plex/surface_budget.h"
#include <ctime>
#include <charconv>
#ifdef PLEX_PS5
extern "C" const char *plat_data_dir(void);
#endif
#if __has_include("build_identity.h")
#include "build_identity.h"
#endif
#ifndef PLEX_BUILD_COMMIT
#define PLEX_BUILD_COMMIT "not-stamped"
#endif
#ifndef PLEX_SOURCE_DATE
#define PLEX_SOURCE_DATE "not-stamped"
#endif
#ifndef PLEX_APP_VERSION
#define PLEX_APP_VERSION "not-stamped"
#endif
namespace plexium {
__attribute__((noinline)) void Session::decorate(){
 model.connected=connection_verified;model.quality=preferred_quality;model.autoplay=preferred_autoplay;model.preferences=preferences.value;model.build_sha=PLEX_BUILD_COMMIT;model.build_date=PLEX_SOURCE_DATE;model.app_version=PLEX_APP_VERSION;
#ifdef PLEX_PS5
 model.runtime_info="Native PS5 · hardware acceptance pending · 1080p negotiation cap";
#else
 model.runtime_info="Host application · software/fixture validation · 1080p negotiation cap";
#endif
 model.account_label=account_name.empty()?"Linked Plex account":account_name;
 model.audio_preference=audio_language.empty()?"Server default":audio_language;model.subtitle_preference=prefer_subtitles?subtitle_language:"Off";
 model.music_active=audio_active;model.music_now.clear();model.music_preview.clear();model.music_total=int(audio_queue.order.size());model.music_cursor=int(audio_queue.cursor);model.music_shuffle=audio_queue.shuffled;model.music_repeat=int(audio_queue.repeat);
 if(audio_active){auto item=playing_audio;item.media={};model.music_now.push_back(std::move(item));for(size_t n=audio_queue.cursor;n<audio_queue.order.size()&&model.music_preview.size()<8;++n)model.music_preview.push_back(audio_queue.entries[audio_queue.order[n]]);}
}
__attribute__((noinline)) void Session::notice_command(const Command &c){
#ifdef PLEX_PS5
 auto location=std::string(plat_data_dir());
#else
 auto location=std::string(".");
#endif
 if(c.action==Action::Notices){if(model.screen!=Screen::Notices){if(history.size()>=16)history.erase(history.begin());history.push_back(model);}model.notice_body.clear();model.notice_labels.clear();notice_index=package_notices(location);for(const auto &entry:notice_index)model.notice_labels.push_back(entry.label);model.screen=Screen::Notices;return;}
 size_t at=0;auto parsed=std::from_chars(c.id.data(),c.id.data()+c.id.size(),at);if(parsed.ec!=std::errc()||parsed.ptr!=c.id.data()+c.id.size()||at>=notice_index.size()){model.message="Select an installed license notice.";return;}
 if(!resource_text(location,notice_index[at].path,65536,model.notice_body,true))model.message="This installed notice could not be read safely.";
}
__attribute__((noinline)) void Session::search_command(const Command &c){
 if(c.text.empty()){model.search_scope=c.id;model.query.clear();model.entries.clear();model.rails.clear();model.screen=Screen::Search;return;}
 if(c.text.size()>100){model.message="Search is limited to 100 characters.";return;}
 auto scope=c.id.empty()?model.search_scope:c.id;
 if(!scope.empty()&&std::none_of(model.sections.begin(),model.sections.end(),[&](const auto&s){return s.id==scope;})){model.message="Search library is not authorized.";return;}
 auto key="search:"+scope+":"+c.text;auto cached=pages.find(key);
 if(c.action!=Action::Suggest&&cached!=pages.end()&&now()-cached->second.loaded<60){model=cached->second.value;return;}
 model.query=c.text;model.search_scope=scope;model.entries.clear();model.rails.clear();model.books=false;
 auto bounded=client.bounded(800,1500);auto path="/hubs/search?query="+plex::encode(c.text)+"&limit=12"+(scope.empty()?"":"&sectionId="+plex::encode(scope));
 if(c.action==Action::Suggest&&!scope.empty())path="/library/sections/"+plex::encode(scope)+"/autocomplete?title.query="+plex::encode(c.text);
 auto result=bounded.call("GET",base,path,token);
 if(result)model.rails=search_rows(result.data,model.sections,scope);
 if(model.rails.empty()){
  auto fallback=bounded.browse(base,token,scope.empty()?"/search?query="+plex::encode(c.text):"/library/sections/"+plex::encode(scope)+"/all?title="+plex::encode(c.text),0,60);
  if(fallback){auto entries=plex::items(fallback.data,false);if(!scope.empty())for(auto &item:entries)item.section_id=scope;if(!entries.empty())model.rails.push_back({"results","Results",false,std::move(entries)});}
  else {model.connected=false;connection_verified=false;model.message="Search is unavailable. Check your server connection and retry.";}
 }
 for(const auto &rail:model.rails)for(const auto &item:rail.entries){if(model.entries.size()>=60)break;model.entries.push_back(item);}
 model.screen=Screen::Library;model.library_id.clear();model.library_tab="results";model.library_path.clear();model.library_title=(scope.empty()?"Search: ":"Library search: ")+c.text;model.page_start=0;model.total=int(model.entries.size());
 if(model.entries.empty()&&model.message.empty())model.message="No accessible matches. Try a different title or library.";
 if(pages.size()>=12)pages.erase(pages.begin());pages[key]={model,now()};
}
__attribute__((noinline)) void Session::settings_command(const Command &c){
 if(c.action==Action::Diagnose){auto bounded=client.bounded(700,1200);auto identity=bounded.identity(base,token,server_id);auto libraries=identity?bounded.sections(base,token):plex::Result{{},"Identity verification failed",identity.status};model.connected=bool(identity)&&bool(libraries);connection_verified=model.connected;model.message=model.connected?"Server identity and library authorization verified over HTTPS.":"Server connection could not be verified. Select your server again.";return;}
 if(c.id=="theme")preferences.value.theme=(preferences.value.theme+1)%3;
 else if(c.id=="safe_area")preferences.value.safe_area=preferences.value.safe_area==0?3:preferences.value.safe_area==3?5:0;
 else if(c.id=="artwork")preferences.value.artwork=!preferences.value.artwork;
 else if(c.id=="clock")preferences.value.clock=!preferences.value.clock;
 else if(c.id=="direct_play")preferences.value.direct_play=!preferences.value.direct_play;
 else if(c.id=="audio"||c.id=="subtitle"){
  if(c.id=="audio")audio_preference_dirty=true;else subtitle_preference_dirty=true;
  const std::vector<std::string> languages={"","eng","spa","fra","deu","jpn"};auto &selected=c.id=="audio"?audio_language:subtitle_language;auto at=std::find(languages.begin(),languages.end(),selected);selected=languages[(at==languages.end()?0:size_t(at-languages.begin()+1))%languages.size()];if(c.id=="subtitle")prefer_subtitles=!selected.empty();save_preferences();return;
 }else return;
 if(!preferences.save())model.message="Settings could not be saved safely; the previous file was preserved.";
}
__attribute__((noinline)) void Session::report_audio(const std::string &state,int64_t position){
 std::swap(model.detail,playing_audio);remember_position(position);report_progress(state,position);std::swap(model.detail,playing_audio);
}
__attribute__((noinline)) bool Session::stop_audio(int64_t captured){
 if(!audio_active)return true;auto observation=player.observe();auto position=captured>=0?captured:observation.failed?playing_audio.offset:observation.ended&&playing_audio.duration>0?playing_audio.duration:player.position();position=std::clamp<int64_t>(position,0,playing_audio.duration>0?playing_audio.duration:INT64_MAX);
 bool observed=audio_started||!observation.known||(!observation.failed&&(observation.playing||observation.paused));player.stop();if(observed){playing_audio.offset=position;report_audio("stopped",position);}
 if(!close_stream()){model.message="Music session cleanup failed. Retry Stop before opening another item.";return false;}
 audio_active=false;audio_started=false;return true;
}
__attribute__((noinline)) bool Session::open_audio(){
 const auto *queued=audio_queue.current();if(!queued)return false;auto id=queued->id;
 auto response=client.bounded(800,2000).browse(base,token,"/library/metadata/"+plex::encode(id),0,1);auto entries=response?plex::items(response.data):std::vector<plex::Item>{};
 if(entries.size()!=1||entries[0].id!=id||entries[0].type!="track"){model.message="The selected music track is unavailable.";return false;}
 model.detail=std::move(entries[0]);if(model.detail.key.empty())model.detail.key="/library/metadata/"+id;if(audio_resume>=0){model.detail.offset=audio_resume;audio_resume=-1;}model.media_version=0;load_tracks();
 if(!open_playback())return false;playing_audio=model.detail;audio_active=true;audio_started=false;return true;
}
__attribute__((noinline)) void Session::music_command(const Command &c){
 if(c.action==Action::MusicHome){home();return;}
 if(c.action==Action::MusicOpen){if(audio_active){model.detail=playing_audio;model.screen=Screen::Playback;}return;}
 if(c.action==Action::MusicShuffle){if(audio_queue.shuffled)audio_queue.unshuffle();else audio_queue.shuffle(uint32_t(++playback_serial));return;}
 if(c.action==Action::MusicRepeat){audio_queue.repeat=static_cast<plex::Repeat>((int(audio_queue.repeat)+1)%3);return;}
 if(c.action==Action::QueuePlay){
  auto item=model.detail;if(item.type!="album"&&item.type!="playlist"&&item.type!="track"){model.message="Choose an album, audio playlist or track.";return;}
  std::vector<plex::Item> tracks;
  if(item.type=="track"){item.media={};tracks.push_back(item);}else{auto bounded=client.bounded(800,1500);auto result=plex::audio_listing(bounded,base,token,item.type=="playlist"?"/playlists/"+plex::encode(item.id)+"/items":"/library/metadata/"+plex::encode(item.id)+"/children",[]{return true;},true);if(!result){model.message="The complete music queue could not be loaded.";return;}tracks=plex::audio_tracks(result.data,item.type=="album");}
  if(tracks.empty()){model.message="No playable tracks were returned by Plex.";return;}
  if(audio_active&&!stop_audio())return;
  if((model.screen==Screen::Playback||model.screen==Screen::Tracks)&&model.detail.type!="track"){stop_command({Action::Stop,{},{}});if(!stream.session.empty())return;}
  audio_queue.assign(std::move(tracks));if(!model.books)audio_resume=0;open_audio();return;
 }
 if(!audio_active&&c.action!=Action::MusicNext&&c.action!=Action::MusicPrevious)return;
 if(c.action==Action::MusicPrevious&&player.position()>3000){player.seek(0);playing_audio.offset=0;report_audio(model.paused?"paused":"playing",0);return;}
 bool ended=c.text=="ended";if(!stop_audio())return;
 auto old=audio_queue.cursor;bool next=c.action==Action::MusicPrevious?(audio_queue.cursor==0?true:audio_queue.previous()):audio_queue.advance(ended);
 if(!model.books||audio_queue.repeat==plex::Repeat::One)audio_resume=0;
 if(!next){model.screen=Screen::Details;model.detail=playing_audio;return;}
 if(!open_audio())audio_queue.cursor=old;
}
__attribute__((noinline)) bool Session::music_tick(){
 if(!audio_active)return false;auto observation=player.observe();
 if(observation.failed){model.playback_state=PlaybackState::Error;model.message="Decoder failure: music stopped. Retry this track or choose another.";return true;}
 playing_audio.offset=player.position();model.paused=player.paused();
 if(observation.ended){if(playing_audio.duration>0)playing_audio.offset=playing_audio.duration;report_audio("stopped",playing_audio.offset);music_command({Action::MusicNext,{},"ended"});return true;}
 if(observation.playing||observation.paused){audio_started=true;model.playback_state=observation.paused?PlaybackState::Paused:PlaybackState::Playing;report_audio(observation.paused?"paused":"playing",playing_audio.offset);}else model.playback_state=PlaybackState::Buffering;
 if(model.screen==Screen::Playback||(model.screen==Screen::Tracks&&model.tracks_during_playback)){model.detail=playing_audio;apply_tracks();}
 return true;
}
__attribute__((noinline)) bool Session::feature_command(const Command &c){
 switch(c.action){
 case Action::Notices:case Action::OpenNotice:notice_command(c);return true;
 case Action::Search:case Action::Suggest:search_command(c);return true;
 case Action::Setting:case Action::Diagnose:settings_command(c);return true;
 case Action::QueuePlay:case Action::MusicNext:case Action::MusicPrevious:case Action::MusicShuffle:case Action::MusicRepeat:case Action::MusicHome:case Action::MusicOpen:music_command(c);return true;
 case Action::Tick:if(music_tick())return true;return false;
 case Action::Play:if(model.detail.type=="track"){music_command({Action::QueuePlay,{},{}});return true;}if(audio_active){if(!stop_audio())return true;audio_queue.clear();}return false;
 case Action::Stop:if(audio_active){if(c.id=="ended")music_command({Action::MusicNext,{},"ended"});else {stop_audio(c.text.empty()?-1:plex::number(plex::Json{{"position",c.text}},"position",-1));model.detail=playing_audio;model.screen=Screen::Details;}return true;}return false;
 case Action::Back:if(audio_active&&model.screen==Screen::Playback){home();return true;}return false;
 case Action::Restart:
  if(model.detail.type=="track"){if(audio_active&&!stop_audio())return true;model.detail.offset=0;audio_resume=0;music_command({Action::QueuePlay,{},{}});return true;}
  if(model.screen==Screen::Playback){stop_command({Action::Stop,{},{}});if(!stream.session.empty())return true;}
  model.detail.offset=0;open_playback();return true;
 case Action::RetryPlayback:
  if(audio_active){auto position=playing_audio.offset;if(stop_audio()){model.detail=playing_audio;model.detail.offset=position;audio_resume=position;open_audio();}return true;}
  {if(model.detail.type!="movie"&&model.detail.type!="episode"){model.message="Choose this stream again from its library or guide.";return true;}
   auto id=model.detail.id;auto observed=player.observe();auto position=observed.failed?model.detail.offset:std::max<int64_t>(0,player.position());stop_command({Action::Stop,{},std::to_string(position)});if(!stream.session.empty())return true;
   auto response=client.bounded(800,2000).browse(base,token,"/library/metadata/"+plex::encode(id)+"?includeMarkers=1&includeChapters=1",0,1);auto values=response?plex::items(response.data):std::vector<plex::Item>{};
   if(values.size()!=1||values[0].id!=id){model.playback_state=PlaybackState::Error;model.message="Plex metadata is unavailable for retry. Check your connection and retry.";return true;}
   model.detail=std::move(values[0]);model.detail.offset=position;model.media_version=std::min(model.media_version,model.detail.media.empty()?size_t(0):model.detail.media.size()-1);load_tracks();open_playback(true);return true;}
 case Action::FastSeek:case Action::Chapter:{
  if(model.screen!=Screen::Playback||model.detail.type=="channel")return true;auto at=std::max<int64_t>(0,player.position());int64_t target=at;
  if(c.action==Action::FastSeek)target=seek_position(at,c.id=="back"?-60000:60000,model.detail.duration);
  else if(c.id=="previous"){for(const auto &chapter:model.detail.chapters)if(at>1500&&chapter.begins<at-1500)target=chapter.begins;}
  else {for(const auto &chapter:model.detail.chapters)if(chapter.begins>at&&chapter.begins-at>1000){target=chapter.begins;break;}}
  target=std::clamp<int64_t>(target,0,model.detail.duration>0?model.detail.duration:INT64_MAX);player.seek(target);model.detail.offset=target;
  if(audio_active){playing_audio.offset=target;report_audio(model.paused?"paused":"playing",target);}else if(playback_started||!player.observe().known)report_progress(model.paused?"paused":"playing",target);return true;}
 case Action::PlaybackInfo:model.playback_info=!model.playback_info;return true;
 case Action::MarkWatched:{
  if(model.screen!=Screen::Details||(model.detail.type!="movie"&&model.detail.type!="episode"))return true;
  bool watched=!model.detail.watched;auto result=client.bounded(500,1000).watched(base,token,model.detail,watched);
  if(!result){model.message="Plex could not update watched state. Retry when connected.";return true;}
  for(auto it=outbox.entries.begin();it!=outbox.entries.end();)if(it->item.id==model.detail.id)it=outbox.entries.erase(it);else ++it;persist_progress();
  model.detail.watched=watched;if(!watched)model.detail.offset=0;cached_home.clear();pages.clear();model.message=watched?"Marked watched on Plex.":"Marked unwatched on Plex.";return true;}
 case Action::EpisodePrevious:case Action::EpisodeNext:{
  if(model.detail.type!="episode")return true;auto bounded=client.bounded(700,2000);auto result=c.action==Action::EpisodePrevious?plex::previous_episode(bounded,base,token,model.detail):plex::next_episode(bounded,base,token,model.detail);
  if(!result){model.message="No adjacent episode is available in Plex order.";return true;}auto values=plex::items(result.data);if(values.size()!=1)return true;
  if(model.screen==Screen::Playback){stop_command({Action::Stop,{},{}});if(!stream.session.empty())return true;}
  model.upcoming=std::move(values);
  if(c.action==Action::EpisodePrevious){auto id=model.upcoming[0].id;auto response=bounded.browse(base,token,"/library/metadata/"+plex::encode(id)+"?includeMarkers=1&includeChapters=1",0,1);auto entries=response?plex::items(response.data):std::vector<plex::Item>{};if(entries.size()==1&&entries[0].id==id){model.detail=std::move(entries[0]);if(model.detail.key.empty())model.detail.key="/library/metadata/"+id;if(audio_resume>=0){model.detail.offset=audio_resume;audio_resume=-1;}model.media_version=0;load_tracks();open_playback();}}else play_next();return true;}
 default:return false;
 }
}
}
