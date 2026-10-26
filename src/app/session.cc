// SPDX-License-Identifier: GPL-3.0-or-later
#include "session.h"
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
 out.push_back({"profiles","Switch profile"});out.push_back({"servers","Change server"});out.push_back({"settings","Settings"});return out;
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
 player.stop();active.clear();base.clear();token.clear();server_id.clear();resources.clear();history.clear();
 model.sections.clear();model.entries.clear();model.rails.clear();model.navigation.clear();model.detail={};model.server_name.clear();model.servers.clear();
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
 base.clear();token.clear();server_id.clear();history.clear();model.sections.clear();model.rails.clear();model.entries.clear();model.navigation.clear();model.detail={};model.screen=Screen::Servers;
 if(found->token.empty())found->token=active;
 std::string selected;
 // Secure default: never silently downgrade to HTTP, even for local discovery.
 auto r=client.select(*found,false,selected);if(!r){fail("Cannot verify server identity and authorized libraries",r.status);return false;}
 base=selected;token=found->token.empty()?active:found->token;server_id=id;
 model.server_name=found->name;model.sections=plex::items(r.data,false);
 model.navigation=navigation(model.sections);
 if(!current_profile.empty())plex::save_credentials(root+"/server-"+current_profile+".json",current_profile,server_id);return home();
}
bool Session::home(){
 model.screen=Screen::Home;model.entries.clear();model.rails.clear();
 if(base.empty()){fail("Select a server first");return false;}
 auto bounded=client.bounded(2000,4000);auto deadline=now()+8;plex::Json partial;
 auto r=plex::home_feed(bounded,base,token,model.sections,[&]{return now()<deadline;},[&](const plex::Json&data){partial=data;});
 if(!r){model.rails=plex::home_rows(partial);fail("Home loading reached its bounded request budget; reopen Home to retry",r.status);return !model.rails.empty();}
 model.rails=plex::home_rows(r.data);return true;
}
bool Session::listing(const std::string &path){
 auto r=client.browse(base,token,path,0,60);if(!r){fail("Media request failed",r.status);return false;}
 model.entries=plex::items(r.data);return true;
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
  current_profile=c.id;model.profile_name=p->name;discover();break;}
 case Action::Server:if(c.id.empty()){model.screen=Screen::Servers;}else connect(c.id);break;
 case Action::Home:if(c.id=="profiles"){clear_context();profiles();}else home();break;
 case Action::Library:{
  if(std::none_of(model.sections.begin(),model.sections.end(),[&](const auto&s){return s.id==c.id;})){fail("Library is not authorized");break;}
  if(history.size()>=16)history.erase(history.begin());history.push_back(model);model.screen=Screen::Library;listing("/library/sections/"+plex::encode(c.id)+"/all");break;}
 case Action::Search:
  model.screen=Screen::Search;model.query=c.text;if(!c.text.empty()){listing("/search?query="+plex::encode(c.text));model.screen=Screen::Library;}break;
 case Action::Details:
  if(c.id.empty()||c.id.find_first_not_of("0123456789")!=std::string::npos){fail("Invalid media identity");break;}
  if(history.size()>=16)history.erase(history.begin());history.push_back(model);
  if(listing("/library/metadata/"+c.id)&&!model.entries.empty()){model.detail=model.entries.front();model.entries.clear();model.screen=Screen::Details;}break;
 case Action::Children:
  if(c.id.empty()||c.id.find_first_not_of("0123456789")!=std::string::npos){fail("Invalid parent identity");break;}
  if(history.size()>=16)history.erase(history.begin());history.push_back(model);model.screen=Screen::Library;listing("/library/metadata/"+c.id+"/children");break;
 case Action::Play:{
  if(model.detail.type!="movie"&&model.detail.type!="episode"&&model.detail.type!="track"){fail("Select a playable item");break;}
  auto decision=plex::direct_play(base,token,model.detail);
  if(decision.mode!=plex::Mode::DirectPlay){fail("This version requires an unsupported playback path");break;}
  if(!player.open(decision.url,model.detail.offset)){fail("Native playback did not open");break;}
  model.screen=Screen::Playback;break;}
 case Action::Stop:{auto position=player.position();if(!c.text.empty())position=std::clamp<int64_t>(plex::number(plex::Json{{"at",c.text}},"at",position),0,model.detail.duration>0?model.detail.duration:INT64_MAX);player.stop();client.bounded(500,1000).timeline(base,token,model.detail,"stopped",position);model.detail.offset=position;model.screen=Screen::Details;break;}
 case Action::Pause:player.pause();client.bounded(500,1000).timeline(base,token,model.detail,player.paused()?"paused":"playing",player.position());break;
 case Action::Seek:player.seek(std::max<int64_t>(0,player.position()+(c.id=="back"?-10000:10000)));break;
 case Action::Audio:player.audio_next();break;
 case Action::Subtitle:player.subtitle_next();break;
 case Action::Tick:if(model.screen==Screen::Playback){model.detail.offset=player.position();client.bounded(500,1000).timeline(base,token,model.detail,player.paused()?"paused":"playing",model.detail.offset);}break;
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
 model.busy=true;queue.push_back({generation,std::move(command)});cv.notify_one();return true;
}
Model Runtime::snapshot()const{std::lock_guard<std::mutex> l(mutex);return model;}
MediaContext Runtime::media_context()const{std::lock_guard<std::mutex> l(mutex);return context;}
void Runtime::run(){
 for(;;){std::pair<uint64_t,Command> next;
  {std::unique_lock<std::mutex> l(mutex);cv.wait(l,[&]{return closing||!queue.empty();});if(closing)return;next=std::move(queue.front());queue.pop_front();}
  auto updated=session->dispatch(next.second);
  {std::lock_guard<std::mutex> l(mutex);if(next.first==generation){model=std::move(updated);context=session->media_context();model.busy=!queue.empty();}}
 }
}
}
