// SPDX-License-Identifier: GPL-3.0-or-later
#include "session.h"
#include "browse.h"
#include "plex/navigation.h"
#include "plex/audio.h"
#include "plex/free_tv.h"
#include <chrono>
#include <fstream>
#include <random>
#include <sys/stat.h>
#include <unistd.h>
namespace plexium {
std::vector<Destination> navigation(const std::vector<plex::Item>& sections) {
 std::vector<Destination> out={{"home","Home"},{"search","Search"}};
 std::set<std::string> seen;
 for(const auto &s:sections) if(!s.id.empty() && seen.insert(s.id).second)out.push_back({"library:"+s.id,s.title});
 out.push_back({"free-tv","Free Live TV (Beta)"});out.push_back({"profiles","Switch profile"});out.push_back({"servers","Change server"});out.push_back({"settings","Settings"});return out;
}
std::string poster(const plex::Item &item) {
 if(item.type=="episode")return item.grandparent_thumb;
 return item.thumb;
}
double Session::now()const{return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
Session::Session(plex::Transport t,const std::string &path,PlaybackPort &p):client(std::move(t),"pending"),root(path),player(p){}
void Session::fail(const char *stage,int status){model.message=std::string(stage)+(status?" (HTTP "+std::to_string(status)+")":"")+". Retry or return with Circle.";}
bool Session::private_root(){
 struct stat s{};
 if(mkdir(root.c_str(),0700)!=0 && errno!=EEXIST)return false;
 if(lstat(root.c_str(),&s)!=0 || !S_ISDIR(s.st_mode) || s.st_uid!=geteuid() || (s.st_mode&077))return false;
 return true;
}
void Session::clear_context(){
 player.stop();active.clear();base.clear();token.clear();server_id.clear();resources.clear();history.clear();cached_home.clear();pages.clear();metadata.clear();channels.clear();live_parts.clear();channels_loaded=0;home_loaded=0;
 model.sections.clear();model.entries.clear();model.rails.clear();model.navigation.clear();model.detail={};model.detail_rails.clear();model.library_id.clear();model.library_path.clear();model.server_name.clear();model.servers.clear();
}
bool Session::profiles(){
 auto r=client.home_users(account);if(!r){fail("Unable to load Plex Home profiles",r.status);return false;}
 model.profiles=plex::profiles(r.data);model.screen=Screen::Profiles;
 if(model.profiles.empty()){fail("Plex returned no selectable Home profiles");return false;}return true;
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
 base.clear();token.clear();server_id.clear();history.clear();cached_home.clear();pages.clear();metadata.clear();channels.clear();live_parts.clear();channels_loaded=0;home_loaded=0;model.sections.clear();model.rails.clear();model.entries.clear();model.navigation.clear();model.detail={};model.screen=Screen::Servers;
 if(found->token.empty())found->token=active;
 std::string selected;
 // Secure default: never silently downgrade to HTTP, even for local discovery.
 std::string route_id,preferred;
 if(!current_profile.empty()&&plex::load_credentials(root+"/route-"+current_profile+".json",route_id,preferred)&&route_id!=id)preferred.clear();
 auto r=client.bounded(1200,3500).select(*found,false,selected,nullptr,preferred);if(!r){fail("Cannot verify server identity and authorized libraries",r.status);return false;}
 base=selected;token=found->token.empty()?active:found->token;server_id=id;
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
Model Session::dispatch(const Command &c){
 model.message.clear();
 switch(c.action){
 case Action::Restore:{
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
   auto r=client.validate_account(account);if(r)profiles();else{account.clear();fail("Saved account needs sign-in",r.status);}
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
   if(!plex::save_credentials(root+"/account.json",device,account)){account.clear();fail("Cannot persist account securely");break;}profiles();
  }else if(auth.state==plex::AuthState::Failed||auth.state==plex::AuthState::Expired){model.code.clear();fail("Activation expired or failed; press Cross to retry");}
  break;
 case Action::Cancel:auth.cancel();model.code.clear();model.screen=Screen::Welcome;break;
 case Action::Profile:{
  auto p=std::find_if(model.profiles.begin(),model.profiles.end(),[&](const auto&p){return p.id==c.id;});
  if(p==model.profiles.end()){fail("Unknown profile");break;}
  if(p->protected_user&&!plex::valid_pin(c.text)){model.screen=Screen::Pin;break;}
  // Wipe all old authorized content BEFORE switching; failure cannot retain it.
  clear_context();auto r=client.switch_home(account,c.id,c.text);
  if(!r){model.screen=Screen::Profiles;fail("Profile authorization failed",r.status);break;}
  active=plex::text(r.data,"authenticationToken",plex::text(r.data,"authToken"));
  if(active.empty()){model.screen=Screen::Profiles;fail("Profile response has no authorization");break;}
  current_profile=c.id;model.profile_name=p->name;model.profile_avatar=p->avatar;discover();break;}
 case Action::Server:if(c.id.empty()){model.screen=Screen::Servers;}else connect(c.id);break;
 case Action::Home:if(c.id=="profiles"){clear_context();profiles();}else home(c.text=="refresh");break;
 case Action::Library:{
  if(!c.id.empty()&&c.id!=model.library_id){if(history.size()>=16)history.erase(history.begin());history.push_back(model);}
  library(c.id,c.text);break;}
 case Action::Browse:{
  const auto prefix="/library/sections/"+plex::encode(model.library_id)+"/";
  if(model.library_id.empty()||c.text.rfind(prefix,0)!=0||c.text.find("..")!=std::string::npos){fail("Invalid library category");break;}
  if(history.size()>=16)history.erase(history.begin());history.push_back(model);auto category=std::find_if(model.entries.begin(),model.entries.end(),[&](const auto &i){return i.key==c.text;});if(category!=model.entries.end())model.library_title+=" · "+category->title;model.library_tab="category";listing(c.text);break;}
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
  model.screen=Screen::Search;model.query=c.text;model.library_title="Search results";model.books=false;model.library_id.clear();model.library_tab.clear();model.rails.clear();if(!c.text.empty()){listing("/search?query="+plex::encode(c.text));model.screen=Screen::Library;}break;
 case Action::Details:
  if(c.id.empty()||c.id.find_first_not_of("0123456789")!=std::string::npos){fail("Invalid media identity");break;}
  if(history.size()>=16)history.erase(history.begin());history.push_back(model);
  {auto cached=pages.find("detail:"+c.id);if(cached!=pages.end()&&now()-cached->second.loaded<180){model=cached->second.value;break;}}
  if(listing("/library/metadata/"+c.id)&&!model.entries.empty()){
   model.detail=model.entries.front();model.entries.clear();model.detail_rails.clear();model.screen=Screen::Details;if(progress)progress(model);
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
   if(type=="show"||type=="season"||type=="artist"||type=="album"||type=="collection"){
    auto path=type=="collection"?"/library/collections/":"/library/metadata/";
    auto response=bounded.browse(base,token,path+c.id+"/children",0,12);
    if(response){auto children=plex::items(response.data,false);if(!children.empty())model.detail_rails.push_back({"children",type=="show"?"Seasons":type=="season"?"Episodes":type=="artist"?(model.books?"Books":"Albums"):type=="album"?(model.books?"Chapters":"Tracks"):"Collection",false,std::move(children)});}
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
  listing((model.detail.type=="collection"?"/library/collections/":"/library/metadata/")+c.id+"/children");break;
 case Action::Play:{
  if(model.detail.type!="movie"&&model.detail.type!="episode"&&model.detail.type!="track"){fail("Select a playable item");break;}
  auto decision=model.detail.type=="track"?plex::audio_direct_play(base,token,model.detail):plex::direct_play(base,token,model.detail);
  if(decision.mode!=plex::Mode::DirectPlay){fail("This version requires an unsupported playback path");break;}
  if(!player.open(decision.url,model.detail.offset)){fail("Native playback did not open");break;}
  model.paused=false;model.screen=Screen::Playback;break;}
 case Action::Stop:{pages.erase("detail:"+model.detail.id);auto position=player.position();if(!c.text.empty())position=std::clamp<int64_t>(plex::number(plex::Json{{"at",c.text}},"at",position),0,model.detail.duration>0?model.detail.duration:INT64_MAX);player.stop();if(model.detail.type!="channel")remember_position(position);if(model.detail.type!="channel")client.bounded(500,1000).timeline(base,token,model.detail,"stopped",position);model.detail.offset=position;model.screen=model.detail.type=="channel"?Screen::Library:Screen::Details;if(model.detail.type=="channel"&&c.id=="ended"){channels_loaded=0;model.message="The live stream ended or could not be decoded. Select the channel again to reconnect.";}break;}
 case Action::Pause:player.pause();model.paused=player.paused();if(model.detail.type!="channel")client.bounded(500,1000).timeline(base,token,model.detail,player.paused()?"paused":"playing",player.position());break;
 case Action::Seek:if(model.detail.type=="channel")break;player.seek(std::max<int64_t>(0,player.position()+(c.id=="back"?-10000:10000)));break;
 case Action::Audio:player.audio_next();break;
 case Action::Subtitle:player.subtitle_next();break;
 case Action::Tick:if(model.screen==Screen::Playback){model.detail.offset=player.position();model.paused=player.paused();if(model.detail.type!="channel")client.bounded(500,1000).timeline(base,token,model.detail,player.paused()?"paused":"playing",model.detail.offset);}break;
 case Action::Settings:if(history.size()>=16)history.erase(history.begin());history.push_back(model);model.screen=Screen::Settings;break;
 case Action::SignOut:
  clear_context();account.clear();auth.cancel();unlink((root+"/account.json").c_str());model=Model{};break;
 case Action::Back:
  if(!history.empty()){model=history.back();history.pop_back();}
  else if(model.screen==Screen::Pin)model.screen=Screen::Profiles;
  else if(model.screen==Screen::Linking){auth.cancel();model.code.clear();model.screen=Screen::Welcome;}
  else if(!base.empty())home();else model.screen=Screen::Welcome;
  break;
 }
 return model;
}
Runtime::Runtime(std::unique_ptr<Session>s):session(std::move(s)),worker(&Runtime::run,this){}
Runtime::~Runtime(){{std::lock_guard<std::mutex> l(mutex);closing=true;queue.clear();}cv.notify_all();worker.join();}
bool Runtime::send(Command command){
 std::lock_guard<std::mutex> l(mutex);
 if((command.action==Action::Back||command.action==Action::Stop)&&model.busy){++generation;queue.clear();}
 else if(command.action==Action::Cancel||command.action==Action::SignOut){++generation;queue.clear();model=Model{};context={};}
 else if(model.busy)return false;
 if(command.action!=Action::Tick)model.busy=true;queue.push_back({generation,std::move(command)});cv.notify_one();return true;
}
Model Runtime::snapshot()const{std::lock_guard<std::mutex> l(mutex);return model;}
MediaContext Runtime::media_context()const{std::lock_guard<std::mutex> l(mutex);return context;}
void Runtime::run(){
 for(;;){std::pair<uint64_t,Command> next;
  {std::unique_lock<std::mutex> l(mutex);cv.wait(l,[&]{return closing||!queue.empty();});if(closing)return;next=std::move(queue.front());queue.pop_front();}
  session->progress=[this,epoch=next.first](const Model &updated){
   std::lock_guard<std::mutex> l(mutex);if(epoch!=generation)return;
   model=updated;model.busy=!queue.empty()||updated.screen!=Screen::Details;context=session->media_context();
  };
  auto updated=session->dispatch(next.second);session->progress={};
  {std::lock_guard<std::mutex> l(mutex);if(next.first==generation){model=std::move(updated);context=session->media_context();model.busy=!queue.empty();}}
 }
}
}
