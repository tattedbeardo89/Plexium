// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "worker.h"
#include <future>
#include <memory>
#include <utility>
namespace plexium {
// One outstanding job per lane. The owner polls on the render thread. Joining
// precedes destruction of both the callback and its result, including teardown.
template<class T> class Task {
 std::unique_ptr<std::packaged_task<T()>> callback;
 std::future<T> result;
 Worker worker;
 static void *run(void *context){(*static_cast<std::packaged_task<T()>*>(context))();return nullptr;}
 public:
 static constexpr size_t stack_bytes=1024u*1024;
 bool valid()const{return result.valid();}
 template<class Function> int start(Function fn,size_t bytes=stack_bytes){
  if(valid())return EBUSY;
  callback=std::make_unique<std::packaged_task<T()>>(std::move(fn));
  auto future=callback->get_future();
  int rc=worker.start(run,callback.get(),bytes);
  if(rc){callback.reset();return rc;}
  result=std::move(future);return 0;
 }
 template<class Rep,class Period> std::future_status wait_for(const std::chrono::duration<Rep,Period>&duration)const{return result.wait_for(duration);}
 T get(){worker.join();T value=result.get();callback.reset();return value;}
};
}
