// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "progress_outbox.h"
#include "runtime/private_json.h"
namespace plex {
class ProgressJournal {
 Json records=Json::array();std::string file,scope;bool writable=true;
 public:
 bool bind(const std::string &root,const std::string &identity,ProgressOutbox &outbox,int64_t wall,double monotonic){
  scope=identity;file=root+"/progress-v1.json";outbox.entries.clear();outbox.serial=0;records=Json::array();writable=true;
  if(scope.empty()||scope.size()>384||scope.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-/")!=std::string::npos){writable=false;return false;}
  Json data;struct stat st{};
  if(lstat(file.c_str(),&st)==0){
   if(!plexium::read_private_json(file,data)||number(data,"schema")!=1||!data.contains("records")||!data["records"].is_array()||data["records"].size()>128){writable=false;return false;}
   records=data["records"];
  }else if(errno!=ENOENT){writable=false;return false;}
  std::set<std::string> seen;
  for(const auto &r:records){if(text(r,"scope")!=scope)continue;
   Item item;item.id=text(r,"id");item.type=text(r,"type");item.duration=number(r,"duration");item.key="/library/metadata/"+item.id;
   auto state=text(r,"state"),session=text(r,"session");auto updated=number(r,"updated");
   if(item.id.empty()||item.id.size()>128||item.id.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-")!=std::string::npos||session.size()>128||session.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-")!=std::string::npos||updated<=0||wall<updated||wall-updated>86400||(state!="playing"&&state!="paused"&&state!="stopped"))continue;
   if(!seen.insert(item.id).second)continue;
   outbox.put(item,"stopped",number(r,"position"),updated,session);auto &u=outbox.entries.back();
   u.baseline=std::max<int64_t>(0,number(r,"baseline"));u.baseline_viewed=std::max<int64_t>(0,number(r,"baseline_viewed"));u.recovered=true;u.attempts=int(std::clamp<int64_t>(number(r,"attempts"),0,8));
   u.due=monotonic+std::clamp<int64_t>(number(r,"retry_at",wall)-wall,0,300);u.blocked=false;
  }
  return true;
 }
 bool save(const ProgressOutbox &outbox,int64_t wall,double monotonic){
  if(file.empty()||scope.empty()||!writable)return false;
  Json next=Json::array();for(const auto &r:records)if(text(r,"scope")!=scope&&next.size()<128&&number(r,"updated")>0&&wall>=number(r,"updated")&&wall-number(r,"updated")<=86400)next.push_back(r);
  for(const auto &u:outbox.entries){if(next.size()>=128)return false;
   next.push_back({{"scope",scope},{"id",u.item.id},{"type",u.item.type},{"duration",u.item.duration},{"state",u.state},{"position",u.position},{"updated",u.updated},{"session",u.session},{"baseline",u.baseline},{"baseline_viewed",u.baseline_viewed},{"attempts",u.attempts},{"retry_at",wall+int64_t(std::max(0.,u.due-monotonic))}});
  }
  if(!plexium::write_private_json(file,Json{{"schema",1},{"records",next}}))return false;records=std::move(next);return true;
 }
 void release(){scope.clear();file.clear();records=Json::array();writable=true;}
};
// Conservative restart reconciliation: only a server position still matching
// the observed baseline may receive a delayed write. Newer/different progress
// wins; an already-applied identical update is acknowledged without replay.
enum class Reconcile {Send,AlreadyApplied,Conflict};
inline Reconcile reconcile_progress(const ProgressUpdate &u,const Item &server){
 if(server.id!=u.item.id||server.last_played>u.updated||server.watched)return Reconcile::Conflict;
 if(server.offset==u.position&&u.position!=u.baseline)return Reconcile::AlreadyApplied;
 if(server.offset!=u.baseline)return Reconcile::Conflict;
 return Reconcile::Send;
}
}
