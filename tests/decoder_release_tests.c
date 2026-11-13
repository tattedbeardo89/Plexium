// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "../ps5/modules/ps5vdec.c"
static int delete_result,deleted,unmapped,released,failed,unmap_result,release_result;
int sceVideodec2DeleteDecoder(void *decoder){TEST_REQUIRE(decoder);++deleted;return delete_result;}
int sceKernelMunmap(void *address,size_t size){TEST_REQUIRE(address && size);++unmapped;return unmap_result;}
int sceKernelReleaseDirectMemory(int64_t start,size_t size){TEST_REQUIRE(start>=0 && size);++released;return release_result;}
void vlc_ps5_vdec_cleanup_failed(void){++failed;}
void vlc_mutex_lock(vlc_mutex_t *lock){(void)lock;}
void vlc_mutex_unlock(vlc_mutex_t *lock){(void)lock;}
int64_t mdate(void){return 0;}
int main(void){
 decoder_sys_t *sys=calloc(1,sizeof(*sys));TEST_REQUIRE(sys);
 sys->dec=(void*)1;sys->frames.addr=(void*)2;sys->frames.len=0x4000;sys->frames.start=0;
 sys->headers=malloc(32);TEST_REQUIRE(sys->headers);
 delete_result=-1;TEST_REQUIRE(!free_sys(sys));
 TEST_REQUIRE(deleted==1 && failed==1 && unmapped==0 && released==0 && sys->dec && sys->frames.addr);
 delete_result=0;TEST_REQUIRE(free_sys(sys));TEST_REQUIRE(deleted==2 && unmapped==1 && released==1);
 sys=calloc(1,sizeof(*sys));TEST_REQUIRE(sys);sys->dec=(void*)1;
 sys->frames=(Mem){.addr=(void*)2,.len=0x4000,.start=0};
 int before_delete=deleted,before_unmap=unmapped,before_release=released;
 unmap_result=-1;TEST_REQUIRE(!free_sys(sys));
 TEST_REQUIRE(!sys->dec && sys->frames.addr && released==before_release);
 unmap_result=0;release_result=-1;TEST_REQUIRE(!free_sys(sys));
 TEST_REQUIRE(!sys->frames.addr && sys->frames.len && sys->frames.start==0);
 release_result=0;TEST_REQUIRE(free_sys(sys));
 TEST_REQUIRE(deleted==before_delete+1 && unmapped==before_unmap+2 && released==before_release+2);
 puts("PASS: production decoder delete/unmap/backing release failures retain ownership, ordered release and retry without double delete/unmap");
}
