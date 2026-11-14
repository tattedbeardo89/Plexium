// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "plex/live_session.h"
#include "plex/guide_preferences.h"
#include <filesystem>
#include <iostream>
using namespace plex;
// Synthetic consumer grants. These fields are deliberately NOT asserted to be
// PMS tune-response fields. Only DvrRequests emits actual documented requests.
struct DvrFixture:LiveProtocol {
 int tunes=0,pings=0,releases=0,player_opens=0,player_stops=0;bool authorize=true,missing=false,release_fail=false,heartbeat_fail=false,player_fail=false,stop_fail=false,ambiguous=false;int heartbeat_status=503;
 std::vector<std::string> events;Client client;DvrRequests requests;
 DvrFixture():client([this](const Request&q){
  TEST_REQUIRE(q.headers.at("X-Plex-Token")=="fixture-profile"&&q.timeout_ms<=2000);
  if(q.method=="POST"){++tunes;events.push_back("acquire");if(!authorize)return Reply{401,"{}",""};return Reply{200,R"({"MediaContainer":{"Metadata":[{"live":true,"Media":[{"uuid":"fixture-session"}]}]}})",""};}
  if(q.method=="DELETE"){++releases;events.push_back("release");return Reply{release_fail?503:200,"{}",""};}
  ++pings;return Reply{heartbeat_fail?heartbeat_status:200,"{}",""};
 },"fixture-device"),requests(client,"https://fixture.test","fixture-profile"){}
 bool validated()const override{return true;}
 LiveAcquisition acquire(const LiveChannel &channel)override{
  auto r=requests.tune(channel);if(!r)return {r,{}};
  if(ambiguous)return {{{},"Unknown acquisition outcome",0},{"","","",false,true}};
  return {r,{"fixture-session","fixture-consumer",missing?"":"https://fixture.test/livetv/sessions/fixture-session/fixture-consumer/index.m3u8",true,false,true}};
 }
 Result heartbeat(const LiveLease&l)override{return requests.inspect(l.session);} // fixture health pulse, not a verified PMS consumer keepalive
 Result release(const LiveLease&l)override{return requests.release_owned(l);}
 LivePlayer player(){return {[this](const std::string&){++player_opens;events.push_back("open");return !player_fail;},[this]{++player_stops;events.push_back("stop");return !stop_fail;}};}
};
int main(){
 LiveChannel a{"one","1","First","2.1"},b{"two","1","Second","2.2"};
 PmsLiveProtocol production;int touched=0;LiveTvSession gated(production,{[&](const std::string&){++touched;return true;},[&]{++touched;return true;}},"https://fixture.test");TEST_REQUIRE(!gated.start(a,0)&&touched==0);
 {DvrFixture f;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(s.start(a,0)&&s.snapshot().state==LiveState::Buffering);s.tick(1,true,false,false);TEST_REQUIRE(s.snapshot().state==LiveState::Playing);s.tick(11,true,false,false);TEST_REQUIRE(f.pings==1);TEST_REQUIRE(s.step({a,b},1,12)&&s.snapshot().previous.id==a.id);TEST_REQUIRE(f.events[2]=="stop"&&f.events[3]=="release"&&f.events[4]=="acquire");TEST_REQUIRE(s.last(13)&&s.snapshot().current.id==a.id);TEST_REQUIRE(s.shutdown()&&s.stop()&&f.releases==3&&!s.owns_resources());}
 {DvrFixture f;f.authorize=false;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(!s.start(a,0)&&f.player_opens==0&&f.releases==0);}
 {DvrFixture f;f.missing=true;f.release_fail=true;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(!s.start(a,0)&&s.owns_resources());auto calls=f.tunes;TEST_REQUIRE(!s.start(b,1)&&f.tunes==calls);f.release_fail=false;TEST_REQUIRE(s.stop()&&!s.owns_resources());}
 {DvrFixture f;f.player_fail=true;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(!s.start(a,0)&&f.player_stops==1&&f.releases==1);}
 {DvrFixture f;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(s.start(a,0));f.stop_fail=true;TEST_REQUIRE(!s.stop()&&f.releases==0);TEST_REQUIRE(!s.start(b,1)&&f.tunes==1);f.stop_fail=false;TEST_REQUIRE(s.shutdown()&&f.releases==1);}
 for(int status:{401,403,404,503}){DvrFixture f;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(s.start(a,0));f.heartbeat_fail=true;f.heartbeat_status=status;s.tick(11,true,false,false);if(status==503){TEST_REQUIRE(s.owns_resources());s.tick(12,true,false,false);TEST_REQUIRE(f.pings==1);s.tick(22,true,false,false);s.tick(43,true,false,false);}TEST_REQUIRE(!s.owns_resources()&&f.releases==1);}
 {DvrFixture f;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(s.start(a,0));s.tick(2,false,false,true);TEST_REQUIRE(!s.owns_resources()&&f.releases==1);}
 {DvrFixture f;f.ambiguous=true;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(!s.start(a,0)&&s.owns_resources());TEST_REQUIRE(!s.start(b,2)&&f.tunes==1&&f.releases==0);}
 {DvrFixture f;f.player_fail=true;f.stop_fail=true;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(!s.start(a,0)&&s.owns_resources());TEST_REQUIRE(!s.start(b,1)&&f.tunes==1&&f.releases==0);f.stop_fail=false;TEST_REQUIRE(s.stop()&&f.releases==1);}
 {DvrFixture f;LiveTvSession s(f,f.player(),"https://fixture.test");TEST_REQUIRE(s.start(a,0));f.release_fail=true;TEST_REQUIRE(!s.stop());s.tick(20,true,false,false);TEST_REQUIRE(s.snapshot().state==LiveState::Recovery&&f.pings==0);f.release_fail=false;s.tick(31,true,false,false);TEST_REQUIRE(!s.owns_resources());}
 TEST_REQUIRE(!live_playlist("https://fixture.test","https://evil.test/livetv/sessions/a/b/index.m3u8"));TEST_REQUIRE(!live_playlist("https://fixture.test","https://fixture.test/livetv/sessions/a/../index.m3u8"));TEST_REQUIRE(!live_playlist("https://fixture.test","https://fixture.test/livetv/sessions/a/b/index.m3u8.exe"));
 char dir[]="/tmp/plexium-guide-prefs-XXXXXX";TEST_REQUIRE(mkdtemp(dir));GuidePreferences prefs;TEST_REQUIRE(prefs.bind(dir,"a/p/s")&&prefs.toggle("tv.plex.providers.epg.xmltv:1/one")&&prefs.filter());GuidePreferences restored;TEST_REQUIRE(restored.bind(dir,"a/p/s")&&restored.only&&restored.favorites.size()==1);TEST_REQUIRE(restored.bind(dir,"a/other/s")&&!restored.only&&restored.favorites.empty());std::filesystem::remove_all(dir);
 std::cout<<"PASS: fixture-only live ownership, acquisition failure/success, missing stream, bounded health recovery, expiry, switch/last, failed release/stop retention, duplicate stop/shutdown; production tuning gate and scoped guide preferences\n";
}
