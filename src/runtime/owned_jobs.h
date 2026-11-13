// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "worker.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
namespace plexium {
class OwnedJob {
 std::function<void()> function;
 std::mutex mutex;
 std::condition_variable changed;
 bool done=true;
 Worker worker;
 public:
 int start(std::function<void()> fn,size_t stack_bytes=1024u*1024){
  if(worker.joinable()){if(!wait(std::chrono::milliseconds(0)))return EBUSY;}
  function=std::move(fn);done=false;
  int rc=worker.start([](void *p)->void*{
   auto &job=*static_cast<OwnedJob*>(p);job.function();
   {std::lock_guard<std::mutex> lock(job.mutex);job.done=true;}
   job.changed.notify_all();return nullptr;
  },this,stack_bytes);
  if(rc){done=true;function={};}return rc;
 }
 bool wait(std::chrono::milliseconds timeout){
  if(!worker.joinable())return true;
  {std::unique_lock<std::mutex> lock(mutex);if(!changed.wait_for(lock,timeout,[&]{return done;}))return false;}
  if(worker.join()!=0)return false;function={};return true;
 }
 bool joinable()const{return worker.joinable();}
};
// Owner survives any timeout. No timed detach: callers must retain the media
// context until close succeeds, or terminate the process with those jobs owned.
class OwnedJobs {
 std::vector<std::unique_ptr<OwnedJob>> jobs;
 bool closed=false;
 public:
 bool start(std::function<void()> function){
  if(closed)return false;
  for(auto i=jobs.begin();i!=jobs.end();)if((*i)->wait(std::chrono::milliseconds(0)))i=jobs.erase(i);else ++i;
  if(jobs.size()>=8)return false;
  auto job=std::make_unique<OwnedJob>();if(job->start(std::move(function)))return false;
  jobs.push_back(std::move(job));return true;
 }
 bool close(std::chrono::milliseconds timeout){
  closed=true;auto end=std::chrono::steady_clock::now()+timeout;
  for(auto &job:jobs){auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(end-std::chrono::steady_clock::now());
   if(!job->wait(std::max(remaining,std::chrono::milliseconds(0))))return false;
  }
  jobs.clear();return true;
 }
 bool reopen(){if(!jobs.empty())return false;closed=false;return true;}
};
}
