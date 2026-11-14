// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "plex/track_types.h"
#include <vlc/vlc.h>
#include <cstring>
inline std::vector<plex::NativeTrack> vlc_track_descriptors(libvlc_media_player_t *player,bool subtitle){
 std::vector<plex::NativeTrack> result;if(!player)return result;
 auto bounded=[](const char *value,size_t max){return value?std::string(value,strnlen(value,max)):std::string();};
 auto *media=libvlc_media_player_get_media(player);if(!media)return result;
 libvlc_media_track_t **tracks=nullptr;auto count=libvlc_media_tracks_get(media,&tracks);
 auto *labels=subtitle?libvlc_video_get_spu_description(player):libvlc_audio_get_track_description(player);
 for(auto *label=labels;label&&result.size()<96;label=label->p_next){
  if(label->i_id<0)continue;
  plex::NativeTrack track;track.id=label->i_id;track.name=bounded(label->psz_name,256);
  for(unsigned i=0;i<count;++i){auto *info=tracks[i];if(!info||info->i_id!=track.id||info->i_type!=(subtitle?libvlc_track_text:libvlc_track_audio))continue;
   track.language=bounded(info->psz_language,32);
   if(info->psz_description&&*info->psz_description)track.name+=" "+bounded(info->psz_description,256);
   if(info->i_type==libvlc_track_audio&&info->audio)track.channels=int(info->audio->i_channels);
   for(int shift=0;shift<32;shift+=8)track.codec+=char((info->i_codec>>shift)&255);
  }
  result.push_back(std::move(track));
 }
 if(labels)libvlc_track_description_list_release(labels);if(tracks)libvlc_media_tracks_release(tracks,count);libvlc_media_release(media);return result;
}
