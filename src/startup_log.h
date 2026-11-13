// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
/* Unlike freopen, failure must not invalidate stderr's FILE object. */
static inline int plex_startup_log_open(const char *path) {
    int fd=open(path,O_WRONLY|O_CREAT|O_APPEND|O_NOFOLLOW|O_NONBLOCK,0600);
    if(fd<0)return 0;
    struct stat st;
    if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_uid!=geteuid() || (st.st_mode&077)){close(fd);return 0;}
    int ok=dup2(fd,STDERR_FILENO)>=0;
    if(ok)(void)dup2(fd,STDOUT_FILENO);
    if(fd>STDERR_FILENO)close(fd);
    return ok;
}
