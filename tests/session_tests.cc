// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/session.h"
#include "plex/free_tv.h"
#include "plex/presentation.h"
#include <cassert>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>
using namespace plexium;
struct Player:PlaybackPort {std::string last_url;int opened=0,stopped=0;bool is_paused=false;void pause()override{is_paused=!is_paused;}bool paused()const override{return is_paused;}int64_t position()const override{return 12345;}bool open(const std::string&url,int64_t)override{last_url=url;++opened;return true;}void stop()override{++stopped;}};
int main(){
 static_assert(std::string_view(title_id)!="PPSA85301");
 assert(std::string(data_path)!="/data/plexstation-client");
 auto nav=navigation({});assert(std::any_of(nav.begin(),nav.end(),[](const auto &i){return i.id=="free-tv";}));assert(nav.front().id=="home");assert(nav[1].id=="search");
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
  if(q.url.find("epg.provider.plex.tv")!=std::string::npos){assert(q.headers.at("X-Plex-Token")=="profile-token");return reply(plex::Json::parse(R"({"MediaContainer":{"Channel":[{"id":"abc123","title":"Fixture channel","Media":[{"Part":[{"key":"/library/parts/channel123"}]}]}]}})"));}
  if(q.url.find("/library/sections/1/all?sort=titleSort")!=std::string::npos){
   int start=q.url.find("X-Plex-Container-Start=60")!=std::string::npos?60:0;plex::Json entries=plex::Json::array();
   for(int i=0;i<60;++i)entries.push_back({{"ratingKey",std::to_string(start+i+1)},{"title","Paged movie"},{"type","movie"}});
   return reply({{"MediaContainer",{{"Metadata",entries},{"totalSize",3700}}}});
  }
  if(q.url.find("/hubs/sections/1")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Hub":[{"title":"Recommended Movies","Metadata":[{"ratingKey":"1","title":"Film","type":"movie","librarySectionID":"1"}]}]}})"));
  if(q.url.find("/library/metadata/1/related")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Hub":[{"title":"Related Movies","Metadata":[{"ratingKey":"2","title":"Another film","type":"movie"}]}]}})"));
  if(q.url.find("/library/metadata/1")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Metadata":[{"ratingKey":"1","title":"Film","type":"movie","duration":90000,"viewOffset":12000,"Role":[{"tag":"Actor One","role":"Lead","thumb":"/actor/1"},{"tag":"Actor Two","role":"Supporting","thumb":"/actor/2"}],"Media":[{"videoCodec":"h264","container":"mp4","Part":[{"key":"/library/parts/1/file.mp4"}]}]}]}})"));
  if(q.url.find("/library/sections/1")!=std::string::npos||q.url.find("/hubs/continueWatching")!=std::string::npos)return reply({{"MediaContainer",{{"Metadata",plex::Json::array({{{"ratingKey","1"},{"title","Film"},{"type","movie"},{"librarySectionID","1"},{"originallyAvailableAt","2026-10-01"},{"duration",90000},{"viewOffset",12000}}})}}}});
  return reply({{"MediaContainer",{{"Metadata",plex::Json::array()}}}});
 };
 assert(plex::save_credentials(root+"/account.json","test-device","account-token"));
 Session session(transport,root,player);
 std::vector<std::pair<int,size_t>> partials;session.progress=[&](const Model& value){if(value.screen==Screen::Home&&!value.rails.empty())partials.push_back({requests,value.rails.size()});};
 auto m=session.dispatch({Action::Restore,{},{}});assert(m.screen==Screen::Profiles&&m.profiles.size()==2);
 m=session.dispatch({Action::Profile,"1",{}});assert(m.screen==Screen::Home);assert(m.sections.size()==1);assert(m.navigation[2].id=="library:1");
 assert(!m.rails.empty());assert(m.rails.front().id=="continueWatching");assert(!partials.empty()&&partials.front().first<requests);assert(session.media_context().account_token=="account-token");
 std::set<std::string> seen;size_t last=0;for(auto&r:m.rails){assert(seen.insert(r.id).second);auto order=plex::home_order();size_t at=std::find(order.begin(),order.end(),r.id)-order.begin();assert(at>=last);last=at;}
 int before=requests;m=session.dispatch({Action::Home,{},{}});assert(requests==before);
 m=session.dispatch({Action::Library,"1",{}});assert(m.screen==Screen::Library&&m.library_tab=="recommended"&&!m.rails.empty());
 before=requests;m=session.dispatch({Action::Library,"1",{}});assert(requests==before);
 m=session.dispatch({Action::Library,"1","all"});assert(m.entries.size()==60&&m.total==3700&&m.page_start==0);
 m=session.dispatch({Action::Page,"next",{}});assert(m.entries.front().id=="61"&&m.page_start==60);
 m=session.dispatch({Action::Page,"previous",{}});assert(m.entries.front().id=="1"&&m.page_start==0);
 m=session.dispatch({Action::Browse,"1","/library/sections/999/all"});assert(!m.message.empty());
 m=session.dispatch({Action::Library,"999",{}});assert(!m.message.empty());
 m=session.dispatch({Action::Details,"1",{}});assert(m.screen==Screen::Details&&m.detail.id=="1");
 assert(m.detail.cast.size()==2&&m.detail.credits.size()==2&&m.detail.credits[0].role=="Lead");assert(!m.detail_rails.empty());
 before=requests;m=session.dispatch({Action::Details,"1",{}});assert(requests==before);
 m=session.dispatch({Action::Play,{},{}});assert(m.screen==Screen::Playback&&player.opened==1);
 m=session.dispatch({Action::Pause,{},{}});assert(player.is_paused&&last_timeline.find("state=paused")!=std::string::npos);
 m=session.dispatch({Action::Stop,{},"45000"});assert(m.screen==Screen::Details&&m.detail.offset==45000&&last_timeline.find("time=45000")!=std::string::npos);
 before=requests;m=session.dispatch({Action::Home,{},{}});assert(requests==before&&m.rails.front().entries.front().offset==45000);
 m=session.dispatch({Action::FreeTV,{},{}});assert(m.entries.size()==1&&m.entries.front().type=="channel");
 assert(session.media_context().base==plex::free_tv_origin&&session.media_context().token=="profile-token");
 m=session.dispatch({Action::PlayLive,"abc123",{}});assert(m.screen==Screen::Playback&&player.last_url.rfind("https://epg.provider.plex.tv/library/parts/channel123?",0)==0);
 before=requests;m=session.dispatch({Action::Tick,{},{}});m=session.dispatch({Action::Stop,{},{}});assert(requests==before&&m.screen==Screen::Library);
 auto protected_data=plex::Json::parse(R"({"MediaContainer":{"Channel":[{"id":"a","Media":[{"drm":"widevine","Part":[{"key":"/library/parts/1"}]}]},{"id":"b","Media":[{"Part":[{"key":"https://foreign.test/stream"}]}]}]}})");
 assert(plex::free_channels(protected_data).empty());
 plex::Item track;track.type="track";track.parent_thumb="/album-cover";assert(plex::image_candidates(track).front()=="/album-cover");
 assert(plex::image_candidates(episode).front()=="/show-poster");
 m=session.dispatch({Action::Profile,"2",{}});assert(m.screen==Screen::Pin);
 switch_failure=true;m=session.dispatch({Action::Profile,"2","1234"});assert(m.screen==Screen::Profiles);assert(m.sections.empty()&&m.rails.empty()&&m.entries.empty()&&m.navigation.empty());
 m=session.dispatch({Action::SignOut,{},{}});assert(m.screen==Screen::Welcome);assert(!std::filesystem::exists(root+"/account.json"));
 struct stat st{};assert(stat((root+"/device.json").c_str(),&st)==0);assert((st.st_mode&0777)==0600);
 std::filesystem::remove_all(root);
 std::cout<<"PASS: identity, navigation, episode poster, secure persistence, restore/profile/PIN, server discovery, authorized Home, details, direct-play dispatch, context isolation, sign-out ("<<requests<<" fixture requests)\n";
}
