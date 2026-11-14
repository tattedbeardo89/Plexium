// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
namespace plex {
struct MediaTrack {std::string id,language,title,codec,label,key;int channels=0;bool subtitle=false,selected=false;};
struct NativeTrack {int id=-1;std::string name,language,codec;int channels=0;};
struct TrackSelection {std::string audio,subtitle;bool operator==(const TrackSelection&o)const{return audio==o.audio&&subtitle==o.subtitle;}};
}
