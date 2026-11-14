// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
typedef struct {int stereo,surround;} PlexAudioPorts;
// Called only after the audio worker and VLC callbacks have quiesced. A failed
// close retains ownership for retry; it is never reported as released.
static inline int plex_audio_ports_close(PlexAudioPorts *ports,int (*close_port)(int)){
 int ok=1;int *values[2]={&ports->stereo,&ports->surround};
 for(int i=0;i<2;++i)if(*values[i]>=0){
  if(!close_port || close_port(*values[i])<0)ok=0;else *values[i]=-1;
 }
 return ok;
}
