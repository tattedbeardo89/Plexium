// SPDX-License-Identifier: GPL-3.0-or-later
#include "startup_log.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
static void wait_child(pid_t pid){int status=0;assert(waitpid(pid,&status,0)==pid);assert(WIFEXITED(status)&&WEXITSTATUS(status)==0);}
int main(void){
    int pipefd[2];assert(pipe(pipefd)==0);pid_t pid=fork();assert(pid>=0);
    if(!pid){close(pipefd[0]);assert(dup2(pipefd[1],STDERR_FILENO)>=0);close(pipefd[1]);assert(!plex_startup_log_open("/nonexistent-plexium-parent/log"));assert(fprintf(stderr,"stderr survived\n")>0);assert(fflush(stderr)==0);_exit(0);}
    close(pipefd[1]);char buf[128]={0};assert(read(pipefd[0],buf,sizeof(buf)-1)>0);close(pipefd[0]);wait_child(pid);assert(strcmp(buf,"stderr survived\n")==0);
    char path[]="/tmp/plexium-log-test-XXXXXX";int fd=mkstemp(path);assert(fd>=0);close(fd);pid=fork();assert(pid>=0);
    if(!pid){assert(plex_startup_log_open(path));assert(fprintf(stderr,"native log\n")>0);assert(fflush(stderr)==0);_exit(0);}
    wait_child(pid);fd=open(path,O_RDONLY);assert(fd>=0);memset(buf,0,sizeof(buf));assert(read(fd,buf,sizeof(buf)-1)>0);close(fd);assert(strcmp(buf,"native log\n")==0);struct stat st;assert(stat(path,&st)==0&&(st.st_mode&0777)==0600);unlink(path);
    puts("PASS: failed log open preserves stderr; successful redirect retains private file permissions");
}
