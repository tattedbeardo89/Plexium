// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "runtime/private_json.h"
#include "live_tv.h"
#include <set>
namespace plex {
class GuidePreferences {
 Json profiles=Json::object();std::string file,scope;bool writable=false;
 public:std::set<std::string> favorites;bool only=false;
 bool bind(const std::string &root,const std::string &identity){
  file=root+"/guide-preferences-v1.json";scope=identity;profiles=Json::object();favorites.clear();only=false;writable=false;
  if(scope.empty()||scope.size()>384||scope.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-/")!=std::string::npos)return false;
  Json data;struct stat st{};
  if(lstat(file.c_str(),&st)==0){if(!plexium::read_private_json(file,data)||number(data,"schema")!=1||!data.contains("profiles")||!data["profiles"].is_object()||data["profiles"].size()>64)return false;profiles=data["profiles"];}
  else if(errno!=ENOENT)return false;
  auto current=profiles.find(scope);
  if(current!=profiles.end()){
   auto list=current->find("favorites");if(list==current->end()||!list->is_array()||list->size()>512)return false;
   for(const auto &id:*list){if(!id.is_string())return false;auto key=id.get<std::string>();if(!valid(key))return false;favorites.insert(key);}
   only=flag(*current,"only");
  }
  writable=true;return true;
 }
 static bool valid(const std::string &id){return !id.empty()&&id.size()<=384&&id.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-.:/")==std::string::npos;}
 bool save(){
  if(!writable||favorites.size()>512||(!profiles.contains(scope)&&profiles.size()>=64))return false;
  profiles[scope]={{"favorites",favorites},{"only",only}};
  return plexium::write_private_json(file,Json{{"schema",1},{"profiles",profiles}});
 }
 bool toggle(const std::string &id){if(!valid(id)||(!favorites.count(id)&&favorites.size()==512))return false;auto old=favorites;if(!favorites.erase(id))favorites.insert(id);if(save())return true;favorites=std::move(old);return false;}
 bool filter(){only=!only;if(save())return true;only=!only;return false;}
};
}
