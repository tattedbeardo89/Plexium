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
struct FixturePlayer:HostVlcPlayer {
 std::string file,external_file,negotiated_file,audio_one,audio_two;bool reject_direct=false,music_mode=false;int negotiated_opens=0,opens=0;
 bool add_external_subtitle(const std::string &url)override{TEST_REQUIRE(url.rfind("https://fixture.test/library/streams/",0)==0);return HostVlcPlayer::add_external_subtitle("file://"+external_file);}
 bool open(const std::string &url,int64_t offset)override{
  bool negotiated=url.rfind("https://fixture.test/video/:/transcode/universal/start.m3u8?",0)==0;
  TEST_REQUIRE(negotiated||url.rfind("https://fixture.test/library/parts/",0)==0);
  if(!negotiated&&reject_direct){reject_direct=false;return false;}
  ++opens;if(music_mode)return HostVlcPlayer::open(url.find("/parts/102/")!=std::string::npos?audio_two:audio_one,offset);
  if(negotiated)++negotiated_opens;
  return HostVlcPlayer::open(negotiated?negotiated_file:file,offset);
 }
};
template<class F>bool until(F f){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(12);do{if(f())return true;std::this_thread::sleep_for(std::chrono::milliseconds(20));}while(std::chrono::steady_clock::now()<end);return false;}
int main(int argc,char **argv){
 std::cout.setf(std::ios::unitbuf);TEST_REQUIRE(argc==7);FixturePlayer player;TEST_REQUIRE(player.ready());player.file=argv[1];player.negotiated_file=argv[4];player.audio_one=argv[5];player.audio_two=argv[6];
 auto root="/tmp/plexium-vlc-"+std::to_string(getpid());std::filesystem::create_directory(root);test_posix_result(chmod(root.c_str(),0700),"chmod");TEST_REQUIRE(plex::save_credentials(root+"/account.json","host-device","account-token"));
 Fixture fixture;fixture.metadata["duration"]=18000;double clock=1000;bool episode_mode=false;
 auto transport=[&](const plex::Request&q){
  if(player.music_mode){
   if(q.url.find("/library/sections")!=std::string::npos)return plex::Reply{200,plex::Json{{"MediaContainer",{{"Directory",plex::Json::array({{{"key","1"},{"type","movie"},{"title","Movies"}},{{"key","3"},{"type","artist"},{"title","Music"}}})}}}}.dump(),""};
   auto track=[&](const char *id){return plex::Json{{"ratingKey",id},{"type","track"},{"librarySectionID","3"},{"title",std::string("Fixture audio ")+id},{"duration",3000},{"index",std::string(id)=="101"?1:2},{"Media",plex::Json::array({{{"container","m4a"},{"audioCodec","aac"},{"Part",plex::Json::array({{{"key",std::string("/library/parts/")+id+"/file.m4a"}}})}}})}};};
   plex::Json entries;
   if(q.url.find("/library/metadata/101,102?")!=std::string::npos)entries=plex::Json::array({track("101"),track("102")});
   else if(q.url.find("/library/metadata/500/children")!=std::string::npos)entries=plex::Json::array({track("101"),track("102")});
   else if(q.url.find("/library/metadata/500?")!=std::string::npos)entries=plex::Json::array({{{"ratingKey","500"},{"type","album"},{"librarySectionID","3"},{"title","Synthetic audio album"}}});
   else for(const auto *id:{"101","102"})if(q.url.find(std::string("/library/metadata/")+id+"?")!=std::string::npos)entries=plex::Json::array({track(id)});
   if(entries.is_array())return plex::Reply{200,plex::Json{{"MediaContainer",{{"Metadata",entries},{"totalSize",entries.size()}}}}.dump(),""};
  }
  if(episode_mode&&(q.url.find("/allLeaves")!=std::string::npos||q.url.find("/library/metadata/2?")!=std::string::npos)){
   auto next=fixture.metadata;next["ratingKey"]="2";next["key"]="/library/metadata/2";next["viewOffset"]=1000;next["viewCount"]=0;
   auto entries=q.url.find("/allLeaves")!=std::string::npos?plex::Json::array({fixture.metadata,next}):plex::Json::array({next});
   return plex::Reply{200,plex::Json{{"MediaContainer",{{"totalSize",2},{"Metadata",entries}}}}.dump(),""};
  }
  return fixture(q);
 };
 Session session(transport,root,player,[&]{return clock;});
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
 // Actual local HLS playback with production negotiated-session ownership.
 send(Action::Version);auto streamed=send(Action::Play);TEST_REQUIRE(streamed.playback_mode==plex::Mode::DirectStream);
 TEST_REQUIRE(until([&]{return player.frames>=8&&player.samples>=4800;}));send(Action::Tick);auto released=fixture.stops;send(Action::Stop);TEST_REQUIRE(fixture.stops==released+1);
 send(Action::Version);fixture.video="transcode";player.reject_direct=true;auto fallback=send(Action::Play);TEST_REQUIRE(fallback.playback_mode==plex::Mode::Transcode);
 TEST_REQUIRE(until([&]{return player.frames>=8&&player.samples>=4800;}));send(Action::Tick);released=fixture.stops;send(Action::Stop);TEST_REQUIRE(fixture.stops==released+1&&player.negotiated_opens==2);fixture.video="copy";
 std::cout<<"PASS: actual local HLS decode for fixture Direct Stream and transcode-fallback decisions; matched session stop/release (no real server transcoder claim)\n";
 send(Action::SignOut);
 // Real VLC end detection and next-episode reopen with fresh fixture resume data.
 episode_mode=true;fixture.metadata["type"]="episode";fixture.metadata["grandparentRatingKey"]="10";fixture.metadata["viewOffset"]=0;
 TEST_REQUIRE(plex::save_credentials(root+"/account.json","host-device","account-token"));send(Action::Restore);send(Action::Profile,"1");send(Action::Details,"1");send(Action::Play);
 TEST_REQUIRE(until([&]{return player.frames>=8&&player.samples>=4800;}));send(Action::Tick);player.seek(17500);
 TEST_REQUIRE(until([&]{auto m=send(Action::Tick);return m.screen==Screen::Details&&m.upcoming.size()==1;}));clock+=11;auto next=send(Action::Tick);TEST_REQUIRE(next.detail.id=="2"&&next.playback_state==PlaybackState::Opening);
 TEST_REQUIRE(until([&]{return player.frames>=8&&player.samples>=4800&&player.position()>=1000;}));send(Action::Tick);send(Action::Stop);send(Action::SignOut);episode_mode=false;fixture.metadata["type"]="movie";
 std::cout<<"PASS: actual VLC end observation, automatic next episode, resumed decode and stopped-session timeline cleanup through Session\n";
 player.file=argv[2];player.external_file=argv[3];fixture.metadata["duration"]=8000;fixture.metadata["viewOffset"]=0;fixture.metadata["Media"]=plex::Json::array({fixture.metadata["Media"][0]});
 fixture.metadata["Media"][0]["container"]="mkv";
 fixture.metadata["Media"][0]["Part"][0]["Stream"]=plex::Json::parse(R"([{"id":100,"streamType":2,"languageCode":"eng","codec":"aac","channels":2},{"id":101,"streamType":2,"languageCode":"spa","codec":"aac","channels":2},{"id":200,"streamType":3,"languageCode":"eng","codec":"srt"},{"id":201,"streamType":3,"languageCode":"fra","codec":"srt","key":"/library/streams/201"}])");
 TEST_REQUIRE(plex::save_credentials(root+"/account.json","host-device","account-token"));send(Action::Restore);send(Action::Profile,"1");send(Action::Details,"1");send(Action::Play);
 TEST_REQUIRE(until([&]{return player.frames>=8&&player.samples>=4800&&player.native_tracks(false).size()>=2&&player.native_tracks(true).size()>=1;}));send(Action::Tick);
 auto audio=player.native_tracks(false);auto model=send(Action::Tracks);TEST_REQUIRE(model.audio_tracks.size()==2&&model.subtitle_tracks.size()==2);
 auto spanish=plex::native_track_match(model.audio_tracks[1],audio);TEST_REQUIRE(spanish>=0&&spanish!=101);
 send(Action::SelectAudio,"101");TEST_REQUIRE(until([&]{send(Action::Tick);return player.selected_native_track(false)==spanish;}));
 model=send(Action::SelectSubtitle,"200");auto english=plex::native_track_match(model.subtitle_tracks[0],player.native_tracks(true));TEST_REQUIRE(english>=0&&english!=200);
 TEST_REQUIRE(until([&]{send(Action::Tick);return player.selected_native_track(true)==english;}));
 send(Action::SelectSubtitle,"off");TEST_REQUIRE(until([&]{send(Action::Tick);return player.selected_native_track(true)==-1;}));
 send(Action::SelectSubtitle,"201");TEST_REQUIRE(until([&]{send(Action::Tick);return player.selected_native_track(true)>=0;}));
 send(Action::SelectSubtitle,"off");TEST_REQUIRE(until([&]{send(Action::Tick);return player.selected_native_track(true)==-1;}));
 send(Action::Back);send(Action::Stop);send(Action::SignOut);
 std::cout<<"PASS: real host VLC Spanish audio, embedded subtitle, external subtitle and Off through shared Session; Plex IDs never used as ES IDs\n";
 player.music_mode=true;TEST_REQUIRE(plex::save_credentials(root+"/account.json","host-device","account-token"));send(Action::Restore);send(Action::Profile,"1");send(Action::Details,"500");auto music=send(Action::QueuePlay);TEST_REQUIRE(music.music_total==2&&music.detail.id=="101");
 TEST_REQUIRE(until([&]{return player.samples>=4800;}));send(Action::Tick);send(Action::Pause);TEST_REQUIRE(until([&]{return player.paused();}));send(Action::Pause);send(Action::MusicHome);music=send(Action::Tick);TEST_REQUIRE(music.music_active&&music.screen==Screen::Home);
 send(Action::MusicOpen);music=send(Action::MusicNext);TEST_REQUIRE(music.detail.id=="102");TEST_REQUIRE(until([&]{return player.samples>=4800;}));send(Action::Tick);send(Action::MusicRepeat);send(Action::MusicRepeat);auto opens=player.opens;
 TEST_REQUIRE(until([&]{send(Action::Tick);return player.opens>opens;}));music=send(Action::Tick);TEST_REQUIRE(music.music_active&&music.detail.id=="102"&&music.music_repeat==2);send(Action::Details,"101");send(Action::QueueAdd,"later");auto queue=send(Action::Queue);TEST_REQUIRE(queue.music_total==3&&queue.queue_ids[0]!=queue.queue_ids[2]);auto playing=queue.queue_ids[size_t(queue.music_cursor)];auto duplicate=queue.queue_ids.back();auto before_edit=player.opens;
 session.dispatch({Action::QueueEdit,duplicate,"up"});queue=send(Action::Queue);TEST_REQUIRE(queue.queue_ids[size_t(queue.music_cursor)]==playing&&player.opens==before_edit);
 session.dispatch({Action::QueueEdit,duplicate,"play"});TEST_REQUIRE(until([&]{return player.samples>=4800&&player.position()>=500;}));send(Action::Tick);
 send(Action::Stop);auto samples=player.samples.load();std::this_thread::sleep_for(std::chrono::milliseconds(100));TEST_REQUIRE(player.samples==samples);session.shutdown();
 auto stopped_opens=player.opens;Session restored(transport,root,player,[&]{return clock;});auto saved=restored.dispatch({Action::Restore,{},{}});TEST_REQUIRE(saved.music_restored&&saved.music_available&&!saved.music_active&&player.opens==stopped_opens);
 restored.dispatch({Action::QueueResume,{},{}});TEST_REQUIRE(until([&]{return player.samples>=4800;}));restored.dispatch({Action::Tick,{},{}});restored.dispatch({Action::MusicHome,{},{}});auto background_opens=player.opens;TEST_REQUIRE(until([&]{saved=restored.dispatch({Action::Tick,{},{}});return player.opens>background_opens;}));TEST_REQUIRE(saved.screen==Screen::Home&&saved.music_active);restored.dispatch({Action::Stop,{},{}});restored.dispatch({Action::SignOut,{},{}});
 std::cout<<"PASS: real AAC queue, stable duplicate edits, background/next/repeat-one, persisted restart without autoplay, resumed decode and stopped callback ownership through Session\n";
 std::filesystem::remove_all(root);
}
