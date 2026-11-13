// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "app/session.h"
#include <vlc/vlc.h>
#include <fontconfig/fontconfig.h>
#include <omp.h>
#include <pixman.h>
#include <atomic>
#include <vector>
namespace plexium {
// Host software decoding adapter for shared Session integration. Silent decoded
// audio is counted, not sent to host hardware. No PS5 player/driver emulation.
class HostVlcPlayer:public PlaybackPort {
 bool fonts=false;
 libvlc_instance_t *instance=nullptr;
 libvlc_media_player_t *player=nullptr;
 std::vector<unsigned char> pixels=std::vector<unsigned char>(1920*1080*4);
 static void *lock(void *opaque,void **planes){auto *self=static_cast<HostVlcPlayer*>(opaque);planes[0]=self->pixels.data();return nullptr;}
 static void display(void *opaque,void*){static_cast<HostVlcPlayer*>(opaque)->frames.fetch_add(1);}
 static void audio(void *opaque,const void*,unsigned count,int64_t){static_cast<HostVlcPlayer*>(opaque)->samples.fetch_add(count);}
 public:
 std::atomic<unsigned> frames{0},samples{0};
 HostVlcPlayer(){
  // Pixman 0.42 initializes a process-global implementation chain and has no
  // public destructor. Own it as an executable dependency, so unloading a VLC
  // plugin cannot unmap the only roots of that still-live global cache.
  if(pixman_version()<=0)return;
  fonts=FcInit();if(!fonts)return;
  const char *args[]={"--no-xlib","--no-video-title-show","--avcodec-hw=none","--quiet"};
  instance=libvlc_new(4,args);if(!instance)return;
  player=libvlc_media_player_new(instance);if(!player)return;
  libvlc_video_set_callbacks(player,lock,nullptr,display,this);
  libvlc_video_set_format(player,"RV32",1920,1080,1920*4);
  libvlc_audio_set_callbacks(player,audio,nullptr,nullptr,nullptr,nullptr,this);
  libvlc_audio_set_format(player,"S16N",48000,2);
 }
 ~HostVlcPlayer()override{if(player){libvlc_media_player_stop(player);libvlc_media_player_release(player);}if(instance)libvlc_release(instance);if(fonts)FcFini();omp_pause_resource_all(omp_pause_hard);}
 bool ready()const{return player!=nullptr;}
 bool open(const std::string &url,int64_t offset)override{
  if(!player)return false;stop();frames=0;samples=0;
  auto *media=url.find("://")==std::string::npos?libvlc_media_new_path(instance,url.c_str()):libvlc_media_new_location(instance,url.c_str());
  if(!media)return false;
  if(offset>0){std::string option=":start-time="+std::to_string(offset/1000.0);libvlc_media_add_option(media,option.c_str());}
  libvlc_media_player_set_media(player,media);libvlc_media_release(media);
  return libvlc_media_player_play(player)==0;
 }
 void stop()override{if(player){libvlc_media_player_stop(player);libvlc_media_player_set_media(player,nullptr);}}
 int64_t position()const override{return player?std::max<int64_t>(0,libvlc_media_player_get_time(player)):0;}
 void pause()override{if(player)libvlc_media_player_set_pause(player,!paused());}
 bool paused()const override{return player&&libvlc_media_player_get_state(player)==libvlc_Paused;}
 void seek(int64_t ms)override{if(player)libvlc_media_player_set_time(player,ms);}
 PlaybackObservation observe()const override{
  if(!player)return {true,false,false,false,false,true};
  auto state=libvlc_media_player_get_state(player);
  return {true,state==libvlc_Playing,state==libvlc_Opening||state==libvlc_Buffering,state==libvlc_Paused,state==libvlc_Ended,state==libvlc_Error};
 }
};
}
