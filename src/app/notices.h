// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "plex/client.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <algorithm>
namespace plexium {
// Read only package resources through non-symlink relative components.
inline bool resource_text(const std::string &root,const std::string &relative,size_t limit,std::string &out,bool excerpt=false){
 out.clear();if(relative.empty()||relative.size()>512||relative.front()=='/'||relative.find("..")!=std::string::npos||relative.find_first_of("\\:\r\n")!=std::string::npos)return false;
 int parent=open(root.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(parent<0)return false;size_t start=0;
 for(;;){auto end=relative.find('/',start);auto part=relative.substr(start,end==std::string::npos?end:end-start);if(part.empty()||part=="."){close(parent);return false;}
  int fd=openat(parent,part.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|(end==std::string::npos?0:O_DIRECTORY));close(parent);if(fd<0)return false;
  if(end!=std::string::npos){parent=fd;start=end+1;continue;}
  struct stat st{};if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0||(!excerpt&&uint64_t(st.st_size)>limit)){close(fd);return false;}
  char buffer[4096];while(out.size()<limit){auto count=read(fd,buffer,std::min(sizeof buffer,limit-out.size()));if(count<0&&errno==EINTR)continue;if(count<0){close(fd);out.clear();return false;}if(!count)break;out.append(buffer,size_t(count));}close(fd);
  if(excerpt&&uint64_t(st.st_size)>limit)out+="\n[Display limit reached; full notice accompanies the installed build.]";
  for(char &c:out)if(static_cast<unsigned char>(c)<32&&c!='\n'&&c!='\t')c=' ';return true;
 }
}
struct Notice {std::string label,path;};
inline std::vector<Notice> package_notices(const std::string &root){
 std::vector<Notice> result;std::string bytes;
 if(resource_text(root,"licenses/INDEX.json",1024*1024,bytes)){
  bool deep=false;auto index=plex::Json::parse(bytes,[&](int depth,plex::Json::parse_event_t,plex::Json&){if(depth>16)deep=true;return !deep;},false);
  if(!deep&&index.is_array())for(const auto &entry:index){auto path=plex::text(entry,"notice");if(path.empty()||path.size()>512||path.front()=='/'||path.find("..")!=std::string::npos||path.find_first_of("\\:\r\n")!=std::string::npos)continue;auto label=plex::text(entry,"component")+" · "+plex::text(entry,"source_path");if(label.size()>180)label.resize(180);result.push_back({label,"licenses/"+path});if(result.size()==1000)break;}
 }
 if(result.empty())result={{"Plexium license","LICENSE"},{"Third-party notices","THIRD_PARTY.md"}};return result;
}
}
