#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "startup_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
static void wait_child(pid_t pid){int status=0;TEST_REQUIRE(waitpid(pid,&status,0)==pid);TEST_REQUIRE(WIFEXITED(status)&&WEXITSTATUS(status)==0);}
int main(void){
    int pipefd[2];TEST_REQUIRE(pipe(pipefd)==0);pid_t pid=fork();TEST_REQUIRE(pid>=0);
    if(!pid){close(pipefd[0]);TEST_REQUIRE(dup2(pipefd[1],STDERR_FILENO)>=0);close(pipefd[1]);TEST_REQUIRE(!plex_startup_log_open("/nonexistent-plexium-parent/log"));TEST_REQUIRE(fprintf(stderr,"stderr survived\n")>0);TEST_REQUIRE(fflush(stderr)==0);_exit(0);}
    close(pipefd[1]);char buf[128]={0};TEST_REQUIRE(read(pipefd[0],buf,sizeof(buf)-1)>0);close(pipefd[0]);wait_child(pid);TEST_REQUIRE(strcmp(buf,"stderr survived\n")==0);
    char path[]="/tmp/plexium-log-test-XXXXXX";int fd=mkstemp(path);TEST_REQUIRE(fd>=0);close(fd);pid=fork();TEST_REQUIRE(pid>=0);
    if(!pid){TEST_REQUIRE(plex_startup_log_open(path));TEST_REQUIRE(fprintf(stderr,"native log\n")>0);TEST_REQUIRE(fflush(stderr)==0);_exit(0);}
    wait_child(pid);fd=open(path,O_RDONLY);TEST_REQUIRE(fd>=0);memset(buf,0,sizeof(buf));TEST_REQUIRE(read(fd,buf,sizeof(buf)-1)>0);close(fd);TEST_REQUIRE(strcmp(buf,"native log\n")==0);struct stat st;TEST_REQUIRE(stat(path,&st)==0&&(st.st_mode&0777)==0600);test_posix_result(unlink(path),"unlink");
    puts("PASS: failed log open preserves stderr; successful redirect retains private file permissions");
}
