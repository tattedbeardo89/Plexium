// SPDX-License-Identifier: GPL-3.0-or-later
#include "canvas.h"
namespace plexium {
namespace {Command activate(Action a,std::string id="",std::string text=""){return {a,std::move(id),std::move(text)};}}
__attribute__((noinline)) std::optional<Command> View::queue_frame(const Model&m,unsigned keys,Canvas&c){
 auto identity="queue:"+std::to_string(m.queue_offset);if(listing_focus!=identity){listing_focus=identity;column=std::max(0,m.music_cursor-int(m.queue_offset));}
 c.text(105,175,"Music queue · "+std::to_string(m.music_total)+" tracks",43);
 c.fit(105,245,1650,m.music_restored?"Saved session · select the current track to Resume":m.connection_label,24,c.muted);
 if(m.queue_rows.empty()){c.text(105,390,"Your queue is empty. Add an album, artist or track from Music.",28);return {};}
 bool moving=(keys&Square)&&(keys&(Up|Down));if(!moving){if(keys&Down)++column;if(keys&Up)--column;}
 column=std::clamp(column,0,int(m.queue_rows.size())-1);int first=std::max(0,column-5);
 for(int n=first;n<std::min(first+6,int(m.queue_rows.size()));++n){const auto&i=m.queue_rows[n];auto label=std::to_string(m.queue_offset+n+1)+". "+i.title+" · "+i.artist+" · "+duration_label(i.duration);if(int(m.queue_offset+n)==m.music_cursor)label="[Current] "+label;c.button(105,315+(n-first)*90,1650,label,n==column);}
 c.fit(105,920,1670,"Cross Play/Resume · Square Remove · Square+Up/Down Move · Triangle Play Next",23,c.muted);
 c.text(105,980,"L1/R1 Page · Options Clear queue · Circle Return",23,c.muted);
 if(keys&L1)return activate(Action::Queue,"previous");if(keys&R1)return activate(Action::Queue,"next");if(keys==Options&&!m.queue_ids.empty())return activate(Action::QueueEdit,m.queue_ids.front(),"clear");
 if(size_t(column)>=m.queue_ids.size())return {};const auto&id=m.queue_ids[size_t(column)];
 if(moving){auto at=m.queue_offset+size_t(column);auto target=(keys&Up)?(at?at-1:0):std::min(at+1,size_t(m.music_total-1));auto action=activate(Action::QueueEdit,id,(keys&Up)?"up":"down");column=int(target%60);listing_focus="queue:"+std::to_string(target/60*60);return action;}if(keys&Cross)return activate(Action::QueueEdit,id,"play");if(keys==Square)return activate(Action::QueueEdit,id,"remove");if(keys&Triangle){auto action=activate(Action::QueueEdit,id,"next");if(int(m.queue_offset+size_t(column))!=m.music_cursor){auto target=std::min(size_t(m.music_cursor+1),size_t(m.music_total-1));column=int(target%60);listing_focus="queue:"+std::to_string(target/60*60);}return action;}return {};
}
__attribute__((noinline)) std::optional<Command> View::settings_frame(const Model &m,unsigned keys,Canvas &c){
 const std::vector<std::string> sections={"Account","Server","Playback","Appearance","Live TV","System"};
 if(keys&L1){settings_section=(settings_section+5)%6;column=0;}if(keys&R1){settings_section=(settings_section+1)%6;column=0;}
 c.text(105,175,"Settings · "+sections[settings_section],45);c.text(105,245,"L1 / R1  Section",24,c.muted);c.fit(680,245,1100,m.connection_label,23,c.muted);
 std::vector<std::pair<std::string,Command>> rows;
 if(settings_section==0){c.fit(105,295,1650,m.account_label,25,c.muted);c.fit(680,185,1100,m.profile_name.empty()?"No Plex profile selected":m.profile_name,29);rows={{"Switch Plex Home user",activate(Action::Home,"profiles")},{"Sign out",activate(Action::SignOut)}};}
 else if(settings_section==1){c.fit(680,185,1100,m.server_name.empty()?"No server selected":m.server_name,29);rows={{"Select server",activate(Action::Server)},{"Verify HTTPS connection and access",activate(Action::Diagnose)},{"Reconnect selected server",activate(Action::ReconnectServer)}};}
 else if(settings_section==2){rows={{std::string("Quality: ")+plex::quality_name(m.quality),activate(Action::Quality)},{std::string("Prefer Direct Play: ")+(m.preferences.direct_play?"On":"Off"),activate(Action::Setting,"direct_play")},{"Audio language: "+m.audio_preference,activate(Action::Setting,"audio")},{"Subtitles: "+m.subtitle_preference,activate(Action::Setting,"subtitle")},{std::string("Auto-play next episode: ")+(m.autoplay?"On":"Off"),activate(Action::AutoPlay)},{std::string("Resume music after video ends: ")+(m.preferences.resume_music?"On":"Off"),activate(Action::Setting,"resume_music")},{std::string("Reconnect interrupted music: ")+(m.preferences.reconnect_music?"On":"Off"),activate(Action::Setting,"reconnect_music")}};}
 else if(settings_section==3){rows={{"Theme: "+std::string(m.preferences.theme==0?"Plexium Purple":m.preferences.theme==1?"Blue":"High Contrast"),activate(Action::Setting,"theme")},{"UI safe area: "+std::to_string(m.preferences.safe_area)+"%",activate(Action::Setting,"safe_area")},{std::string("Artwork: ")+(m.preferences.artwork?"On":"Off"),activate(Action::Setting,"artwork")},{std::string("Show time: ")+(m.preferences.clock?"On":"Off"),activate(Action::Setting,"clock")}};}
 else if(settings_section==4){rows={{"Open Plex guide",activate(Action::LiveTV)},{"Refresh Plex guide",activate(Action::LiveTV,"","refresh")},{"Read-only Plex recordings",activate(Action::Recordings)}};c.wrap(105,825,"Live tuning and recording changes remain disabled in this development build.",1650,25);}
 else {c.button(105,790,1650,"Read installed license notices",true);if(keys&Cross)return activate(Action::Notices);c.text(105,340,"Plexium "+m.app_version,32);c.wrap(105,410,"Build: "+m.build_sha+"\nSource date: "+m.build_date+"\n"+m.runtime_info,1650,25);c.fit(105,590,1650,"Metadata caches: "+std::to_string(m.cache_bytes/1024)+" KiB · Queue: "+std::to_string(m.queue_bytes/1024)+" KiB · Next track: "+std::to_string(m.prefetch_bytes/1024)+" KiB · Evictions: "+std::to_string(m.cache_evictions),23,c.muted);c.wrap(105,650,"Plexium: GPL-3.0-or-later · VLC: GPL-2.0-or-later. Complete third-party notices accompany the installed build.\nHardware hold active. Physical PS5 acceptance remains pending.",1650,25);}
 if(settings_section==2&&m.playback_state==PlaybackState::Error)rows.push_back({"Stop / retry pending playback release",activate(Action::Stop)});
 if(keys&Down)++column;if(keys&Up)--column;column=std::clamp(column,0,std::max(0,int(rows.size())-1));
 for(size_t i=0;i<rows.size();++i)c.button(105,315+float(i)*78,1650,rows[i].first,int(i)==column);
 c.text(105,1020,"D-pad Select   Cross Change / Open   Circle Return",23,c.muted);
 if((keys&Cross)&&!rows.empty())return rows[size_t(column)].second;return {};
}
__attribute__((noinline)) std::optional<Command> View::notices_frame(const Model &m,unsigned keys,Canvas &c){
 c.text(105,175,"Installed license notices",44);
 if(!m.notice_body.empty()){
  if(keys&Down)detail_scroll+=80;if(keys&Up)detail_scroll-=80;float height=ImGui::GetFont()->CalcTextSizeA(22*c.s,1e6f,1650*c.s,m.notice_body.c_str()).y/c.s;detail_scroll=std::clamp(detail_scroll,0.f,std::max(0.f,height-650));
  c.d->PushClipRect(c.p(105,290),c.p(1800,970),true);c.wrap(105,290-detail_scroll,m.notice_body,1650,22);c.d->PopClipRect();
 }else{detail_scroll=0;if(keys&Down)++column;if(keys&Up)--column;column=std::clamp(column,0,std::max(0,int(m.notice_labels.size())-1));int first=std::max(0,column-6);for(int n=first;n<std::min(int(m.notice_labels.size()),first+7);++n)c.button(105,290+(n-first)*90,1650,m.notice_labels[n],n==column);if((keys&Cross)&&!m.notice_labels.empty())return activate(Action::OpenNotice,std::to_string(column));}
 c.text(105,1020,"D-pad Scroll / Select   Cross Read   Circle Return",23,c.muted);return {};
}
__attribute__((noinline)) std::optional<Command> View::music_frame(const Model &m,unsigned keys,Canvas &c){
 c.box(0,0,1920,1080,IM_COL32(10,11,18,255),0);c.brand();
 if(image&&m.preferences.artwork){auto texture=image(m.detail,false);if(texture)c.photo(100,245,480,480,texture);}
 c.fit(650,215,1150,m.detail.title,42);c.fit(650,285,1150,m.detail.artist+" · "+m.detail.album,27,c.muted);
 c.text(650,345,"Track "+std::to_string(m.music_cursor+1)+" / "+std::to_string(m.music_total)+" · Shuffle "+(m.music_shuffle?"On":"Off")+" · Repeat "+(m.music_repeat==0?"Off":m.music_repeat==1?"All":"One"),25,c.muted);
 c.fit(650,385,1150,plex::audio_source_label(m.detail,m.media_version),22,c.muted);
 for(size_t n=0;n<m.music_preview.size()&&n<5;++n)c.text(650,425+n*65,plex::utf8_prefix(m.music_preview[n].title,65),26,n==0?c.accent:c.muted);
 if(!m.message.empty()){c.box(105,735,1695,65,IM_COL32(65,30,45,255));c.fit(125,750,1650,m.message,24);}
 c.text(105,810,duration_label(m.detail.offset)+" / "+duration_label(m.detail.duration)+" · -"+duration_label(m.detail.duration-m.detail.offset),28);
 if(m.detail.duration>0)c.box(105,875,1700*std::clamp(double(m.detail.offset)/m.detail.duration,0.,1.),6,c.accent,0);
 c.text(105,930,"Cross Pause/Resume   L1/R1 Previous/Next   Left/Right Seek   Triangle Shuffle   Square Repeat",22,c.muted);
 c.text(105,980,"Options Stop · Options+Right Queue · Options+Triangle Tracks · Circle Browse",22,c.muted);
 if((keys&(Options|Right))==(Options|Right))return activate(Action::Queue);
 if((keys&(Options|Triangle))==(Options|Triangle))return activate(Action::Tracks);
 if(keys&Cross)return activate(Action::Pause);if(keys&L1)return activate(Action::MusicPrevious);if(keys&R1)return activate(Action::MusicNext);
 if(keys&Left)return activate(Action::Seek,"back");if(keys&Right)return activate(Action::Seek,"forward");if(keys&Triangle)return activate(Action::MusicShuffle);if(keys&Square)return activate(Action::MusicRepeat);if(keys&Options)return activate(Action::Stop);return {};
}
}
