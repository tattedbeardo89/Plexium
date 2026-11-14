// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <limits>
namespace plex {
// Surface storage lower bound only. Driver arenas/queues/bitstreams are excluded.
// This is never evidence that a hardware allocation or 4K decode can succeed.
inline bool surface_bytes(uint64_t pitch,uint64_t height,unsigned depth,unsigned frames,uint64_t budget,uint64_t &bytes){
 bytes=0;if(!pitch||!height||pitch%2||height%2||(depth!=8&&depth!=10)||!frames||frames>32)return false;
 auto factor=uint64_t(3)*frames; // Y + interleaved UV; Main10 stores 16-bit samples.
 if(pitch>std::numeric_limits<uint64_t>::max()/height)return false;auto pixels=pitch*height;
 if(pixels>std::numeric_limits<uint64_t>::max()/factor)return false;
 bytes=pixels*factor;if(depth==8)bytes/=2;
 return bytes<=budget;
}
}
