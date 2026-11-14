// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "runtime/private_json.h"
#include "plex/home.h"
namespace plexium {
struct Preferences {int theme=0,safe_area=0;bool artwork=true,clock=true,direct_play=true;};
class PreferenceStore {
 plex::Json profiles=plex::Json::object();std::string path,scope;bool writable=false;
 public:
 Preferences value;
 bool bind(const std::string &root,const std::string &id){
  value={};profiles=plex::Json::object();path=root+"/application-preferences-v1.json";scope=id;writable=false;
  if(id.empty()||id.size()>384||id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-/")!=std::string::npos)return false;
  struct stat st{};plex::Json data;if(lstat(path.c_str(),&st)==0){if(!read_private_json(path,data)||plex::number(data,"schema")!=1||!data.contains("profiles")||!data["profiles"].is_object()||data["profiles"].size()>64)return false;profiles=data["profiles"];}
  else if(errno!=ENOENT)return false;
  auto p=profiles.find(id);if(p!=profiles.end()){
   if(!p->is_object())return false;value.theme=int(std::clamp<int64_t>(plex::number(*p,"theme"),0,2));value.safe_area=int(std::clamp<int64_t>(plex::number(*p,"safe_area"),0,5));
   value.artwork=plex::flag(*p,"artwork",true);value.clock=plex::flag(*p,"clock",true);value.direct_play=plex::flag(*p,"direct_play",true);
  }writable=true;return true;
 }
 bool save(){if(!writable||(!profiles.contains(scope)&&profiles.size()>=64))return false;profiles[scope]={{"theme",value.theme},{"safe_area",value.safe_area},{"artwork",value.artwork},{"clock",value.clock},{"direct_play",value.direct_play}};return write_private_json(path,plex::Json{{"schema",1},{"profiles",profiles}});}
};
}
