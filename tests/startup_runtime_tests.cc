// SPDX-License-Identifier: GPL-3.0-or-later
#include "runtime/worker.h"
#include "runtime/render_queue.h"
#include <cassert>
#include <thread>
#include <vector>
#include <iostream>
using namespace plexium;
int main(){
 Worker worker;std::atomic<int> calls{0};
 auto entry=[](void *p)->void*{++*static_cast<std::atomic<int>*>(p);return nullptr;};
 assert(worker.start(entry,&calls,1024)==EINVAL);
 assert(worker.start(entry,&calls,3u*1024*1024)==EINVAL);
 assert(worker.start(entry,&calls)==0);assert(worker.start(entry,&calls)==EBUSY);
 assert(worker.join()==0 && calls==1);assert(worker.join()==0);
 assert(worker.start(entry,&calls)==0);assert(worker.join()==0 && calls==2);
 RenderQueue queue;std::atomic<int> executed{0};
 auto caller=std::async(std::launch::async,[&]{return queue.invoke([&]{++executed;return int64_t(42);});});
 while(!queue.size())std::this_thread::yield();queue.pump();assert(caller.get()==42 && executed==1);
 // Saturate the eight-slot queue. The ninth request fails immediately, and
 // shutdown wakes all eight waiters without executing GPU/media callbacks.
 std::vector<std::future<int64_t>> waiting;
 for(int i=0;i<8;++i)waiting.push_back(std::async(std::launch::async,[&]{return queue.invoke([&]{++executed;return int64_t(7);});}));
 while(queue.size()!=8)std::this_thread::yield();
 assert(queue.invoke([]{return int64_t(99);})==-1);
 queue.close();for(auto &result:waiting)assert(result.get()==-1);
 assert(executed==1 && queue.size()==0);assert(queue.invoke([]{return int64_t(1);})==-1);queue.close();
 // Expired operations must never execute later when the render thread resumes.
 RenderQueue timeout;
 auto expired=std::async(std::launch::async,[&]{return timeout.invoke([&]{++executed;return int64_t(8);});});
 assert(expired.get()==-1);timeout.pump();assert(executed==1);
 std::cout<<"PASS: explicit worker stack bounds, restart/join, render queue capacity, cancellation, timeout, shutdown wakeup\n";
}
