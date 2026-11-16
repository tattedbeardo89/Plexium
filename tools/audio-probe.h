// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>
namespace plexium {
// Opt-in host PCM observations, never a speaker/device-output measurement.
// One record per decode block, bounded independently of soak duration.
struct PcmBlock {uint64_t generation;int64_t callback_us,pts_us,vlc_clock_us;unsigned rate,channels,frames,zero_frames,zero_run,leading_zeros,trailing_zeros;int16_t first_left,first_right,last_left,last_right;};
class AudioProbe {
 mutable std::mutex mutex;std::vector<PcmBlock> blocks;uint64_t generation=0;bool enabled=false;size_t dropped=0;
 public:
 static int64_t clock(){return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
 void enable(){std::lock_guard<std::mutex> g(mutex);enabled=true;blocks.reserve(8192);}
 void begin(){std::lock_guard<std::mutex> g(mutex);++generation;}
 void record(const void *pcm,unsigned frames,int64_t pts,int64_t vlc_clock,unsigned rate=48000,unsigned channels=2){
  const auto observed=clock(); // Pair clocks before sample scanning/lock delay.
  std::lock_guard<std::mutex> g(mutex);if(!enabled||!pcm||!frames||!rate||!channels||channels>8)return;
  if(blocks.size()==8192){++dropped;return;}
  const auto *p=static_cast<const int16_t*>(pcm);unsigned zeros=0,run=0,longest=0,leading=0;
  for(unsigned n=0;n<frames;++n){bool silent=true;for(unsigned c=0;c<channels;++c)if(p[n*channels+c])silent=false;if(silent){++zeros;++run;if(n==leading)++leading;longest=std::max(longest,run);}else run=0;}
  blocks.push_back({generation,observed,pts,vlc_clock,rate,channels,frames,zeros,longest,leading,run,p[0],p[channels>1?1:0],p[(frames-1)*channels],p[(frames-1)*channels+(channels>1?1:0)]});
 }
 std::vector<PcmBlock> take(){std::lock_guard<std::mutex> g(mutex);auto result=std::move(blocks);blocks.clear();blocks.reserve(8192);return result;}
 size_t lost()const{std::lock_guard<std::mutex> g(mutex);return dropped;}
};
}
