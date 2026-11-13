// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <pthread.h>
#include <cerrno>
#include <cstddef>
namespace plexium {
// pthread owns the stack mapping and reclaims it on join. No default-sized
// std::thread stacks on this application-owned session worker.
class Worker {
 pthread_t thread{};
 bool running=false;
 public:
 static constexpr size_t session_stack=512u*1024;
 Worker()=default;
 Worker(const Worker&)=delete;
 Worker& operator=(const Worker&)=delete;
 int start(void *(*entry)(void*),void *context,size_t bytes=session_stack) {
  if(running)return EBUSY;
  if(!entry || bytes<64u*1024 || bytes>2u*1024*1024)return EINVAL;
  pthread_attr_t attr;
  int rc=pthread_attr_init(&attr);if(rc)return rc;
  rc=pthread_attr_setstacksize(&attr,bytes);
  if(!rc)rc=pthread_create(&thread,&attr,entry,context);
  pthread_attr_destroy(&attr);
  running=rc==0;return rc;
 }
 bool joinable()const{return running;}
 int join(){if(!running)return 0;int rc=pthread_join(thread,nullptr);if(!rc)running=false;return rc;}
 // Owner must signal stop before destruction. Never detach work using its owner.
 ~Worker(){join();}
};
}
