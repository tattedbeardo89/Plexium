// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "runtime/private_json.h"
#include "plex/audio.h"
namespace plexium {
// Only identifiers/state persist. Current metadata and permissions are revalidated.
class MusicStore {
 plex::Json profiles=plex::Json::object();std::string path,scope;bool writable=false;
 static bool key(const std::string&s){return !s.empty()&&s.size()<=64&&s.find_first_not_of("0123456789")==std::string::npos;}
 public:
 static constexpr size_t file_limit=2u*1024*1024;
 bool bind(const std::string &root,const std::string &identity,plex::AudioQueue &q,int64_t &position){
  q.clear();position=0;profiles=plex::Json::object();scope=identity;path=root+"/music-sessions-v1.json";writable=false;
  if(scope.empty()||scope.size()>384||scope.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-/")!=std::string::npos)return false;
  struct stat st{};plex::Json data;
  if(lstat(path.c_str(),&st)==0){if(!read_private_json(path,data,file_limit)||plex::number(data,"schema")!=1||!data.contains("profiles")||!data["profiles"].is_object()||data["profiles"].size()>16)return false;profiles=data["profiles"];}
  else if(errno!=ENOENT)return false;
  auto it=profiles.find(scope);if(it==profiles.end()){writable=true;return true;}const auto &saved=*it;
  if(!saved.is_object()||plex::text(saved,"scope")!=scope||!saved.contains("entries")||!saved["entries"].is_array()||saved["entries"].size()>plex::AudioQueue::capacity)return false;
  plex::AudioQueue restored;std::set<uint64_t> unique;
  for(const auto &entry:saved["entries"]){auto id=plex::text(entry,"id"),section=plex::text(entry,"section");auto uid=plex::number(entry,"uid");if(!key(id)||!key(section)||uid<=0||!unique.insert(uint64_t(uid)).second)return false;plex::Item item;item.id=id;item.type="track";item.section_id=section;item.title="Track "+id;if(!restored.add(item))return false;restored.identities.back()=uint64_t(uid);restored.serial=std::max(restored.serial,uint64_t(uid));}
  if(!saved.contains("order")||!saved["order"].is_array()||saved["order"].size()!=restored.entries.size())return false;
  std::vector<size_t> order;std::set<uint64_t> ordered;
  for(const auto &entry:saved["order"]){if(!entry.is_number_integer())return false;auto uid=entry.get<int64_t>();auto at=restored.locate(uint64_t(uid));if(uid<=0||at>=restored.order.size()||!ordered.insert(uint64_t(uid)).second)return false;order.push_back(restored.order[at]);}
  restored.order=std::move(order);auto current=plex::number(saved,"current");restored.cursor=restored.locate(uint64_t(current));if(!restored.entries.empty()&&restored.cursor>=restored.order.size())return false;
  auto repeat=plex::number(saved,"repeat");position=plex::number(saved,"position");if(repeat<0||repeat>2||position<0)return false;restored.repeat=static_cast<plex::Repeat>(repeat);restored.shuffled=plex::flag(saved,"shuffle");q=std::move(restored);writable=true;return true;
 }
 bool save(const plex::AudioQueue&q,int64_t position){
  if(!writable||q.entries.size()>plex::AudioQueue::capacity||q.bytes()>plex::AudioQueue::byte_limit||(!profiles.contains(scope)&&profiles.size()>=16))return false;
  auto entries=plex::Json::array(),order=plex::Json::array();
  // Unknown sections are not persisted; they cannot establish restored access.
  for(size_t n=0;n<q.entries.size();++n){const auto&i=q.entries[n];if(!key(i.id)||!key(i.section_id)||n>=q.identities.size()||q.identities[n]>INT64_MAX)return false;entries.push_back({{"id",i.id},{"section",i.section_id},{"uid",q.identities[n]}});}
  for(size_t n=0;n<q.order.size();++n)order.push_back(q.uid(n));
  auto next=profiles;next[scope]={{"scope",scope},{"entries",std::move(entries)},{"order",std::move(order)},{"current",q.uid(q.cursor)},{"position",std::max<int64_t>(0,position)},{"shuffle",q.shuffled},{"repeat",int(q.repeat)}};
  if(!write_private_json(path,plex::Json{{"schema",1},{"profiles",next}},file_limit))return false;profiles=std::move(next);return true;
 }
 void release(){profiles=plex::Json::object();path.clear();scope.clear();writable=false;}
};
}
