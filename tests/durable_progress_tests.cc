// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "app/session.h"
#include "fixtures/plex_server.h"
#include <filesystem>
#include <iostream>
using namespace plexium;
struct Engine:PlaybackPort{int64_t at=0;bool open(const std::string&,int64_t position)override{at=position;return true;}void stop()override{}int64_t position()const override{return at;}PlaybackObservation observe()const override{return {true,true,false,false,false,false};}};
int main(){
 char folder[]="/tmp/plexium-durable-XXXXXX";TEST_REQUIRE(mkdtemp(folder));std::string root=folder;TEST_REQUIRE(plex::save_credentials(root+"/account.json","fixture-device","account-token"));
 Fixture fixture;Engine engine;double clock=1000;bool expired=false;
 auto transport=[&](const plex::Request&q){if(expired&&q.url.find("/:/timeline")!=std::string::npos){++fixture.timelines;return plex::Reply{401,"{}",""};}return fixture(q);};
 {Session session(transport,root,engine,[&]{return clock;});session.dispatch({Action::Restore,{},{}});session.dispatch({Action::Profile,"1",{}});session.dispatch({Action::Details,"1",{}});session.dispatch({Action::Play,{},{}});session.dispatch({Action::Tick,{},{}});
  fixture.outage=true;engine.at=12000;session.dispatch({Action::Tick,{},{}});session.shutdown();}
 plex::Json saved;TEST_REQUIRE(read_private_json(root+"/progress-v1.json",saved)&&saved["records"].size()==1&&plex::text(saved["records"][0],"state")=="stopped");
 fixture.outage=false;clock+=100;int before=fixture.timelines;
 {Session restart(transport,root,engine,[&]{return clock;});auto model=restart.dispatch({Action::Restore,{},{}});TEST_REQUIRE(model.screen==Screen::Home);clock+=301;restart.dispatch({Action::Tick,{},{}});TEST_REQUIRE(fixture.timelines==before+1&&fixture.last_timeline.find("time=12000")!=std::string::npos);before=fixture.timelines;restart.dispatch({Action::Tick,{},{}});TEST_REQUIRE(fixture.timelines==before);}
 // Persist another interrupted session, then prove newer server state wins.
 {Session session(transport,root,engine,[&]{return clock;});session.dispatch({Action::Restore,{},{}});session.dispatch({Action::Details,"1",{}});session.dispatch({Action::Play,{},{}});session.dispatch({Action::Tick,{},{}});fixture.outage=true;engine.at=22000;session.dispatch({Action::Tick,{},{}});session.shutdown();}
 fixture.outage=false;fixture.metadata["viewOffset"]=30000;before=fixture.timelines;
 {Session restart(transport,root,engine,[&]{return clock;});restart.dispatch({Action::Restore,{},{}});clock+=301;restart.dispatch({Action::Tick,{},{}});TEST_REQUIRE(fixture.timelines==before);}
 // Unauthorized writes are retained but stop retrying until a fresh connection.
 fixture.metadata["viewOffset"]=1000;
 {Session session(transport,root,engine,[&]{return clock;});session.dispatch({Action::Restore,{},{}});session.dispatch({Action::Details,"1",{}});session.dispatch({Action::Play,{},{}});session.dispatch({Action::Tick,{},{}});expired=true;engine.at=24000;session.dispatch({Action::Tick,{},{}});before=fixture.timelines;clock+=1000;session.dispatch({Action::Tick,{},{}});TEST_REQUIRE(fixture.timelines==before);expired=false;session.dispatch({Action::Server,"server",{}});clock+=301;session.dispatch({Action::Tick,{},{}});TEST_REQUIRE(fixture.timelines>before);session.dispatch({Action::SignOut,{},{}});}
 std::filesystem::remove_all(root);std::cout<<"PASS: interrupted playback journal, application restart/reconnect, duplicate suppression, newer server progress protection and expired-authorization retry gating\n";
}
