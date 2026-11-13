// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <atomic>
#include <cstdio>
namespace plexium {
inline std::atomic<unsigned> last_shutdown_phase{0};
enum class ShutdownStatus {NotStarted,Started,Completed,Failed,TimedOut,Skipped};
class Shutdown {
 std::array<ShutdownStatus,8> states{};
 unsigned base;
 public:
 explicit Shutdown(unsigned first=0):base(first){}
 template<class Function> ShutdownStatus step(unsigned id,const char *label,bool enabled,Function function){
  if(id>=states.size())return ShutdownStatus::Failed;
  if(states[id]!=ShutdownStatus::NotStarted)return states[id];
  if(!enabled){states[id]=ShutdownStatus::Skipped;fprintf(stderr,"shutdown: step=%u phase=%s status=skipped\n",base+id,label);return states[id];}
  states[id]=ShutdownStatus::Started;fprintf(stderr,"shutdown: step=%u phase=%s status=started\n",base+id,label);
  auto result=function();
  if(result!=ShutdownStatus::Completed && result!=ShutdownStatus::TimedOut)result=ShutdownStatus::Failed;
  if(result==ShutdownStatus::Completed)last_shutdown_phase.store(base+id+1,std::memory_order_relaxed);
  states[id]=result;fprintf(stderr,"shutdown: step=%u phase=%s status=%s\n",base+id,label,result==ShutdownStatus::Completed?"completed":result==ShutdownStatus::TimedOut?"timed-out":"failed");
  return result;
 }
 bool successful()const{for(auto s:states)if(s==ShutdownStatus::Failed||s==ShutdownStatus::TimedOut||s==ShutdownStatus::Started)return false;return true;}
};
}
