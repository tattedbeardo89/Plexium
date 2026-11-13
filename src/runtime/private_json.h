// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "plex/client.h"
#include <atomic>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
namespace plexium {
inline bool read_private_json(const std::string &path,plex::Json &value,size_t limit=262144){
 int fd=open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK);if(fd<0)return false;
 struct stat st{};if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&077)||st.st_nlink!=1||st.st_size<0||uint64_t(st.st_size)>limit){close(fd);return false;}
 std::string bytes;char buffer[4096];ssize_t n=0;
 while((n=read(fd,buffer,sizeof buffer))>0){if(bytes.size()+size_t(n)>limit){close(fd);return false;}bytes.append(buffer,size_t(n));}close(fd);if(n<0)return false;
 int depth=0;bool quoted=false,escaped=false;for(unsigned char c:bytes){if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')quoted=false;}else if(c=='"')quoted=true;else if(c=='{'||c=='['){if(++depth>24)return false;}else if(c=='}'||c==']'){if(--depth<0)return false;}}
 bool too_deep=false;value=plex::Json::parse(bytes,[&](int depth,plex::Json::parse_event_t,plex::Json&){if(depth>24)too_deep=true;return !too_deep;},false);
 return !too_deep&&!value.is_discarded()&&value.is_object();
}
inline bool write_private_json(const std::string &path,const plex::Json &value,size_t limit=262144){
 auto bytes=value.dump();if(bytes.size()>limit)return false;struct stat st{};
 if(lstat(path.c_str(),&st)==0&&(!S_ISREG(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&077)||st.st_nlink!=1))return false;
 auto temp=path+".tmp";
 int fd=open(temp.c_str(),O_WRONLY|O_CREAT|O_NOFOLLOW|O_NONBLOCK,0600);if(fd<0)return false;
 struct stat temporary{};
 if(fstat(fd,&temporary)||!S_ISREG(temporary.st_mode)||temporary.st_uid!=geteuid()||(temporary.st_mode&077)||temporary.st_nlink!=1||ftruncate(fd,0)){close(fd);return false;}
 size_t at=0;bool ok=true;while(at<bytes.size()){ssize_t n=write(fd,bytes.data()+at,bytes.size()-at);if(n<0&&errno==EINTR)continue;if(n<=0){ok=false;break;}at+=size_t(n);}
 if(fsync(fd))ok=false;if(close(fd))ok=false;
 if(ok&&rename(temp.c_str(),path.c_str())==0){auto slash=path.rfind('/');if(slash!=std::string::npos){int dir=open(path.substr(0,slash).c_str(),O_RDONLY|O_DIRECTORY);if(dir>=0){int result=fsync(dir);close(dir);return result==0;}}return true;}
 unlink(temp.c_str());return false;
}
}
