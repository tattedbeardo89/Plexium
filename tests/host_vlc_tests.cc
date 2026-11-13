// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "../tools/host-vlc.h"
#include "fixtures/plex_server.h"
#include <filesystem>
#include <thread>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
using namespace plexium;
// Explicit synthetic Plex-part mapping; no network or private Plex media used.
struct FixturePlayer:HostVlcPlayer {std::string file;bool open(const std::string &url,int64_t offset)override{TEST_REQUIRE(url.rfind("https://fixture.test/library/parts/",0)==0);return HostVlcPlayer::open(file,offset);}};
template<class F>bool until(F f){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(12);do{if(f())return true;std::this_thread::sleep_for(std::chrono::milliseconds(20));}while(std::chrono::steady_clock::now()<end);return false;}
int main(int argc,char **argv){
 std::cout.setf(std::ios::unitbuf);TEST_REQUIRE(argc==2);FixturePlayer player;TEST_REQUIRE(player.ready());player.file=argv[1];
 auto root="/tmp/plexium-vlc-"+std::to_string(getpid());std::filesystem::create_directory(root);test_posix_result(chmod(root.c_str(),0700),"chmod");TEST_REQUIRE(plex::save_credentials(root+"/account.json","host-device","account-token"));
 Fixture fixture;fixture.metadata["duration"]=18000;Session session([&](const plex::Request&q){return fixture(q);},root,player);
 auto send=[&](Action a,std::string id=""){return session.dispatch({a,id,{}});};
 TEST_REQUIRE(send(Action::Restore).screen==Screen::Profiles);TEST_REQUIRE(send(Action::Profile,"1").screen==Screen::Home);TEST_REQUIRE(send(Action::Details,"1").screen==Screen::Details);
 for(int n=0;n<3;++n){
  auto model=send(Action::Play);TEST_REQUIRE(model.playback_state==PlaybackState::Opening);
  TEST_REQUIRE(until([&]{return player.frames>=8&&player.samples>=4800&&player.position()>1100;}));
  model=send(Action::Tick);TEST_REQUIRE(model.playback_state==PlaybackState::Playing);
  send(Action::Pause);TEST_REQUIRE(until([&]{return player.paused();}));
  model=send(Action::Tick);TEST_REQUIRE(model.playback_state==PlaybackState::Paused);
  send(Action::Pause);TEST_REQUIRE(until([&]{return !player.paused();}));
  auto count=player.frames.load();auto before_seek=player.position();send(Action::Seek,"back");TEST_REQUIRE(until([&]{return player.frames>count+3&&player.position()<before_seek;}));
  model=send(Action::Stop);TEST_REQUIRE(model.screen==Screen::Details&&model.playback_state==PlaybackState::Stopped);
  count=player.frames;auto samples=player.samples.load();std::this_thread::sleep_for(std::chrono::milliseconds(100));TEST_REQUIRE(player.frames==count&&player.samples==samples);
  std::cout<<"PASS: host software 1080p session "<<n+1<<", frames="<<count<<", audio samples="<<player.samples.load()<<"; pause/resume/seek/stop\n";
 }
 send(Action::SignOut);std::filesystem::remove_all(root);
}
