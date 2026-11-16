// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
namespace plexium {
// Decoder input bounds. Output contracts are explicit and separately selected;
// accepting an input does not establish native codec/device support.
inline bool valid_pcm_source(unsigned rate,unsigned channels){return rate>=8000&&rate<=384000&&channels>=1&&channels<=8;}
inline bool native_pcm_output(unsigned rate,unsigned channels){return rate==48000&&(channels==2||channels==6||channels==8);}
}
