// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "app/session.h"
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
using namespace plexium;
struct Engine:PlaybackPort {
 int opens=0,stops=0;bool reject=false;int64_t at=12000;
 PlaybackObservation observation{true,false,true,false,false,false};
 bool open(const std::string&,int64_t offset)override{++opens;at=offset;return !reject;}
 void stop()override{++stops;}int64_t position()const override{return at;}
 void seek(int64_t value)override{at=value;}void pause()override{observation.paused=!observation.paused;}
 bool paused()const override{return observation.paused;}PlaybackObservation observe()const override{return observation;}
};
#include "fixtures/plex_server.h"

int main(){
 std::string root="/tmp/plexium-playback-"+std::to_string(getpid());std::filesystem::create_directory(root);test_posix_result(chmod(root.c_str(),0700),"chmod");
 TEST_REQUIRE(plex::save_credentials(root+"/account.json","test-device","account-token"));
 double clock=1000;Fixture fixture;Engine engine;Session session([&](const plex::Request &q){return fixture(q);},root,engine,[&]{return clock;});
 auto send=[&](Action a,std::string id=""){return session.dispatch({a,id,{}});};
 TEST_REQUIRE(send(Action::Restore).screen==Screen::Profiles);TEST_REQUIRE(send(Action::Profile,"1").screen==Screen::Home);
 auto model=send(Action::Details,"1");TEST_REQUIRE(model.detail.media.size()==2);
 model=send(Action::Play);TEST_REQUIRE(model.screen==Screen::Playback&&model.playback_mode==plex::Mode::DirectPlay&&model.playback_state==PlaybackState::Opening&&fixture.timelines==0);
 model=send(Action::Tick);TEST_REQUIRE(model.playback_state==PlaybackState::Buffering&&fixture.timelines==0);
 engine.observation={true,true,false,false,false,false};model=send(Action::Tick);TEST_REQUIRE(model.playback_state==PlaybackState::Playing&&fixture.timelines==1);
 model=send(Action::Pause);TEST_REQUIRE(model.playback_state==PlaybackState::Paused&&fixture.last_timeline.find("state=paused")!=std::string::npos);
 model=send(Action::Seek);TEST_REQUIRE(model.playback_state==PlaybackState::Seeking&&engine.at==11000);
 fixture.outage=true;model=send(Action::Stop);TEST_REQUIRE(model.playback_state==PlaybackState::Stopped);fixture.outage=false;auto reports=fixture.timelines;clock+=11;send(Action::Tick);TEST_REQUIRE(fixture.timelines==reports+1&&fixture.last_timeline.find("state=stopped")!=std::string::npos);
 model=send(Action::Version);TEST_REQUIRE(model.media_version==1);
 model=send(Action::Play);TEST_REQUIRE(model.playback_mode==plex::Mode::DirectStream&&fixture.last_decision.find("mediaIndex=1")!=std::string::npos&&fixture.last_decision.find("Chrome")==std::string::npos);
 clock+=11;send(Action::Tick);TEST_REQUIRE(fixture.pings==1);
 fixture.outage=true;model=send(Action::Stop);TEST_REQUIRE(model.playback_state==PlaybackState::Error);
 auto opens=engine.opens;model=send(Action::Play);TEST_REQUIRE(engine.opens==opens); // retained lease blocks overwrite
 fixture.outage=false;model=send(Action::Stop);TEST_REQUIRE(model.playback_state==PlaybackState::Stopped);
 fixture.video="transcode";model=send(Action::Play);TEST_REQUIRE(model.playback_mode==plex::Mode::Transcode);send(Action::Stop);
 fixture.bad=true;opens=engine.opens;model=send(Action::Play);TEST_REQUIRE(model.playback_state==PlaybackState::Error&&engine.opens==opens);fixture.bad=false;
 // Immediate player failure triggers a single negotiated fallback; HTTP success alone never means Playing.
 send(Action::Version);engine.reject=true;opens=engine.opens;model=send(Action::Play);TEST_REQUIRE(engine.opens==opens+2&&model.playback_state==PlaybackState::Error);engine.reject=false;
 // Output capabilities and malformed decisions are validated independently of HTTP status.
 plex::Client client([&](const plex::Request&q){return fixture(q);},"id");plex::Item item;item.id="1";item.type="movie";item.media=fixture.metadata["Media"];
 fixture.video="copy";fixture.audio="transcode";auto plan=plex::negotiate_video(client,"https://fixture.test","server-token",item,1,plex::Quality::FHD,true,"session");TEST_REQUIRE(plan.playback.mode==plex::Mode::Transcode);
 TEST_REQUIRE(plex::negotiate_video(client,"https://fixture.test","server-token",item,8,plex::Quality::Original,true,"session").playback.mode==plex::Mode::Unsupported);
 TEST_REQUIRE(!plex::stream_control(client,"https://fixture.test","server-token","bad&session",true));
 // Several failed titles cannot multiply per-dispatch network wait budgets.
 fixture.outage=true;engine.reject=false;engine.observation={true,true,false,false,false,false};
 for(const char *id:{"2","3"}){fixture.metadata["ratingKey"]=id;fixture.metadata["key"]=std::string("/library/metadata/")+id;send(Action::Details,id);send(Action::Play);send(Action::Tick);send(Action::Stop);}
 fixture.metadata["ratingKey"]="4";fixture.metadata["key"]="/library/metadata/4";send(Action::Details,"4");send(Action::Play);clock+=100;
 auto budget=fixture.timelines;send(Action::Tick);TEST_REQUIRE(fixture.timelines-budget==2);fixture.outage=false;send(Action::Stop);
 send(Action::SignOut);auto count=fixture.timelines;send(Action::Tick);TEST_REQUIRE(fixture.timelines==count);
 std::filesystem::remove_all(root);std::cout<<"PASS: production session negotiation, versions, Direct Play/Stream/transcode, state observations, timeline failures, retained lease retry and profile isolation\n";
}
