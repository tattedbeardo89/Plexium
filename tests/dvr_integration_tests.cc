// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "app/session.h"
#include "fixtures/plex_server.h"
#include <filesystem>
#include <iostream>
using namespace plexium;
struct GuidePlayer:PlaybackPort{int opens=0;bool open(const std::string&,int64_t)override{++opens;return true;}void stop()override{}};
int main(){
 char folder[]="/tmp/plexium-dvr-integration-XXXXXX";TEST_REQUIRE(mkdtemp(folder));std::string root=folder;TEST_REQUIRE(plex::save_credentials(root+"/account.json","fixture-device","account-token"));
 Fixture fixture;GuidePlayer player;bool configured=true,offline=false,expired=false;int lineup_calls=0,mutations=0;double clock=1000;std::string profile="1";
 auto transport=[&](const plex::Request&q){
  auto reply=[](plex::Json data){return plex::Reply{200,data.dump(),""};};
  if(q.url.find("/api/v2/home/users")!=std::string::npos)return reply({{"users",plex::Json::array({{{"id",1},{"title","Owner"},{"protected",false}},{{"id",2},{"title","Guest"},{"protected",false}}})}});
  if(q.url.find("/api/home/users/")!=std::string::npos){profile=q.url.find("/2/")!=std::string::npos?"2":"1";return reply({{"id",profile},{"authenticationToken","profile-token"}});}
  bool dvr=q.url.find("/livetv/")!=std::string::npos||q.url.find("/tv.plex.providers.")!=std::string::npos||q.url.find("/media/subscriptions")!=std::string::npos;
  if(dvr){TEST_REQUIRE(q.headers.at("X-Plex-Token")=="server-token");if(q.method!="GET")++mutations;if(expired)return plex::Reply{401,"{}",""};if(offline)return plex::Reply{503,"{}",""};}
  if(q.url.find("/livetv/dvrs")!=std::string::npos)return reply({{"MediaContainer",{{"Dvr",configured?plex::Json::array({{{"key","12"}}}):plex::Json::array()}}}});
  if(q.url.find("/media/providers")!=std::string::npos)return reply({{"MediaContainer",{{"MediaProvider",plex::Json::array({{{"identifier","tv.plex.providers.epg.xmltv:12"}}})}}}});
  if(q.url.find("/lineups/dvr/channels")!=std::string::npos){++lineup_calls;auto p=q.url.find("X-Plex-Container-Start=");int start=std::stoi(q.url.substr(p+23));plex::Json channels=plex::Json::array();for(int n=start;n<std::min(start+200,640);++n)channels.push_back({{"id","channel-"+std::to_string(n)},{"gridKey","grid-"+std::to_string(n)},{"vcn",std::to_string(n+1)},{"title","Plex fixture channel "+std::to_string(n+1)},{"thumb","/photo/fixture"}});return reply({{"MediaContainer",{{"totalSize",640},{"Channel",channels}}}});}
  if(q.url.find("/grid?")!=std::string::npos){auto wall=int64_t(std::time(nullptr));return reply({{"MediaContainer",{{"Metadata",plex::Json::array({{{"title","Plex current program"},{"summary","Fixture program details"},{"Media",plex::Json::array({{{"beginsAt",wall-600},{"endsAt",wall+1800}}})}},{{"title","Plex upcoming program"},{"Media",plex::Json::array({{{"beginsAt",wall+1800},{"endsAt",wall+3600}}})}}})}}}});}
  if(q.url.find("/media/subscriptions/scheduled")!=std::string::npos)return reply({{"MediaContainer",{{"MediaGrabOperation",plex::Json::array({{{"status","scheduled"},{"Metadata",{{"title","Fixture recording"}}}}})}}}});
  if(q.url.find("/media/subscriptions")!=std::string::npos)return reply({{"MediaContainer",{{"MediaSubscription",plex::Json::array({{{"key","1"},{"type",2},{"targetLibrarySectionID",1},{"title","Fixture recording rule"}}})}}}});
  if(q.url.find("/livetv/recordings")!=std::string::npos)return reply({{"MediaContainer",{{"Metadata",plex::Json::array({{{"ratingKey","123"},{"type","episode"},{"title","Fixture completed recording"}}})}}}});
  return fixture(q);
 };
 Session session(transport,root,player,[&]{return clock;});auto send=[&](Action a,std::string id=""){return session.dispatch({a,id,{}});};send(Action::Restore);send(Action::Profile,"1");
 auto model=send(Action::LiveTV);TEST_REQUIRE(model.guide.total==640&&model.guide.channels.size()==6&&lineup_calls==4);auto id=model.guide.channels[0].id;
 model=send(Action::TuneChannel,id);TEST_REQUIRE(player.opens==0&&mutations==0&&model.message.find("pending verified")!=std::string::npos);
 model=send(Action::FavoriteChannel,id);TEST_REQUIRE(model.guide.channels[0].favorite);model=send(Action::FavoriteFilter);TEST_REQUIRE(model.guide.total==1&&model.guide.favorites_only);
 model=send(Action::MiniGuide);TEST_REQUIRE(model.guide.mini);model=send(Action::Recordings);TEST_REQUIRE(model.screen==Screen::Recordings&&model.recording_rows.size()==3&&mutations==0);
 send(Action::Home,"profiles");send(Action::Profile,"2");model=send(Action::LiveTV);TEST_REQUIRE(profile=="2"&&model.guide.total==640&&!model.guide.favorites_only&&!model.guide.channels[0].favorite);
 send(Action::Home,"profiles");send(Action::Profile,"1");model=send(Action::LiveTV);TEST_REQUIRE(model.guide.total==1&&model.guide.channels[0].favorite);
 expired=true;model=send(Action::GuidePage,"refresh");TEST_REQUIRE(model.guide.channels.empty()&&!model.message.empty());expired=false;model=send(Action::GuidePage,"refresh");TEST_REQUIRE(!model.guide.channels.empty());
 offline=true;model=send(Action::GuidePage,"refresh");TEST_REQUIRE(model.guide.stale&&!model.guide.channels.empty());offline=false;send(Action::GuidePage,"refresh");
 model=send(Action::FavoriteChannel,id);TEST_REQUIRE(model.guide.channels.empty()&&model.guide.favorites_only);send(Action::FavoriteFilter);configured=false;model=send(Action::GuidePage,"refresh");TEST_REQUIRE(!model.guide.configured&&model.guide.channels.empty());
 session.shutdown();TEST_REQUIRE(mutations==0&&player.opens==0);send(Action::SignOut);std::filesystem::remove_all(root);
 std::cout<<"PASS: production Session 640-channel pagination, program metadata, scoped favorites/filter, mini guide, read-only recordings, expired authorization, cached offline guide, empty DVR and zero production tuning allocations\n";
}
