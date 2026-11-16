// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "home.h"
#include <set>
namespace plex {
inline constexpr const char *free_tv_origin="https://epg.provider.plex.tv";
inline bool protected_stream(const Json &value){
 for(const char *field:{"drm","drmType","keySystem","licenseKey"}){
  auto it=value.find(field);if(it==value.end()||it->is_null()||*it==false||*it==0||*it==""||*it=="0"||*it=="false")continue;
  return true;
 }
 return false;
}
struct FreeChannel {Item item;std::string part;};
inline std::vector<FreeChannel> free_channels(const Json &data){
 std::vector<FreeChannel> channels;std::set<std::string> seen;
 auto mc=data.find("MediaContainer");if(mc==data.end()||!mc->is_object())return channels;
 auto list=mc->find("Channel");if(list==mc->end()||!list->is_array())return channels;
 for(const auto &channel:*list){
  if(!channel.is_object()||protected_stream(channel))continue;
  auto id=text(channel,"id",number(channel,"id")>0?std::to_string(number(channel,"id")):"");
  if(id.empty()||id.size()>128||id.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-")!=std::string::npos||!seen.insert(id).second)continue;
  std::string part;auto media=channel.find("Media");
  if(media!=channel.end()&&media->is_array())for(const auto &version:*media){
   if(!version.is_object()||protected_stream(version))continue;
   auto parts=version.find("Part");if(parts==version.end()||!parts->is_array())continue;
   for(const auto &p:*parts){
    if(!p.is_object()||protected_stream(p))continue;
    auto key=text(p,"key");const std::string origin=free_tv_origin;
    if(key.rfind(origin+"/",0)==0)key.erase(0,origin.size());
    if(key.rfind("/library/parts/",0)!=0||key.size()>4096||key.find("..")!=std::string::npos||key.find_first_of("\r\n\\#")!=std::string::npos)continue;
    part=std::move(key);break;
   }
   if(!part.empty())break;
  }
  if(part.empty())continue;
  Item item;item.id=id;item.type="channel";item.title=text(channel,"title",text(channel,"callSign","Live channel")).substr(0,256);
  item.thumb=text(channel,"thumb");item.summary=text(channel,"summary",text(channel,"description")).substr(0,2048);
  channels.push_back({std::move(item),std::move(part)});if(channels.size()==500)break;
 }
 return channels;
}
}
