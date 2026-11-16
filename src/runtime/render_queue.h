// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <chrono>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
namespace plexium {
// Only pump() executes media/GPU operations. close() runs on that same thread,
// rejects queued work and wakes callers before the session worker is joined.
class RenderQueue {
 mutable std::mutex mutex;
 mutable std::deque<std::packaged_task<int64_t()>> pending;
 std::atomic<bool> closed{false};
 public:
 int64_t invoke(std::function<int64_t()> fn)const {
  auto state=std::make_shared<std::atomic<int>>(0);
  std::packaged_task<int64_t()> task([this,fn=std::move(fn),state]{
   int queued=0;if(closed || !state->compare_exchange_strong(queued,1))return int64_t(-1);
   return fn();
  });
  auto result=task.get_future();
  {std::lock_guard<std::mutex> lock(mutex);if(closed || pending.size()>=8)return -1;pending.push_back(std::move(task));}
  if(result.wait_for(std::chrono::seconds(2))!=std::future_status::ready){int queued=0;if(state->compare_exchange_strong(queued,2))return -1;}
  return result.get();
 }
 void pump(){std::deque<std::packaged_task<int64_t()>> tasks;{std::lock_guard<std::mutex> lock(mutex);tasks.swap(pending);}for(auto &task:tasks)task();}
 void close(){closed=true;pump();}
 size_t size()const{std::lock_guard<std::mutex> lock(mutex);return pending.size();}
 ~RenderQueue(){close();}
};
}
