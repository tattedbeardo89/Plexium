// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "../tools/host-vlc.h"
#include "fixtures/plex_server.h"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <thread>
#include <iostream>
#include <malloc.h>
#include <unistd.h>
using namespace plexium;
#if defined(__SANITIZE_ADDRESS__)
extern "C" size_t __sanitizer_get_current_allocated_bytes();
#endif
namespace {
// ASan retains freed allocations in its default quarantine. Count live owned
// bytes separately; keep all sanitizers enabled and leave quarantine unchanged.
#if defined(__SANITIZE_ADDRESS__)
constexpr size_t rss_growth_limit=320u*1024*1024;
const char *allocator_kind="ASan live allocated bytes (excludes freed quarantine)";
#else
constexpr size_t rss_growth_limit=32u*1024*1024;
const char *allocator_kind="glibc mallinfo2 in-use bytes";
#endif
struct Resources {size_t rss=0,heap=0,fds=0,threads=0;};
Resources resources(){Resources r;std::ifstream stat("/proc/self/statm");size_t pages=0;stat>>pages>>r.rss;r.rss*=size_t(sysconf(_SC_PAGESIZE));
#if defined(__SANITIZE_ADDRESS__)
r.heap=__sanitizer_get_current_allocated_bytes();
#else
r.heap=mallinfo2().uordblks;
#endif
for(const auto&p:std::filesystem::directory_iterator("/proc/self/fd")){(void)p;++r.fds;}for(const auto&p:std::filesystem::directory_iterator("/proc/self/task")){(void)p;++r.threads;}return r;}
plex::Json json(Resources r){return {{"rss_bytes",r.rss},{"allocator_inuse_bytes",r.heap},{"open_files",r.fds},{"threads",r.threads}};}
struct Player:HostVlcPlayer {
 std::string directory;unsigned opens=0;bool broken=false;bool open(const std::string&url,int64_t offset)override{TEST_REQUIRE(url.rfind("https://fixture.test/library/parts/",0)==0);const char*ext=url.find("/102/")!=std::string::npos?"mp3":url.find("/103/")!=std::string::npos?"flac":url.find("/104/")!=std::string::npos?"ogg":"m4a";++opens;return HostVlcPlayer::open(directory+(broken?"/deliberately-missing.":"/continuity.")+ext,offset);}
};
struct Catalog {
 Fixture prior;int failure=0;size_t requests=0;size_t timelines=0;unsigned delay_ms=0;int delay_after_opens=-1;Player&player;
 plex::Json track(int n){const char*codec[]={"aac","mp3","flac","vorbis"};const char*ext[]={"m4a","mp3","flac","ogg"};return {{"ratingKey",std::to_string(101+n)},{"type","track"},{"librarySectionID","3"},{"title",std::string("Synthetic ")+codec[n]},{"duration",650},{"Media",plex::Json::array({{{"container",ext[n]},{"audioCodec",codec[n]},{"Part",plex::Json::array({{{"key",std::string("/library/parts/")+std::to_string(101+n)+"/file."+ext[n]}}})}}})}};}
 plex::Reply operator()(const plex::Request&q){++requests;auto has=[&](const char*s){return q.url.find(s)!=std::string::npos;};if(q.url.rfind("https://fixture.test",0)==0&&failure)return {failure==1?0:failure,"{}",failure==1?"Synthetic DNS/TLS/connection failure":""};
  if(has("/:/timeline")){++timelines;if(delay_ms&&int(player.opens)==delay_after_opens){std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));}return {200,"{}",""};}
  if(has("/api/v2/home/users"))return {200,plex::Json{{"users",plex::Json::array({{{"id",1},{"title","Owner"},{"protected",false}},{{"id",2},{"title","Other"},{"protected",false}}})}}.dump(),""};
  if(has("/api/home/users/2"))return {200,plex::Json{{"id",2},{"authenticationToken","profile-two-token"}}.dump(),""};
  if(has("/library/sections"))return {200,plex::Json{{"MediaContainer",{{"Directory",plex::Json::array({{{"key","3"},{"type","artist"},{"title","Music"}}})}}}}.dump(),""};
  if(has("/library/metadata/500?"))return {200,plex::Json{{"MediaContainer",{{"Metadata",plex::Json::array({{{"ratingKey","500"},{"type","album"},{"librarySectionID","3"},{"title","Continuity fixture"}}})}}}}.dump(),""};
  plex::Json entries=plex::Json::array();if(has("/500/children"))for(int n=0;n<4;++n)entries.push_back(track(n));else for(int n=0;n<4;++n)if(q.url.find(std::to_string(101+n))!=std::string::npos)entries.push_back(track(n));
  if(has("/library/metadata/")&&!has("/related"))return {200,plex::Json{{"MediaContainer",{{"Metadata",entries},{"totalSize",entries.size()}}}}.dump(),""};return prior(q);
 }
};
template<class F>void wait(F f){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(12);while(!f()){TEST_REQUIRE(std::chrono::steady_clock::now()<end);std::this_thread::sleep_until(std::chrono::steady_clock::now()+std::chrono::milliseconds(5));}}
plex::Json stats(std::vector<double> values){std::sort(values.begin(),values.end());TEST_REQUIRE(!values.empty());plex::Json r={{"count",values.size()},{"median_ms",values[values.size()/2]},{"max_ms",values.back()}};if(values.size()>=20)r["p95_ms"]=values[size_t(std::ceil(values.size()*.95))-1];if(values.size()>=100)r["p99_ms"]=values[size_t(std::ceil(values.size()*.99))-1];return r;}
}
int main(int argc,char**argv){
 TEST_REQUIRE(argc==5);std::cout.setf(std::ios::unitbuf);const int cycles=std::stoi(argv[2]),seconds=std::stoi(argv[3]);TEST_REQUIRE(cycles>=4&&cycles<=1000&&seconds>=0&&seconds<=3600);Player player;TEST_REQUIRE(player.ready());player.directory=argv[1];player.enable_measurements();Catalog fixture{{},0,0,0,0,-1,player};
 char temp[]="/tmp/plexium-continuity-XXXXXX";TEST_REQUIRE(mkdtemp(temp));const std::string root=temp;TEST_REQUIRE(plex::save_credentials(root+"/account.json","fixture-device","account-token"));double clock=1000;auto transport=[&](const plex::Request&q){return fixture(q);};
 auto session=std::make_unique<Session>(transport,root,player,[&]{return clock;});auto send=[&](Action a,std::string id="",std::string text=""){return session->dispatch({a,id,text});};send(Action::Restore);send(Action::Profile,"1");send(Action::Details,"500");auto m=send(Action::QueuePlay);TEST_REQUIRE(m.music_total==4);send(Action::MusicRepeat);send(Action::MusicHome);
 std::vector<double> gaps,scheduled,startup,detection,advance;std::vector<PcmBlock> carry;plex::Json edges=plex::Json::array(),resource_samples=plex::Json::array();Resources baseline,peak;auto began=AudioProbe::clock();int completed=0;uint64_t pcm_frames=0,pcm_zeros=0,pcm_longest_zero_run=0,pcm_pts_holes=0;
 for(int n=0;n<cycles||(seconds&&AudioProbe::clock()-began<int64_t(seconds)*1000000);++n){
  std::cout<<"Continuity transition "<<n<<" opens="<<player.opens<<" samples="<<player.samples.load()<<"\n";
  wait([&]{return player.samples>0;});m=send(Action::Tick);TEST_REQUIRE(m.music_active&&m.screen==Screen::Home);
  auto outgoing_opens=player.opens;fixture.delay_ms=200;fixture.delay_after_opens=int(outgoing_opens);wait([&]{return player.observe().ended&&player.ended_us>0;});auto ended=player.ended_us.load();TEST_REQUIRE(ended>0);auto before=std::move(carry);auto tail=player.probe.take();before.insert(before.end(),tail.begin(),tail.end());TEST_REQUIRE(!before.empty());const auto last=before.back();uint64_t zeros=0,frames=0,longest=0,trailing=0,holes=0;int64_t expected=0;for(const auto&b:before){zeros+=b.zero_frames;frames+=b.frames;longest=std::max(longest,std::max(uint64_t(b.zero_run),trailing+b.leading_zeros));trailing=b.trailing_zeros==b.frames?trailing+b.frames:b.trailing_zeros;if(expected&&b.pts_us>expected+42)++holes;expected=b.pts_us+int64_t(b.frames)*1000000/48000;}
  const auto detected=AudioProbe::clock();m=send(Action::Tick);const auto advanced=AudioProbe::clock();TEST_REQUIRE(player.opens==outgoing_opens+1&&m.screen==Screen::Home);
  wait([&]{return player.samples>0;});auto after=player.probe.take();TEST_REQUIRE(!after.empty());auto first=after.front();TEST_REQUIRE(first.generation!=last.generation);double gap=(first.callback_us-last.callback_us)/1000.;double scheduled_gap=(first.pts_us-first.vlc_clock_us+first.callback_us-(last.pts_us-last.vlc_clock_us+last.callback_us+int64_t(last.frames)*1000000/48000))/1000.;
  pcm_frames+=frames;pcm_zeros+=zeros;pcm_longest_zero_run=std::max(pcm_longest_zero_run,longest);pcm_pts_holes+=holes;gaps.push_back(gap);scheduled.push_back(scheduled_gap);startup.push_back((first.callback_us-player.open_started_us)/1000.);detection.push_back((detected-ended)/1000.);advance.push_back((advanced-detected)/1000.);
  if(edges.size()<32)edges.push_back({{"transition",n},{"last_pcm",{last.last_left,last.last_right}},{"first_pcm",{first.first_left,first.first_right}},{"last_pts_us",last.pts_us+int64_t(last.frames-1)*1000000/48000},{"last_callback_monotonic_us",last.callback_us},{"first_callback_monotonic_us",first.callback_us},{"open_started_monotonic_us",player.open_started_us.load()},{"open_returned_monotonic_us",player.open_returned_us.load()},{"end_event_monotonic_us",ended},{"queue_advance_started_monotonic_us",detected},{"queue_advance_finished_monotonic_us",advanced},{"first_pts_us",first.pts_us},{"callback_gap_ms",gap},{"scheduled_pts_gap_ms",scheduled_gap},{"decoded_frames",frames},{"exact_zero_frames",zeros},{"longest_zero_run_frames",longest},{"intra_track_pts_holes",holes}});
  carry=std::move(after);
  if(n%4==3){auto r=resources();resource_samples.push_back(json(r));std::cout<<"Resources "<<json(r).dump()<<"\n";if(n==3)baseline=r;peak.rss=std::max(peak.rss,r.rss);peak.heap=std::max(peak.heap,r.heap);peak.fds=std::max(peak.fds,r.fds);peak.threads=std::max(peak.threads,r.threads);TEST_REQUIRE(r.rss<=baseline.rss+rss_growth_limit&&r.heap<=baseline.heap+8u*1024*1024&&r.fds<=baseline.fds+4&&r.threads<=baseline.threads+8);TEST_REQUIRE(m.queue_bytes<=8u*1024*1024&&m.cache_bytes<=34u*1024*1024&&m.prefetch_bytes<=256u*1024);}
  ++completed;clock+=.65;
 }
 // Control/restart/authorization recovery with real decode, no private service.
 std::cout<<"Continuity controls/restart/recovery\n";
 fixture.delay_ms=0;send(Action::MusicShuffle);send(Action::MusicRepeat);send(Action::MusicPause);wait([&]{return player.paused();});send(Action::MusicPause);wait([&]{return !player.paused();});send(Action::MusicOpen);send(Action::Seek,"back");send(Action::Details,"101");send(Action::QueueAdd,"later");m=send(Action::Queue);TEST_REQUIRE(m.music_total==5);send(Action::QueueEdit,m.queue_ids.back(),"up");send(Action::MusicHome);wait([&]{return player.samples>0;});send(Action::Tick);send(Action::Stop);auto at=player.samples.load();TEST_REQUIRE(!player.observe().playing);session->shutdown();session.reset();TEST_REQUIRE(player.samples==at);
 std::cout<<"Continuity cold restart\n";auto opens=player.opens;session=std::make_unique<Session>(transport,root,player,[&]{return clock;});m=send(Action::Restore);TEST_REQUIRE(m.music_available&&!m.music_active&&player.opens==opens);send(Action::QueueResume);wait([&]{return player.samples>0;});send(Action::MusicHome);
 std::cout<<"Continuity reconnect rounds\n";for(int round=0;round<8;++round)for(int failure:{1,503,408,429}){fixture.failure=failure;clock+=31;m=send(Action::Tick);TEST_REQUIRE(!m.connected);auto requests=fixture.requests;send(Action::Tick);TEST_REQUIRE(fixture.requests==requests);fixture.failure=0;clock+=2;m=send(Action::Tick);TEST_REQUIRE(m.connected);}
 std::cout<<"Continuity injected decoder error\n";player.broken=true;m=send(Action::MusicNext);std::cout<<"Injected state active="<<m.music_active<<" opens="<<player.opens<<" message="<<m.message<<"\n";wait([&]{return player.observe().failed;});fixture.failure=503;clock+=31;m=send(Action::Tick);TEST_REQUIRE(!m.music_active&&!m.connected);fixture.failure=0;player.broken=false;clock+=2;m=send(Action::Tick);TEST_REQUIRE(m.connected&&m.music_active);wait([&]{return player.samples>0;});
 std::cout<<"Continuity revocation/profile isolation\n";fixture.failure=403;clock+=31;m=send(Action::Tick);TEST_REQUIRE(!m.music_active&&!m.music_available&&m.connection_label=="Authorization required");auto requests=fixture.requests;clock+=1000;send(Action::Tick);TEST_REQUIRE(fixture.requests==requests);fixture.failure=0;send(Action::ReconnectServer);TEST_REQUIRE(!player.observe().playing);send(Action::Home,"profiles");m=send(Action::Profile,"2");TEST_REQUIRE(m.connected&&!m.music_available&&!m.music_active);send(Action::Home,"profiles");m=send(Action::Profile,"1");TEST_REQUIRE(m.connected&&m.music_available&&!m.music_active);send(Action::QueueResume);wait([&]{return player.samples>0;});session->shutdown();session.reset();TEST_REQUIRE(!player.observe().playing&&!player.observe().buffering);TEST_REQUIRE(player.probe.lost()==0);
 auto final=resources();plex::Json report={{"schema",1},{"scope","Actual host VLC S16N 48kHz stereo silent callback sink; no device/speaker output"},{"vlc_version",libvlc_get_version()},{"end_poll_interval_ms",5},{"injected_outgoing_timeline_delay_ms",200},{"expected_injected_decode_failures",1},{"successful_reconnect_rounds",32},{"retained_queue_bytes",m.queue_bytes},{"retained_cache_bytes",m.cache_bytes},{"transitions",completed},{"elapsed_seconds",(AudioProbe::clock()-began)/1000000.},{"callback_delivery_gap",stats(gaps)},{"scheduled_pts_gap",stats(scheduled)},{"decoder_startup",stats(startup)},{"end_detection",stats(detection)},{"queue_advance",stats(advance)},{"decoded_frames",pcm_frames},{"exact_zero_frames",pcm_zeros},{"longest_zero_run_frames",pcm_longest_zero_run},{"intra_track_pts_holes",pcm_pts_holes},{"hardware_underruns","unobservable: no output device"},{"samples",edges},{"allocator_measurement",allocator_kind},{"rss_growth_limit_bytes",rss_growth_limit},{"resources_after_warmup",json(baseline)},{"resources_peak",json(peak)},{"resources_final_stopped",json(final)},{"resource_samples",resource_samples},{"playback_failures",0},{"dropped_probe_blocks",player.probe.lost()},{"recovery_cases",{"transport/DNS/TLS category","503 restart","408 delayed response","429 backoff","403 revoked","cold resume","profile context","shutdown while decoding"}}};std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';TEST_REQUIRE(out.good());std::cout<<report.dump(2)<<'\n';std::filesystem::remove_all(root);
}
