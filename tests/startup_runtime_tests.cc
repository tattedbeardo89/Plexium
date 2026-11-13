#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "runtime/worker.h"
#include "runtime/render_queue.h"
#include "runtime/task.h"
#include "runtime/owned_jobs.h"
#include "runtime/shutdown.h"
#include "audio_ports.h"
#include <thread>
#include <vector>
#include <iostream>
using namespace plexium;
int main(){
 // Timeout preserves a running callback's owner; retry joins after an explicit
 // release event. No sleep or detached thread is used to fake cancellation.
 for(int cycle=0;cycle<32;++cycle){
  OwnedJobs owned;std::promise<void> started,release_job;auto barrier=release_job.get_future().share();
  TEST_REQUIRE(owned.start([&]{started.set_value();barrier.wait();}));TEST_REQUIRE(started.get_future().wait_for(std::chrono::seconds(5))==std::future_status::ready);
  TEST_REQUIRE(!owned.close(std::chrono::milliseconds(0)));
  TEST_REQUIRE(!owned.start([]{}));release_job.set_value();
  TEST_REQUIRE(owned.close(std::chrono::seconds(5)));TEST_REQUIRE(owned.close(std::chrono::milliseconds(0)));
  TEST_REQUIRE(owned.reopen() && owned.start([]{}));TEST_REQUIRE(owned.close(std::chrono::seconds(5)));
 }
 Shutdown shutdown;int steps=0;
 {
  OwnedJobs jobs;std::promise<void> release;auto barrier=release.get_future().share();
  for(int i=0;i<8;++i)TEST_REQUIRE(jobs.start([barrier]{barrier.wait();}));
  TEST_REQUIRE(!jobs.start([]{}));release.set_value();TEST_REQUIRE(jobs.close(std::chrono::seconds(5)));
 }
 TEST_REQUIRE(shutdown.step(0,"fixture",true,[&]{++steps;return ShutdownStatus::TimedOut;})==ShutdownStatus::TimedOut);
 shutdown.step(0,"fixture",true,[&]{++steps;return ShutdownStatus::Completed;});
 TEST_REQUIRE(steps==1 && !shutdown.successful());
 TEST_REQUIRE(shutdown.step(1,"dependency",false,[]{return ShutdownStatus::Completed;})==ShutdownStatus::Skipped);
 PlexAudioPorts ports{3,4};static int closes=0;
 auto fail=[](int handle){++closes;return handle==3?-1:0;};
 TEST_REQUIRE(!plex_audio_ports_close(&ports,fail) && ports.stereo==3 && ports.surround==-1 && closes==2);
 TEST_REQUIRE(!plex_audio_ports_close(&ports,nullptr) && ports.stereo==3);
 TEST_REQUIRE(plex_audio_ports_close(&ports,[](int){++closes;return 0;}) && closes==3);
 TEST_REQUIRE(plex_audio_ports_close(&ports,fail) && closes==3);
 Worker worker;std::atomic<int> calls{0};
 auto entry=[](void *p)->void*{++*static_cast<std::atomic<int>*>(p);return nullptr;};
 TEST_REQUIRE(worker.start(entry,&calls,1024)==EINVAL);
 TEST_REQUIRE(worker.start(entry,&calls,3u*1024*1024)==EINVAL);
 TEST_REQUIRE(worker.start(entry,&calls)==0);TEST_REQUIRE(worker.start(entry,&calls)==EBUSY);
 TEST_REQUIRE(worker.join()==0 && calls==1);TEST_REQUIRE(worker.join()==0);
 TEST_REQUIRE(worker.start(entry,&calls)==0);TEST_REQUIRE(worker.join()==0 && calls==2);
 // Failed creation leaves no result or callback live; a lane may be reused.
 Task<int> task;
 TEST_REQUIRE(task.start([&]{++calls;return 1;},1024)==EINVAL);
 TEST_REQUIRE(!task.valid() && calls==2);
 TEST_REQUIRE(task.start([]{return 42;})==0);
 TEST_REQUIRE(task.start([]{return 99;})==EBUSY);
 TEST_REQUIRE(task.get()==42 && !task.valid());
 TEST_REQUIRE(task.start([]{return 7;})==0 && task.get()==7);
 // Teardown must wait for work referencing its owner, rather than detach it.
 std::atomic<bool> entered{false},release{false};
 auto owner=std::async(std::launch::async,[&]{Task<int> joining;
  TEST_REQUIRE(joining.start([&]{entered=true;while(!release)std::this_thread::yield();++calls;return 0;})==0);
 });
 while(!entered)std::this_thread::yield();
 TEST_REQUIRE(owner.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout);
 release=true;owner.get();TEST_REQUIRE(calls==3);
 RenderQueue queue;std::atomic<int> executed{0};
 auto caller=std::async(std::launch::async,[&]{return queue.invoke([&]{++executed;return int64_t(42);});});
 while(!queue.size())std::this_thread::yield();queue.pump();TEST_REQUIRE(caller.get()==42 && executed==1);
 // Saturate the eight-slot queue. The ninth request fails immediately, and
 // shutdown wakes all eight waiters without executing GPU/media callbacks.
 std::vector<std::future<int64_t>> waiting;
 for(int i=0;i<8;++i)waiting.push_back(std::async(std::launch::async,[&]{return queue.invoke([&]{++executed;return int64_t(7);});}));
 while(queue.size()!=8)std::this_thread::yield();
 TEST_REQUIRE(queue.invoke([]{return int64_t(99);})==-1);
 queue.close();for(auto &result:waiting)TEST_REQUIRE(result.get()==-1);
 TEST_REQUIRE(executed==1 && queue.size()==0);TEST_REQUIRE(queue.invoke([]{return int64_t(1);})==-1);queue.close();
 // Expired operations must never execute later when the render thread resumes.
 RenderQueue timeout;
 auto expired=std::async(std::launch::async,[&]{return timeout.invoke([&]{++executed;return int64_t(8);});});
 TEST_REQUIRE(expired.get()==-1);timeout.pump();TEST_REQUIRE(executed==1);
 std::cout<<"PASS: explicit worker stack bounds, restart/join, render queue capacity, cancellation, timeout, shutdown wakeup\n";
}
