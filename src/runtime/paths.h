// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#define PLEX_STATE_ROOT "/data/Plexium"
#define PLEX_LEGACY_STATE_ROOT "/data/plexium-client"
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
static inline int plex_private_directory(const char *path){
 struct stat st;
 if(mkdir(path,0700)!=0 && errno!=EEXIST)return 0;
 return lstat(path,&st)==0 && S_ISDIR(st.st_mode) && st.st_uid==geteuid() && !(st.st_mode&077);
}
