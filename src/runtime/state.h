// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "paths.h"
#include "plex/client.h"
#include <dirent.h>
namespace plexium {
inline bool migration_name(const std::string &name){
 if(name=="account.json" || name=="device.json")return true;
 size_t prefix=name.rfind("server-",0)==0?7:name.rfind("route-",0)==0?6:0;
 if(!prefix || name.size()<=prefix+5 || name.size()>160 || name.substr(name.size()-5)!=".json")return false;
 auto id=name.substr(prefix,name.size()-prefix-5);
 return id.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-")==std::string::npos;
}
// One-time copy of private credential/selection records. Leave the old directory
// intact for rollback. The marker prevents logout from reimporting old tokens.
inline bool migrate_state(const std::string &legacy,const std::string &target){
 if(!plex_private_directory(target.c_str()))return false;
 const auto marker=target+"/migration-v1.json";struct stat st{};
 if(lstat(marker.c_str(),&st)==0){std::string id,value;return plex::load_credentials(marker,id,value)&&id=="Plexium-migration-v1"&&value=="complete";}
 if(errno!=ENOENT)return false;
 if(lstat(legacy.c_str(),&st)==0){
  if(!S_ISDIR(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&077))return false;
  DIR *dir=opendir(legacy.c_str());if(!dir)return false;
  bool ok=true;size_t count=0;
  for(;;){
   errno=0;auto *entry=readdir(dir);if(!entry){if(errno)ok=false;break;}
   std::string name=entry->d_name;if(!migration_name(name))continue;
   if(++count>128){ok=false;break;}
   auto destination=target+"/"+name;
   if(lstat(destination.c_str(),&st)==0){std::string id,value;if(!plex::load_credentials(destination,id,value)){ok=false;break;}continue;}
   if(errno!=ENOENT){ok=false;break;}
   std::string id,value;
   if(!plex::load_credentials(legacy+"/"+name,id,value)||!plex::save_credentials(destination,id,value)){ok=false;break;}
  }
  closedir(dir);if(!ok)return false;
 }else if(errno!=ENOENT)return false;
 return plex::save_credentials(marker,"Plexium-migration-v1","complete");
}
}
