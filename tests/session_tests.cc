#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/session.h"
#include "plex/free_tv.h"
#include "plex/presentation.h"
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>
using namespace plexium;
struct Player:PlaybackPort {std::string last_url;int opened=0,stopped=0;bool is_paused=false;void pause()override{is_paused=!is_paused;}bool paused()const override{return is_paused;}int64_t position()const override{return 12345;}bool open(const std::string&url,int64_t)override{last_url=url;++opened;return true;}void stop()override{++stopped;}};
int main(){
 static_assert(std::string_view(title_id)!="PPSA85301");
 TEST_REQUIRE(std::string(data_path)!="/data/plexstation-client");
 auto nav=navigation({});TEST_REQUIRE(std::any_of(nav.begin(),nav.end(),[](const auto &i){return i.id=="free-tv";}));TEST_REQUIRE(nav.front().id=="home");TEST_REQUIRE(nav[1].id=="search");
 for(auto&d:nav)TEST_REQUIRE(d.id!="rentals"&&d.id!="live"&&d.id!="onDeck");
 plex::Item episode;episode.type="episode";episode.id="44";episode.thumb="/episode-still";episode.grandparent_thumb="/show-poster";
 TEST_REQUIRE(poster(episode)=="/show-poster");TEST_REQUIRE(episode.id=="44");
 plex::Item movie;movie.type="movie";movie.id="1";movie.title="Film";movie.released="2026-10-01";movie.section_id="1";movie.offset=12000;movie.duration=90000;
 std::string root="/tmp/plexium-test-"+std::to_string(getpid());std::filesystem::create_directory(root);test_posix_result(chmod(root.c_str(),0700),"chmod");
 Player player;int requests=0;std::string last_timeline;bool switch_failure=false;int dvr_mode=0;
 auto transport=[&](const plex::Request&q){
  ++requests;TEST_REQUIRE(q.headers.at("X-Plex-Product")=="Plexium");
  auto reply=[](const plex::Json&j){return plex::Reply{200,j.dump(),""};};
  if(q.url.find("/livetv/dvrs")!=std::string::npos){
   TEST_REQUIRE(q.headers.at("X-Plex-Token")=="server-token");
   if(dvr_mode==2)return plex::Reply{403,"{}",""};
   if(dvr_mode==3)return plex::Reply{503,"{}",""};
   if(dvr_mode==0)return reply({{"MediaContainer",{{"size",0}}}});
   return reply(plex::Json::parse(R"({"MediaContainer":{"Dvr":[{"key":"12"}]}})"));
  }
  if(q.url.find("/media/providers")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"MediaProvider":[{"identifier":"tv.plex.providers.epg.xmltv:12"}]}})"));
  if(q.url.find("/lineups/dvr/channels")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Channel":[{"id":"channel10","gridKey":"grid10","vcn":"10","title":"Fixture 10"},{"id":"channel2","gridKey":"grid2","vcn":"2","title":"Fixture 2"}]}})"));
  if(q.url.find("/grid?")!=std::string::npos)return reply({{"MediaContainer",{{"Metadata",plex::Json::array()}}}});
  if(q.url.find("/:/timeline")!=std::string::npos){last_timeline=q.url;return reply({});}
  if(q.url.find("/api/v2/pins")!=std::string::npos)return reply({{"id",1},{"code","ABCD"},{"expiresIn",100}});
  if(q.url.find("/api/home/users/")!=std::string::npos){if(switch_failure)return plex::Reply{403,"{}",""};return reply({{"id",1},{"authenticationToken","profile-token"}});}
  if(q.url.find("/api/home/users")!=std::string::npos||q.url.find("/api/v2/home/users")!=std::string::npos)return reply({{"users",plex::Json::array({{{"id",1},{"title","Owner"},{"protected",false}},{{"id",2},{"title","Child"},{"protected",true}}})}});
  if(q.url.find("/api/v2/user")!=std::string::npos)return reply({{"id",7}});
  if(q.url.find("/api/v2/resources")!=std::string::npos)return reply(plex::Json::array({{{"name","Home server"},{"clientIdentifier","server1"},{"provides","server"},{"accessToken","server-token"},{"connections",plex::Json::array({{{"uri","https://server.test:32400"},{"local",true}}})}}}));
  if(q.url.find("/identity")!=std::string::npos)return reply({{"MediaContainer",{{"machineIdentifier","server1"}}}});
  if(q.url=="https://server.test:32400/library/sections")return reply({{"MediaContainer",{{"Directory",plex::Json::array({{{"key","1"},{"title","Movies"},{"type","movie"}}})}}}});
  if(q.url.find("epg.provider.plex.tv")!=std::string::npos){TEST_REQUIRE(q.headers.at("X-Plex-Token")=="profile-token");return reply(plex::Json::parse(R"({"MediaContainer":{"Channel":[{"id":"abc123","title":"Fixture channel","Media":[{"Part":[{"key":"/library/parts/channel123"}]}]}]}})"));}
  if(q.url.find("/library/sections/1/all?sort=titleSort")!=std::string::npos){
   int start=q.url.find("X-Plex-Container-Start=60")!=std::string::npos?60:0;plex::Json entries=plex::Json::array();
   for(int i=0;i<60;++i)entries.push_back({{"ratingKey",std::to_string(start+i+1)},{"title","Paged movie"},{"type","movie"}});
   return reply({{"MediaContainer",{{"Metadata",entries},{"totalSize",3700}}}});
  }
  if(q.url.find("/hubs/sections/1")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Hub":[{"title":"Recommended Movies","Metadata":[{"ratingKey":"1","title":"Film","type":"movie","librarySectionID":"1"}]}]}})"));
  if(q.url.find("/library/collections/7/children")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Metadata":[{"ratingKey":"1","title":"Film","type":"movie"}]}})"));
  if(q.url.find("/library/collections/7")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Metadata":[{"ratingKey":"7","title":"Fixture collection","type":"collection"}]}})"));
  if(q.url.find("/playlists/8/items")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Metadata":[{"ratingKey":"9","title":"Fixture song","type":"track","parentThumb":"/album-cover"}]}})"));
  if(q.url.find("/playlists/8")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Playlist":[{"ratingKey":"8","title":"Fixture playlist","type":"playlist"}]}})"));
  if(q.url.find("/library/metadata/9")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Metadata":[{"ratingKey":"9","title":"Fixture song","type":"track","duration":90000,"parentTitle":"Fixture album","grandparentTitle":"Fixture artist","Media":[{"audioCodec":"mp3","container":"mp3","Part":[{"key":"/library/parts/9/file.mp3"}]}]}]}})"));
  if(q.url.find("/library/metadata/1/related")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Hub":[{"title":"Related Movies","Metadata":[{"ratingKey":"2","title":"Another film","type":"movie"}]}]}})"));
  if(q.url.find("/library/metadata/1")!=std::string::npos)return reply(plex::Json::parse(R"({"MediaContainer":{"Metadata":[{"ratingKey":"1","title":"Film","type":"movie","duration":90000,"viewOffset":12000,"Role":[{"tag":"Actor One","role":"Lead","thumb":"/actor/1"},{"tag":"Actor Two","role":"Supporting","thumb":"/actor/2"}],"Media":[{"videoCodec":"h264","container":"mp4","Part":[{"key":"/library/parts/1/file.mp4"}]}]}]}})"));
  if(q.url.find("/library/sections/1")!=std::string::npos||q.url.find("/hubs/continueWatching")!=std::string::npos)return reply({{"MediaContainer",{{"Metadata",plex::Json::array({{{"ratingKey","1"},{"title","Film"},{"type","movie"},{"librarySectionID","1"},{"originallyAvailableAt","2026-10-01"},{"duration",90000},{"viewOffset",12000}}})}}}});
  return reply({{"MediaContainer",{{"Metadata",plex::Json::array()}}}});
 };
 TEST_REQUIRE(plex::save_credentials(root+"/account.json","test-device","account-token"));
 Session session(transport,root,player);
 std::vector<std::pair<int,size_t>> partials;session.progress=[&](const Model& value){if(value.screen==Screen::Home&&!value.rails.empty())partials.push_back({requests,value.rails.size()});};
 auto m=session.dispatch({Action::Restore,{},{}});TEST_REQUIRE(m.screen==Screen::Profiles&&m.profiles.size()==2);
 m=session.dispatch({Action::Profile,"1",{}});TEST_REQUIRE(m.screen==Screen::Home);TEST_REQUIRE(m.sections.size()==1);TEST_REQUIRE(m.navigation[2].id=="live-tv"&&m.navigation[3].id=="library:1");
 TEST_REQUIRE(!m.rails.empty());TEST_REQUIRE(m.rails.front().id=="continueWatching");TEST_REQUIRE(!partials.empty()&&partials.front().first<requests);TEST_REQUIRE(session.media_context().account_token=="account-token");
 std::set<std::string> seen;size_t last=0;for(auto&r:m.rails){TEST_REQUIRE(seen.insert(r.id).second);auto order=plex::home_order();size_t at=std::find(order.begin(),order.end(),r.id)-order.begin();TEST_REQUIRE(at>=last);last=at;}
 int before=requests;
 m=session.dispatch({Action::LiveTV,{},{}});TEST_REQUIRE(m.screen==Screen::Guide&&!m.guide.configured&&m.message.find("Plex Web")!=std::string::npos&&requests==before+1);
 dvr_mode=1;m=session.dispatch({Action::LiveTV,{},"refresh"});TEST_REQUIRE(m.guide.configured&&m.guide.total==2&&m.guide.channels.front().number=="2");
 before=requests;auto opened=player.opened;m=session.dispatch({Action::TuneChannel,"channel2",{}});TEST_REQUIRE(requests==before&&player.opened==opened&&!m.message.empty());
 auto begins=m.guide.begins;m=session.dispatch({Action::GuidePage,"later",{}});TEST_REQUIRE(m.guide.begins==begins+3600);
 auto guide_count=m.guide.channels.size();auto guide_begin=m.guide.begins;
 dvr_mode=3;m=session.dispatch({Action::LiveTV,{},"refresh"});TEST_REQUIRE(m.guide.stale&&m.guide.channels.size()==guide_count&&m.guide.begins==guide_begin&&m.message.find("cached")!=std::string::npos);
 dvr_mode=2;m=session.dispatch({Action::LiveTV,{},"refresh"});TEST_REQUIRE(m.screen==Screen::Guide&&m.guide.channels.empty()&&m.message.find("403")!=std::string::npos);
 before=requests;m=session.dispatch({Action::Home,{},{}});TEST_REQUIRE(requests==before);
 m=session.dispatch({Action::Library,"1",{}});TEST_REQUIRE(m.screen==Screen::Library&&m.library_tab=="recommended"&&!m.rails.empty());
 before=requests;m=session.dispatch({Action::Library,"1",{}});TEST_REQUIRE(requests==before);
 m=session.dispatch({Action::Library,"1","all"});TEST_REQUIRE(m.entries.size()==60&&m.total==3700&&m.page_start==0);
 m=session.dispatch({Action::Page,"next",{}});TEST_REQUIRE(m.entries.front().id=="61"&&m.page_start==60);
 m=session.dispatch({Action::Page,"previous",{}});TEST_REQUIRE(m.entries.front().id=="1"&&m.page_start==0);
 m=session.dispatch({Action::Browse,"1","/library/sections/999/all"});TEST_REQUIRE(!m.message.empty());
 m=session.dispatch({Action::Library,"999",{}});TEST_REQUIRE(!m.message.empty());
 m=session.dispatch({Action::Details,"1",{}});TEST_REQUIRE(m.screen==Screen::Details&&m.detail.id=="1");
 TEST_REQUIRE(m.detail.cast.size()==2&&m.detail.credits.size()==2&&m.detail.credits[0].role=="Lead");TEST_REQUIRE(!m.detail_rails.empty());
 before=requests;m=session.dispatch({Action::Details,"1",{}});TEST_REQUIRE(requests==before);
 m=session.dispatch({Action::Play,{},{}});TEST_REQUIRE(m.screen==Screen::Playback&&player.opened==1);
 m=session.dispatch({Action::Pause,{},{}});TEST_REQUIRE(player.is_paused&&last_timeline.find("state=paused")!=std::string::npos);
 m=session.dispatch({Action::Stop,{},"45000"});TEST_REQUIRE(m.screen==Screen::Details&&m.detail.offset==45000&&last_timeline.find("time=45000")!=std::string::npos);
 before=requests;m=session.dispatch({Action::Home,{},{}});TEST_REQUIRE(requests==before&&m.rails.front().entries.front().offset==45000);
 m=session.dispatch({Action::Details,"7","collection"});TEST_REQUIRE(m.detail.type=="collection"&&!m.detail_rails.empty());
 m=session.dispatch({Action::Children,"7",{}});TEST_REQUIRE(m.screen==Screen::Library&&m.entries.front().type=="movie");
 m=session.dispatch({Action::Details,"8","playlist"});TEST_REQUIRE(m.detail.type=="playlist"&&!m.detail_rails.empty());
 m=session.dispatch({Action::Children,"8",{}});TEST_REQUIRE(m.entries.front().type=="track");
 m=session.dispatch({Action::Details,"9","track"});TEST_REQUIRE(m.detail.artist=="Fixture artist");
 m=session.dispatch({Action::Play,{},{}});TEST_REQUIRE(m.screen==Screen::Playback&&player.last_url.find("/library/parts/9/file.mp3?")!=std::string::npos);
 m=session.dispatch({Action::Stop,{},{}});
 m=session.dispatch({Action::FreeTV,{},{}});TEST_REQUIRE(m.entries.size()==1&&m.entries.front().type=="channel");
 TEST_REQUIRE(session.media_context().base==plex::free_tv_origin&&session.media_context().token=="profile-token");
 m=session.dispatch({Action::PlayLive,"abc123",{}});TEST_REQUIRE(m.screen==Screen::Playback&&player.last_url.rfind("https://epg.provider.plex.tv/library/parts/channel123?",0)==0);
 before=requests;m=session.dispatch({Action::Tick,{},{}});m=session.dispatch({Action::Stop,{},{}});TEST_REQUIRE(requests==before&&m.screen==Screen::Library);
 auto protected_data=plex::Json::parse(R"({"MediaContainer":{"Channel":[{"id":"a","Media":[{"drm":"widevine","Part":[{"key":"/library/parts/1"}]}]},{"id":"b","Media":[{"Part":[{"key":"https://foreign.test/stream"}]}]}]}})");
 TEST_REQUIRE(plex::free_channels(protected_data).empty());
 plex::Item track;track.type="track";track.parent_thumb="/album-cover";TEST_REQUIRE(plex::image_candidates(track).front()=="/album-cover");
 TEST_REQUIRE(plex::image_candidates(episode).front()=="/show-poster");
 m=session.dispatch({Action::Profile,"2",{}});TEST_REQUIRE(m.screen==Screen::Pin);
 switch_failure=true;m=session.dispatch({Action::Profile,"2","1234"});TEST_REQUIRE(m.screen==Screen::Profiles);TEST_REQUIRE(m.sections.empty()&&m.rails.empty()&&m.entries.empty()&&m.navigation.empty()&&m.guide.channels.empty());
 m=session.dispatch({Action::SignOut,{},{}});TEST_REQUIRE(m.screen==Screen::Welcome);TEST_REQUIRE(!std::filesystem::exists(root+"/account.json"));
 struct stat st{};TEST_REQUIRE(stat((root+"/device.json").c_str(),&st)==0);TEST_REQUIRE((st.st_mode&0777)==0600);
 std::filesystem::remove_all(root);
 std::cout<<"PASS: identity, navigation, episode poster, secure persistence, restore/profile/PIN, server discovery, authorized Home, details, direct-play dispatch, context isolation, sign-out ("<<requests<<" fixture requests)\n";
}
