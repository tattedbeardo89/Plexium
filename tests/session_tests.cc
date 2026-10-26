// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/session.h"
#include <cassert>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>
using namespace plexium;
struct Player:PlaybackPort {int opened=0,stopped=0;bool is_paused=false;void pause()override{is_paused=!is_paused;}bool paused()const override{return is_paused;}int64_t position()const override{return 12345;}bool open(const std::string&,int64_t)override{++opened;return true;}void stop()override{++stopped;}};
int main(){
 static_assert(std::string_view(title_id)!="PPSA85301");
 assert(std::string(data_path)!="/data/plexstation-client");
 auto nav=navigation({});assert(nav.front().id=="home");assert(nav[1].id=="search");
 for(auto&d:nav)assert(d.id!="rentals"&&d.id!="live"&&d.id!="onDeck");
 plex::Item episode;episode.type="episode";episode.id="44";episode.thumb="/episode-still";episode.grandparent_thumb="/show-poster";
 assert(poster(episode)=="/show-poster");assert(episode.id=="44");
 plex::Item movie;movie.type="movie";movie.id="1";movie.title="Film";movie.released="2026-10-01";movie.section_id="1";movie.offset=12000;movie.duration=90000;
 std::string root="/tmp/plexium-test-"+std::to_string(getpid());std::filesystem::create_directory(root);chmod(root.c_str(),0700);
 Player player;int requests=0;std::string last_timeline;bool switch_failure=false;
 auto transport=[&](const plex::Request&q){
  ++requests;assert(q.headers.at("X-Plex-Product")=="Plexium");
  auto reply=[](const plex::Json&j){return plex::Reply{200,j.dump(),""};};
  if(q.url.find("/:/timeline")!=std::string::npos){last_timeline=q.url;return reply({});}
  if(q.url.find("/api/v2/pins")!=std::string::npos)return reply({{"id",1},{"code","ABCD"},{"expiresIn",100}});
  if(q.url.find("/api/home/users/")!=std::string::npos){if(switch_failure)return plex::Reply{403,"{}",""};return reply({{"id",1},{"authenticationToken","profile-token"}});}
  if(q.url.find("/api/home/users")!=std::string::npos||q.url.find("/api/v2/home/users")!=std::string::npos)return reply({{"users",plex::Json::array({{{"id",1},{"title","Owner"},{"protected",false}},{{"id",2},{"title","Child"},{"protected",true}}})}});
  if(q.url.find("/api/v2/user")!=std::string::npos)return reply({{"id",7}});
  if(q.url.find("/api/v2/resources")!=std::string::npos)return reply(plex::Json::array({{{"name","Home server"},{"clientIdentifier","server1"},{"provides","server"},{"accessToken","server-token"},{"connections",plex::Json::array({{{"uri","https://server.test:32400"},{"local",true}}})}}}));
  if(q.url.find("/identity")!=std::string::npos)return reply({{"MediaContainer",{{"machineIdentifier","server1"}}}});
  if(q.url=="https://server.test:32400/library/sections")return reply({{"MediaContainer",{{"Directory",plex::Json::array({{{"key","1"},{"title","Movies"},{"type","movie"}}})}}}});
  if(q.url.find("/library/metadata/1")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Metadata":[{"ratingKey":"1","title":"Film","type":"movie","duration":90000,"viewOffset":12000,"Media":[{"videoCodec":"h264","container":"mp4","Part":[{"key":"/library/parts/1/file.mp4"}]}]}]}})"));
  if(q.url.find("/library/sections/1")!=std::string::npos||q.url.find("/hubs/continueWatching")!=std::string::npos)return reply({{"MediaContainer",{{"Metadata",plex::Json::array({{{"ratingKey","1"},{"title","Film"},{"type","movie"},{"librarySectionID","1"},{"originallyAvailableAt","2026-10-01"},{"duration",90000},{"viewOffset",12000}}})}}}});
  return reply({{"MediaContainer",{{"Metadata",plex::Json::array()}}}});
 };
 assert(plex::save_credentials(root+"/account.json","test-device","account-token"));
 Session session(transport,root,player);
 auto m=session.dispatch({Action::Restore,{},{}});assert(m.screen==Screen::Profiles&&m.profiles.size()==2);
 m=session.dispatch({Action::Profile,"1",{}});assert(m.screen==Screen::Home);assert(m.sections.size()==1);assert(m.navigation[2].id=="library:1");
 assert(!m.rails.empty());assert(m.rails.front().id=="continueWatching");
 std::set<std::string> seen;size_t last=0;for(auto&r:m.rails){assert(seen.insert(r.id).second);auto order=plex::home_order();size_t at=std::find(order.begin(),order.end(),r.id)-order.begin();assert(at>=last);last=at;}
 m=session.dispatch({Action::Library,"999",{}});assert(!m.message.empty());
 m=session.dispatch({Action::Details,"1",{}});assert(m.screen==Screen::Details&&m.detail.id=="1");
 m=session.dispatch({Action::Play,{},{}});assert(m.screen==Screen::Playback&&player.opened==1);
 m=session.dispatch({Action::Pause,{},{}});assert(player.is_paused&&last_timeline.find("state=paused")!=std::string::npos);
 m=session.dispatch({Action::Stop,{},"45000"});assert(m.screen==Screen::Details&&m.detail.offset==45000&&last_timeline.find("time=45000")!=std::string::npos);
 m=session.dispatch({Action::Profile,"2",{}});assert(m.screen==Screen::Pin);
 switch_failure=true;m=session.dispatch({Action::Profile,"2","1234"});assert(m.screen==Screen::Profiles);assert(m.sections.empty()&&m.rails.empty()&&m.entries.empty()&&m.navigation.empty());
 m=session.dispatch({Action::SignOut,{},{}});assert(m.screen==Screen::Welcome);assert(!std::filesystem::exists(root+"/account.json"));
 struct stat st{};assert(stat((root+"/device.json").c_str(),&st)==0);assert((st.st_mode&0777)==0600);
 std::filesystem::remove_all(root);
 std::cout<<"PASS: identity, navigation, episode poster, secure persistence, restore/profile/PIN, server discovery, authorized Home, details, direct-play dispatch, context isolation, sign-out ("<<requests<<" fixture requests)\n";
}
