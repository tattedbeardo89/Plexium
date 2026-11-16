// SPDX-License-Identifier: GPL-3.0-or-later
#include "view.h"
#include "browse.h"
#include "canvas.h"
#include <cmath>
namespace plexium {

namespace {
Command command(Action a,std::string id="",std::string text=""){return {a,std::move(id),std::move(text)};}
Command details(const plex::Item &item){return command(Action::Details,item.id,item.type);}
}
// Cast/related detail rendering includes a full Item temporary. Keep that
// frame off the stack of Welcome/Profiles and the first hidden startup frames.
__attribute__((noinline)) std::optional<Command> View::details_frame(const Model&m,unsigned keys,Canvas&c){
  if(detail_focus!=m.detail.id){detail_focus=m.detail.id;row=0;column=0;detail_panel=3;detail_scroll=0;}
  if(image&&m.preferences.artwork){auto poster=image(m.detail,false);auto back=image(m.detail,true);
   if(back){c.photo(400,180,1430,470,back);c.box(400,180,1430,470,IM_COL32(10,11,18,185));}
   float height=!m.books&&(m.detail.type=="artist"||m.detail.type=="album"||m.detail.type=="track")?195:293;
   c.box(425,235,195,height,IM_COL32(31,32,49,255));if(poster)c.photo(425,235,195,height,poster);
  }else c.box(425,235,195,!m.books&&(m.detail.type=="artist"||m.detail.type=="album"||m.detail.type=="track")?195:293,IM_COL32(31,32,49,255));
  c.fit(680,235,1100,m.detail.title,43);
  std::string facts=m.detail.type;
  if(m.detail.year)facts+=" · "+std::to_string(m.detail.year);
  if(m.detail.duration)facts+=" · "+(m.detail.type=="track"?duration_label(m.detail.duration):std::to_string(m.detail.duration/60000)+" min");
  if(!m.detail.content_rating.empty())facts+=" · "+m.detail.content_rating;
  if(!m.detail.rating.empty())facts+=" · Rating "+m.detail.rating;
  c.fit(685,303,1100,facts,25,c.muted);
  std::string genres;for(const auto &g:m.detail.genres){if(!genres.empty())genres+=" · ";genres+=g;}
  std::string context=genres;
  if(m.detail.type=="episode")context=m.detail.series+" · S"+std::to_string(m.detail.season)+" E"+std::to_string(m.detail.episode);
  else if((m.detail.type=="track"||m.detail.type=="album")&&!m.detail.artist.empty())context=m.detail.artist+(genres.empty()?"":" · "+genres);
  c.fit(685,345,1100,context,24,c.muted);
  c.wrap(685,391,plex::utf8_prefix(m.detail.summary,530),1080,26);
  bool music=m.detail.type=="track"||m.detail.type=="album"||m.detail.type=="artist"||m.detail.type=="playlist";
  if(music){c.fit(680,655,1100,"Square Play · Artist Triangle Shuffle · Options+Square Add Later · Options+Left Play Next",19,c.muted);
   if((keys&(Options|Square))==(Options|Square))return command(Action::QueueAdd,"later");
   if((keys&(Options|Left))==(Options|Left))return command(Action::QueueAdd,"next");
   if(m.detail.type=="artist"&&(keys&Triangle))return command(Action::QueuePlay,"shuffle");
   if((keys&Square)&&m.detail.type!="track")return command(Action::QueuePlay);
  }
  bool container=m.detail.type=="show"||m.detail.type=="season"||m.detail.type=="artist"||m.detail.type=="album"||m.detail.type=="collection"||m.detail.type=="playlist";
  if(!container&&(keys&Square))return command(Action::Restart);
  if((m.detail.type=="movie"||m.detail.type=="episode")&&(keys&Triangle))return command(Action::MarkWatched);
  if((m.detail.type=="album"||m.detail.type=="playlist")&&(keys&Square))return command(Action::QueuePlay);
  if(keys&Down){if(row==0){row=1;column=0;}else if(detail_panel==3)++row;else if(detail_panel==1)detail_scroll+=70;}
  if(keys&Up){if(row>0){if(detail_panel==1&&detail_scroll>0)detail_scroll=std::max(0.f,detail_scroll-70);else --row;}column=0;}
  if(keys&Right)++column;if(keys&Left)--column;
  if(row==0)column=std::clamp(column,0,3);
  const std::string labels[]={container?(m.detail.type=="show"?"View seasons":m.detail.type=="season"?"View episodes":m.detail.type=="artist"?(m.books?"View books":"View albums"):m.detail.type=="collection"?"View collection":"View tracks"):m.detail.offset>0?"Resume":"Play","Full story","Cast","Related"};
  for(int i=0;i<4;++i)c.button(680+i*270,565,250,labels[i],row==0&&column==i);
  if(!container&&m.detail.media.is_array()&&!m.detail.media.empty()){
   c.text(680,625,"Version "+std::to_string(m.media_version+1)+" / "+std::to_string(m.detail.media.is_array()?m.detail.media.size():0)+" · "+plex::quality_name(m.quality),22,c.muted);
   if(!music)c.text(680,655,"L1 Version · R1 Quality · Options Tracks · Square Restart · Triangle Watched",20,c.muted);
   if(keys&Options)return command(Action::Tracks);
   if(keys&L1)return command(Action::Version);if(keys&R1)return command(Action::Quality);
  }
  if(row==0&&(keys&Cross)){if(column==0)return command(container?Action::Children:Action::Play,m.detail.id);detail_panel=column;row=1;column=0;detail_scroll=0;}
  if(detail_panel==1){
   std::string label="About";if(!m.detail.studio.empty())label+=" · "+m.detail.studio;
   for(const auto &director:m.detail.directors)label+=" · "+director;
   c.text(425,690,plex::utf8_prefix(label,120),30);
   const auto &summary=m.detail.summary;auto height=ImGui::GetFont()->CalcTextSizeA(22*c.s,1e6f,1650*c.s,summary.c_str()).y/c.s;
   detail_scroll=std::clamp(detail_scroll,0.f,std::max(0.f,height-285));
   c.d->PushClipRect(c.p(425,740),c.p(1830,1040),true);c.wrap(425,745-detail_scroll,summary.empty()?"No description was supplied by this server.":summary,1650,22);c.d->PopClipRect();
  }
  else if(detail_panel==2){
   c.text(425,690,"Cast",32);int count=int(m.detail.credits.empty()?m.detail.cast.size():m.detail.credits.size());column=std::clamp(column,0,std::max(0,count-1));int first=(column/12)*12;
   for(int i=first;i<std::min(count,first+12);++i){int at=i-first;float x=425+(at%4)*345,y=755+(at/4)*85;
    auto name=m.detail.credits.empty()?m.detail.cast[i]:m.detail.credits[i].name;
    c.box(x,y,320,72,row>0&&i==column?c.accent:IM_COL32(33,35,52,255));
    if(!m.detail.credits.empty()&&image&&m.preferences.artwork){plex::Item person;person.type="person";person.thumb=m.detail.credits[i].thumb;auto t=image(person,false);if(t)c.d->AddImage(ImTextureRef(t),c.p(x,y),c.p(x+56,y+72));}
    c.text(x+65,y+8,plex::utf8_prefix(name,23),21);
    if(!m.detail.credits.empty())c.text(x+65,y+39,plex::utf8_prefix(m.detail.credits[i].role,28),17,c.muted);
   }
   if(count==0)c.text(425,760,"No cast information was supplied by this server.",25,c.muted);

  }else{
   if(!m.detail_rails.empty()){
    row=std::clamp(row,0,int(m.detail_rails.size()));const auto &rail=m.detail_rails[row>0?row-1:0];
    column=std::clamp(column,0,std::max(0,int(rail.entries.size())-1));int first=std::max(0,column-5);
    c.text(425,690,rail.title,32);if(image&&m.preferences.artwork&&!rail.entries.empty())image(rail.entries[column],false);
    for(int i=first;i<std::min(int(rail.entries.size()),first+6);++i)c.card(425+(i-first)*230,745,rail.entries[i],row>0&&i==column);
    if(row>0&&(keys&Cross)&&!rail.entries.empty())return details(rail.entries[column]);
   }else c.text(425,720,m.busy?"Loading more information…":"No related titles were supplied by this server.",25,c.muted);
  }
 return {};
}
__attribute__((noinline)) std::optional<Command> View::guide_frame(const Model&m,unsigned keys,Canvas&c){
 const auto &guide=m.guide;
 c.text(100,155,guide.stale?"Live TV · Cached":guide.favorites_only?"Live TV · Favorites":"Live TV",50);c.text(105,220,m.server_name+" · "+m.profile_name,25,c.muted);
 c.text(1510,75,plex::guide_time(std::time(nullptr)),32);
 if((keys&(L1|R1))==(L1|R1))return command(Action::FavoriteFilter);
 if((keys&(Square|Options))==(Square|Options))return command(Action::Recordings);
 if((keys&(Square|Triangle))==(Square|Triangle))return command(Action::MiniGuide);
 if(keys&Options)return command(Action::GuidePage,"refresh");
 if(keys&Triangle)return command(Action::GuidePage,"now");
 if(keys&L1)return command(Action::GuidePage,"earlier");
 if(keys&R1)return command(Action::GuidePage,"later");
 if(guide.channels.empty()){
  c.wrap(105,360,m.busy?"Loading Plex guide…":guide.favorites_only?"No favorite channels. L1 + R1 shows all channels.":guide.configured?"No accessible channels available.":"Plex Web > Settings > Live TV & DVR > Set Up Plex DVR",1600,32);
  c.text(105,1015,"Circle  Back     Options  Refresh",24,c.muted);return {};
 }
 if(keys&Down){if(guide_row+1>=int(guide.channels.size())&&guide.offset+6<guide.total){guide_row=0;guide_program=0;return command(Action::GuidePage,"next");}++guide_row;}
 if(keys&Up){if(guide_row<=0&&guide.offset>0){guide_row=5;guide_program=0;return command(Action::GuidePage,"previous");}--guide_row;}
 guide_row=std::clamp(guide_row,0,int(guide.channels.size())-1);
 const auto &selected=guide.channels[guide_row];
 if(keys&Square)return command(Action::FavoriteChannel,selected.id,selected.dvr);
 if(keys&Right){if(guide_program+1>=int(selected.programs.size()))return command(Action::GuidePage,"later");++guide_program;}
 if(keys&Left){if(guide_program<=0)return command(Action::GuidePage,"earlier");--guide_program;}
 guide_program=std::clamp(guide_program,0,std::max(0,int(selected.programs.size())-1));
 constexpr float left=400,width=1420,top=320,height=88;
 const auto span=std::max<int64_t>(1,guide.ends-guide.begins);
 for(int i=0;i<=4;++i){auto at=guide.begins+i*span/4;c.text(left+i*width/4,275,plex::guide_time(at),23,c.muted);}
 for(size_t row=0;row<guide.channels.size();++row){
  if(guide.mini&&(int(row)<guide_row||int(row)>guide_row+2))continue;
  const auto &channel=guide.channels[row];float y=top+(guide.mini?row-size_t(guide_row):row)*height;
  c.box(100,y,285,height-5,int(row)==guide_row?c.accent:IM_COL32(33,35,52,255));
  float label_x=115;
  if(image&&m.preferences.artwork&&!channel.thumb.empty()){plex::Item logo;logo.id=channel.id;logo.type="channel";logo.thumb=channel.thumb;auto texture=image(logo,false);if(texture){c.photo(110,y+12,54,54,texture);label_x=175;}}
  c.text(label_x,y+12,plex::utf8_prefix(std::string(channel.favorite?"* ":"")+channel.number+"  "+channel.title,label_x>115?17:24),23);
  c.box(left,y,width,height-5,IM_COL32(23,25,39,255));
  if(channel.programs.empty())c.text(left+18,y+25,"Guide information unavailable",24,c.muted);
  c.d->PushClipRect(c.p(left,y),c.p(left+width,y+height-5),true);
  for(size_t index=0;index<channel.programs.size();++index){
   const auto &program=channel.programs[index];
   float x=left+width*double(std::max(program.begins,guide.begins)-guide.begins)/span;
   float end=left+width*double(std::min(program.ends,guide.ends)-guide.begins)/span;
   bool focused=int(row)==guide_row&&int(index)==guide_program;
   c.box(x+2,y,std::max(1.f,end-x-4),height-5,focused?c.accent:IM_COL32(42,44,65,255),4);
   c.d->PushClipRect(c.p(x+5,y),c.p(std::max(x+5,end-5),y+height-5),true);
   c.text(x+12,y+16,program.title.empty()?"Title unavailable":program.title,24);
   c.text(x+12,y+50,plex::guide_time(program.begins)+" - "+plex::guide_time(program.ends),19,c.muted);
   c.d->PopClipRect();
   auto progress=plex::guide_progress(program,std::time(nullptr));if(progress>0&&progress<1)c.box(x+3,y+height-10,std::max(1.f,(end-x-6)*progress),3,IM_COL32(105,240,190,255),0);
  }
  c.d->PopClipRect();
 }
 auto now=int64_t(std::time(nullptr));
 if(now>=guide.begins&&now<guide.ends){float x=left+width*double(now-guide.begins)/span;c.d->AddLine(c.p(x,310),c.p(x,top+guide.channels.size()*height),IM_COL32(255,105,125,255),2*c.s);}
 if(!selected.programs.empty()){
  const auto &program=selected.programs[guide_program];
  c.text(105,875,plex::utf8_prefix(program.title+" · "+program.series,95),26);
  c.wrap(105,912,program.summary,1700,23);
 }
 c.text(105,990,"D-pad Browse  L1/R1 Time  Triangle Now  Square Favorite  L1+R1 Favorites",20,c.muted);
 c.text(105,1020,"Square+Triangle Mini Guide  Square+Options Recordings  Options Refresh  Circle Back",20,c.muted);
 if(keys&Cross)return command(Action::TuneChannel,selected.id,selected.dvr);
 return {};
}
std::optional<Command> View::frame(const Model&m,unsigned keys,float dt){
 Canvas c;c.configure(m);c.image=m.preferences.artwork?image:decltype(image){};c.aspect=aspect;c.books=m.books;if(m.screen!=Screen::Playback)c.box(0,0,1920,1080,IM_COL32(10,11,18,255),0);
 if(m.screen!=Screen::Playback)c.d->AddRectFilledMultiColor(c.p(600,0),c.p(1920,800),IM_COL32(34,24,62,255),IM_COL32(18,30,57,255),IM_COL32(10,11,18,255),IM_COL32(10,11,18,255));
 if(m.screen==Screen::Playback)c.box(0,0,1920,1080,IM_COL32(0,0,0,0),0);
 if(m.screen!=Screen::Playback)c.brand();
 if(previous!=m.screen){focus[std::to_string(int(previous))]={row,column};auto f=focus.find(std::to_string(int(m.screen)));row=f==focus.end()?-1:f->second.first;column=f==focus.end()?0:f->second.second;sidebar=false;previous=m.screen;entry.clear();playback_idle=0;playback_controls=true;}
 if(m.screen==Screen::Library){auto identity=m.library_id+":"+m.library_tab+":"+m.library_path+":"+std::to_string(m.page_start);if(identity!=listing_focus){listing_focus=identity;row=m.library_id.empty()?0:-1;column=0;auto tabs=library_tabs(m.library_type,m.books);for(size_t i=0;i<tabs.size();++i)if(tabs[i].first==m.library_tab)column=int(i);}}
 if(m.busy&&m.screen!=Screen::Playback){c.text(1550,80,"Loading…",25,c.muted);keys&=Circle;}
 if(!m.message.empty()){float message_y=m.screen==Screen::Settings?105:m.screen==Screen::Guide?945:850;c.box(420,message_y,1400,62,IM_COL32(60,32,48,255));c.fit(447,message_y+15,1320,m.message,24);}
 if(m.preferences.clock&&m.screen!=Screen::Guide&&m.screen!=Screen::Playback)c.text(1580,80,plex::guide_time(std::time(nullptr)),25,c.muted);
 if(m.music_available&&m.screen!=Screen::Playback){
  if((keys&(Options|Right))==(Options|Right))return command(Action::Queue);
  if((keys&(Options|Down))==(Options|Down))return command(Action::MusicOpen);
  if((keys&(Options|Cross))==(Options|Cross))return command(Action::MusicPause);
  if((keys&(Options|L1))==(Options|L1))return command(Action::MusicPrevious);
  if((keys&(Options|R1))==(Options|R1))return command(Action::MusicNext);
  if(m.screen!=Screen::Queue&&m.screen!=Screen::Tracks&&!m.music_now.empty()){const auto&i=m.music_now[0];c.box(420,945,1400,95,IM_COL32(28,29,46,245));if(c.image){auto t=c.image(i,false);if(t)c.photo(430,951,78,78,t);}c.fit(525,956,1250,i.title+" · "+i.artist+(m.music_paused?" · Paused / Resume":""),25);c.text(525,994,"Options+Cross Pause · Options+L1/R1 Previous/Next · Options+Right Queue · "+duration_label(i.offset),20,c.muted);if(i.duration>0)c.box(525,1030,1250*std::clamp(double(i.offset)/i.duration,0.,1.),4,c.accent,0);if((keys&Square)&&m.screen!=Screen::Details&&m.screen!=Screen::Guide&&m.screen!=Screen::Library)return command(Action::MusicOpen);}
 }
 if(keys&Circle){if(m.screen==Screen::Notices&&!m.notice_body.empty())return command(Action::Notices);if(m.autoplay_remaining>0)return command(Action::CancelNext);if(m.screen==Screen::Playback)return command(m.detail.type=="track"?Action::MusicHome:Action::Stop);if(m.screen==Screen::Linking)return command(Action::Cancel);return command(Action::Back);}
 switch(m.screen){
 case Screen::Queue:return queue_frame(m,keys,c);
 case Screen::Welcome:
  c.text(180,326,"Your library. A bigger stage.",66);c.wrap(183,435,"A cinematic Plex experience for PlayStation 5.\nLink your own Plex account to get started.",1000,31);
  c.text(185,930,"Unofficial Plex client; not affiliated with or endorsed by Plex",22,c.muted);
  c.button(185,602,360,"Sign in to Plex",true);c.text(185,693,"Cross  Select     Circle  Back",24,c.muted);
  if(keys&Cross)return command(Action::Link);break;
 case Screen::Linking:
  c.text(180,245,"Make yourself at home.",59);c.text(184,355,"On your phone or computer, open plex.tv/link",32,c.muted);
  c.text(183,443,m.code.empty()?"Requesting code…":m.code,100);c.text(184,591,"Code expires in "+std::to_string(m.expires)+" seconds",27,c.muted);
  c.button(184,675,280,"Retry linking",true);if(keys&Cross)return command(Action::Link);break;
 case Screen::Profiles:
  if(m.profiles.empty()){c.text(180,310,"Plex authorization is required",43);c.button(180,450,1500,"Reload Plex Home profiles",true);c.text(185,550,"Cross Reload profiles · Triangle Revalidate selected server · Options Sign out",24,c.muted);if(keys&Cross)return command(Action::Home,"profiles");if(keys&Triangle)return command(Action::ReconnectServer);if(keys&Options)return command(Action::SignOut);break;}
  [[fallthrough]];
 case Screen::Servers:{
  bool profiles=m.screen==Screen::Profiles;c.text(180,242,profiles?"Who's watching?":"Choose your server",58);
  int count=int(profiles?m.profiles.size():m.servers.size());
  if(keys&Down)++column;if(keys&Up)--column;column=std::clamp(column,0,std::max(0,count-1));
  int start=std::max(0,column-4);
  for(int i=start;i<std::min(count,start+5);++i){auto label=profiles?m.profiles[i].name:m.servers[i].label;
   if(profiles){auto t=avatar?avatar(m.profiles[i].avatar):0;c.box(185,350+(i-start)*85,64,64,IM_COL32(42,40,67,255));if(t)c.d->AddImage(ImTextureRef(t),c.p(185,350+(i-start)*85),c.p(249,414+(i-start)*85));else c.text(202,368,plex::initials(label),24);}
   c.button(profiles?275:185,350+(i-start)*85,profiles?630:720,label+(profiles&&m.profiles[i].protected_user?"  · PIN":""),i==column);}
  if(count==0)c.text(185,375,"No available selections",29,c.muted);
  if((keys&Cross)&&count){if(profiles){selected_profile=m.profiles[column].id;return command(Action::Profile,selected_profile);}return command(Action::Server,m.servers[column].id);}break;}
 case Screen::Pin:
 case Screen::Search:{
  bool pin=m.screen==Screen::Pin;const std::string symbols=pin?"1234567890":"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -";
  c.text(180,225,pin?"Profile PIN":"Search your server",55);c.text(185,320,pin?std::string(entry.size(),'*'):entry,48);
  if(keys&Right)++column;if(keys&Left)--column;if(keys&Down)column+=10;if(keys&Up)column-=10;
  column=std::clamp(column,0,int(symbols.size())+1);
  for(int i=0;i<int(symbols.size());++i)c.button(185+(i%10)*105,415+(i/10)*86,85,std::string(1,symbols[i]),i==column);
  c.button(185,760,230,"Delete",column==int(symbols.size()));c.button(440,760,280,pin?"Continue":"Search",column==int(symbols.size())+1);
  if(keys&Cross){if(column<int(symbols.size())&&entry.size()<(pin?4u:100u))entry+=symbols[column];else if(column==int(symbols.size())&&!entry.empty())entry.pop_back();else if(column==int(symbols.size())+1){if(pin)return command(Action::Profile,selected_profile,entry);return command(Action::Search,"",entry);}}
  if(!pin&&(keys&Triangle)&&!entry.empty())return command(Action::Suggest,"",entry);
  if(!pin&&!m.entries.empty()){c.text(1000,330,"Results",30);int y=400;for(const auto&i:m.entries){c.text(1000,y,i.title,24);y+=40;if(y>730)break;}c.text(1000,770,"Triangle  Search suggestions",22,c.muted);}
  break;}
 case Screen::Notices:return notices_frame(m,keys,c);
 case Screen::Settings:return settings_frame(m,keys,c);
 case Screen::Recordings:{
  c.text(105,175,"Plex DVR · Recordings",48);c.text(105,250,"Read-only · recording requests and cancellation require verified permissions",24,c.muted);
  if(keys&Down)++column;if(keys&Up)--column;column=std::clamp(column,0,std::max(0,int(m.recording_rows.size())-1));int first=std::max(0,column-5);
  for(int i=first;i<std::min(int(m.recording_rows.size()),first+6);++i){c.button(105,330+(i-first)*88,1650,plex::utf8_prefix(m.recording_rows[i].title+" · "+m.recording_rows[i].summary,100),i==column);}
  if(m.recording_rows.empty())c.text(105,390,m.busy?"Loading recording schedule…":"No accessible recording rules or scheduled recordings.",27,c.muted);
  if(!m.recording_rows.empty())c.wrap(105,900,plex::utf8_prefix(m.recording_rows[column].summary,240),1600,23);
  c.text(105,1020,"D-pad Browse   Cross Open completed recording   Circle Return",23,c.muted);
  if((keys&Cross)&&!m.recording_rows.empty()&&(m.recording_rows[column].type=="movie"||m.recording_rows[column].type=="episode"))return details(m.recording_rows[column]);break;}
 case Screen::Guide:return guide_frame(m,keys,c);
 case Screen::Tracks:{
  c.text(100,180,"Audio & Subtitles",48);c.text(105,245,plex::utf8_prefix(m.detail.title,95),27,c.muted);
  int count=int(m.audio_tracks.size()+m.subtitle_tracks.size()+1);if(keys&Down)++column;if(keys&Up)--column;column=std::clamp(column,0,std::max(0,count-1));int first=std::max(0,column-6);
  for(int i=first;i<std::min(count,first+8);++i){
   std::string label,id;bool subtitle=i>=int(m.audio_tracks.size()),selected=false;
   if(i<int(m.audio_tracks.size())){const auto &track=m.audio_tracks[i];label="Audio: "+track.label;id=track.id;selected=m.tracks.audio==id;}
   else if(i==int(m.audio_tracks.size())){label="Subtitles: Off";id="off";selected=m.tracks.subtitle.empty();}
   else{const auto &track=m.subtitle_tracks[i-int(m.audio_tracks.size())-1];label="Subtitles: "+track.label;id=track.id;selected=m.tracks.subtitle==id;}
   c.button(105,320+(i-first)*70,1680,(selected?"[Selected]  ":"")+label,column==i);
   if((keys&Cross)&&column==i)return command(subtitle?Action::SelectSubtitle:Action::SelectAudio,id);
  }
  c.text(105,1015,"D-pad  Select track     Cross  Apply     Circle  Return",24,c.muted);break;
 }
 case Screen::Details:
  if(m.detail.type=="episode"&&!m.upcoming.empty()){
   c.box(680,945,1120,80,IM_COL32(42,40,67,255));c.text(700,955,"Next: "+plex::utf8_prefix(m.upcoming.front().title,55)+(m.autoplay_remaining>0?" · "+std::to_string(m.autoplay_remaining)+"s":""),27);
   c.text(700,996,"Triangle  Play next     Circle  Cancel countdown",21,c.muted);if(keys&Triangle)return command(Action::PlayNext);
  }

  return details_frame(m,keys,c);
 case Screen::Playback:{
  if(m.detail.type=="track")return music_frame(m,keys,c);
  if(keys&Options)return command(Action::PlaybackInfo);
  if((keys&(L1|R1))==(L1|R1))return command(Action::RetryPlayback);
  if((keys&(Left|Square))==(Left|Square))return command(Action::FastSeek,"back");if((keys&(Right|Square))==(Right|Square))return command(Action::FastSeek,"forward");
  if((keys&(Left|Triangle))==(Left|Triangle))return command(Action::Chapter,"previous");if((keys&(Right|Triangle))==(Right|Triangle))return command(Action::Chapter,"next");
  if((keys&(L1|Square))==(L1|Square))return command(Action::EpisodePrevious);if((keys&(R1|Square))==(R1|Square))return command(Action::EpisodeNext);
  if(m.detail.type=="channel"&&!m.live.current.id.empty()){
   if(keys&Up)return command(Action::ChannelStep,"next");if(keys&Down)return command(Action::ChannelStep,"previous");if(keys&Square)return command(Action::LastChannel);if(keys&Triangle)return command(Action::LiveTV);
   c.box(100,90,1650,150,IM_COL32(15,16,28,225));c.text(125,115,m.live.current.number+" · "+m.live.current.title,35);c.text(125,180,"Up/Down Channel   Square Last channel   Triangle Guide   Circle Stop",23,c.muted);
  }

  if(m.detail.type=="track"){
   c.box(0,0,1920,1080,IM_COL32(10,11,18,255),0);c.brand();
   if(image){auto t=image(m.detail,false);if(t)c.d->AddImage(ImTextureRef(t),c.p(180,230),c.p(660,710));}
   c.text(735,270,plex::utf8_prefix(m.detail.title,48),43);c.text(740,355,m.detail.artist,30,c.muted);c.text(740,410,m.detail.album,27,c.muted);
  }
  playback_idle+=std::max(0.f,dt);
  if(keys){playback_idle=0;playback_controls=true;}
  if(m.paused||m.buffering){playback_idle=0;playback_controls=true;}
  if(playback_idle>=4.f&&m.detail.type!="track")playback_controls=false;
  if(playback_controls){
   c.box(0,805,1920,275,IM_COL32(10,11,18,235),0);c.text(95,840,m.detail.title,40);
   c.text(95,902,"Cross  Pause / Resume     Left / Right  Seek 10s     L1  Audio     R1  Subtitles     Circle  Stop",25,c.muted);
   c.text(95,945,duration_label(m.detail.offset)+" / "+duration_label(m.detail.duration)+" · -"+duration_label(m.detail.duration-m.detail.offset),23);
   if(m.buffering)c.text(1490,945,"Buffering…",24,c.muted);
   if(m.detail.duration>0)c.box(95,1002,1720*std::clamp(double(m.detail.offset)/m.detail.duration,0.,1.),5,c.accent,0);
  }
  if(m.playback_info){c.box(100,130,1600,220,IM_COL32(15,16,28,240));c.text(130,155,std::string("Playback · ")+plex::quality_name(m.quality),30);c.text(130,210,"Version "+std::to_string(m.media_version+1)+" · Audio "+std::to_string(m.audio_tracks.size())+" · Subtitles "+std::to_string(m.subtitle_tracks.size()),25);c.text(130,265,"Square+Left/Right 60s · Triangle+Left/Right chapters · L1+R1 retry",23,c.muted);}
  for(const auto &marker:m.detail.markers)if(m.detail.offset>=marker.begins&&m.detail.offset<marker.ends){c.text(1200,990,"Triangle  Skip "+marker.type,24,c.accent);if(keys&Triangle)return command(Action::SkipMarker);break;}
  if(keys&Cross)return command(Action::Pause);if(keys&Left)return command(Action::Seek,"back");if(keys&Right)return command(Action::Seek,"forward");if(keys&L1)return command(Action::Audio);if(keys&R1)return command(Action::Subtitle);break;}
 case Screen::Home:
 case Screen::Library:{
  if(m.screen==Screen::Home&&(keys&Triangle))return command(Action::Home,"","refresh");
  if(m.screen==Screen::Library&&(keys&Triangle)&&!m.library_id.empty())return command(Action::Search,m.library_id);
  c.fit(425,205,1400,m.busy?"Loading…":!m.connection_label.empty()?m.connection_label:m.connected?"Connected to "+m.server_name:m.home_stale?"Cached library · server unavailable":"Server status pending",20,c.muted);
  int n=int(m.navigation.size());if(n==0)break;side=std::clamp(side,0,n-1);
  if(keys&L1){side=(side+n-1)%n;sidebar=true;}if(keys&R1){side=(side+1)%n;sidebar=true;}
  if(keys&Options)sidebar=!sidebar;
  if(sidebar){if(keys&Down)side=(side+1)%n;if(keys&Up)side=(side+n-1)%n;
   if(keys&Cross){const auto&id=m.navigation[side].id;if(id=="home")return command(Action::Home);if(id=="search")return command(Action::Search);if(id=="live-tv")return command(Action::LiveTV);if(id=="free-tv")return command(Action::FreeTV);if(id=="queue")return command(Action::Queue);if(id=="settings")return command(Action::Settings);if(id=="profiles")return command(Action::Home,"profiles");if(id=="servers")return command(Action::Server);return command(Action::Library,id.substr(8));}
   if(keys&Right)sidebar=false;
  }else {if(keys&Down){if(m.screen==Screen::Library&&row<0)column=0;++row;}if(keys&Up){--row;if(m.screen==Screen::Library&&row<0){auto tabs=library_tabs(m.library_type,m.books);column=0;for(size_t i=0;i<tabs.size();++i)if(tabs[i].first==m.library_tab)column=int(i);}}if(keys&Right)++column;if(keys&Left){if(column==0)sidebar=true;else --column;}}
  int navstart=std::max(0,side-8);for(int i=navstart;i<std::min(n,navstart+10);++i)c.button(70,200+(i-navstart)*67,285,m.navigation[i].label,sidebar&&side==i);
  float profile_y=m.screen==Screen::Library?110:155;
  if(avatar&&!m.profile_avatar.empty()){auto t=avatar(m.profile_avatar);if(t)c.d->AddImage(ImTextureRef(t),c.p(425,profile_y-8),c.p(465,profile_y+32));}
  c.text(m.profile_avatar.empty()?425:480,profile_y,m.profile_name+"  /  "+m.server_name,23,c.muted);
  if(m.screen==Screen::Library){
   auto tabs=library_tabs(m.library_type,m.books);bool has_tabs=!m.library_id.empty()&&m.library_tab!="category";
   c.text(425,160,m.library_title.empty()?"Your library":m.library_title,40);
   if(has_tabs){
    if(row<0)column=std::clamp(column,0,int(tabs.size())-1);
    for(size_t i=0;i<tabs.size();++i)c.button(425+i*(1380.f/tabs.size()),225,1380.f/tabs.size()-10,tabs[i].second,!sidebar&&row<0&&column==int(i));
    if(row<0&&!sidebar&&(keys&Cross))return command(Action::Library,m.library_id,tabs[column].first);
   }
   if((m.library_tab=="recommended"&&has_tabs)||m.library_tab=="results"){
    row=std::clamp(row,-1,std::max(-1,int(m.rails.size())-1));
    if(!m.rails.empty()){
     const auto &rail=m.rails[std::max(0,row)];if(row>=0)column=std::clamp(column,0,std::max(0,int(rail.entries.size())-1));
     int chosen=row<0?0:column,first=std::max(0,chosen-5);c.text(425,330,rail.title,33);
     if(image&&m.preferences.artwork&&!rail.entries.empty())image(rail.entries[chosen],false);
     for(int i=first;i<std::min(int(rail.entries.size()),first+6);++i)c.card(425+(i-first)*230,400,rail.entries[i],!sidebar&&row>=0&&i==column);
     if(std::max(0,row)+1<int(m.rails.size()))c.text(425,850,m.rails[std::max(0,row)+1].title+"  ·  Down for more",28,c.muted);
     if((keys&Cross)&&!sidebar&&row>=0&&!rail.entries.empty())return details(rail.entries[column]);
    }else c.text(425,400,m.busy?"Loading recommendations…":"No recommendations available. Choose Library above.",29,c.muted);
   }else{
    int rows=(int(m.entries.size())+5)/6;bool next=m.page_start+int(m.entries.size())<m.total,prior=m.page_start>0;
    row=std::clamp(row,has_tabs?-1:0,std::max(0,rows+((next||prior)?0:-1)));if(row>=0)column=std::clamp(column,0,row==rows&&prior&&next?1:row==rows?0:5);
    int selected=std::min(std::max(0,row)*6+column,std::max(0,int(m.entries.size())-1));
    int firstrow=std::max(0,row-1);if(row==rows&&next)firstrow=std::max(0,rows-2);
    if(image&&!m.entries.empty())image(m.entries[selected],false);
    for(int i=firstrow*6;i<std::min(int(m.entries.size()),(firstrow+2)*6);++i)c.card(425+(i%6)*230,335+(i/6-firstrow)*340,m.entries[i],!sidebar&&row>=0&&row<rows&&i==selected);
    if(m.entries.empty())c.text(425,400,m.busy?"Loading library…":"No items in this view",29,c.muted);
    if(prior)c.button(1070,1005,300,"Previous page",!sidebar&&row==rows&&column==0);
    if(next)c.button(1450,1005,350,"Next page",!sidebar&&row==rows&&column==(prior?1:0));
    if(m.total)c.text(425,1020,"Page "+std::to_string(m.page_start/60+1)+" · "+std::to_string(m.total)+" items",20,c.muted);
    if((keys&Cross)&&!sidebar&&row>=0){
     if(row==rows&&(next||prior))return command(Action::Page,prior&&column==0?"previous":"next");
     if(!m.entries.empty()){const auto &i=m.entries[selected];return i.type=="category"?command(Action::Browse,m.library_id,i.key):(i.type=="channel"?command(Action::PlayLive,i.id):details(i));}
    }
   }
  }else{
   row=std::clamp(row,-1,std::max(-1,int(m.rails.size())-1));
   if(row<0){
    const plex::Item *hero=nullptr;for(const auto&r:m.rails)if(!r.entries.empty()){hero=&r.entries.front();break;}
    if(hero&&image){auto t=image(*hero,true);if(t){c.d->AddImage(ImTextureRef(t),c.p(400,210),c.p(1830,680));c.box(400,210,1430,470,IM_COL32(10,11,18,180));}}
    c.text(425,270,hero?hero->title:"Welcome to your cinema.",54);
    c.wrap(430,365,hero?hero->summary:"Your authorized media will appear here. Choose a library from the sidebar.",1100,29);
    c.button(430,570,330,"More info",!sidebar);if(hero&&(keys&Cross)&&!sidebar)return details(*hero);
    if(!m.rails.empty()){c.text(425,700,m.rails[0].title,32);for(size_t i=0;i<std::min(size_t(6),m.rails[0].entries.size());++i)c.card(425+i*230,760,m.rails[0].entries[i],false);}
   }else{
    const auto&r=m.rails[row];column=std::clamp(column,0,std::max(0,int(r.entries.size())-1));int first=std::max(0,column-5);
    c.text(425,240,r.title,43);if(image&&!r.entries.empty())image(r.entries[column],false);for(int i=first;i<std::min(int(r.entries.size()),first+6);++i)c.card(425+(i-first)*230,340,r.entries[i],!sidebar&&i==column);
    if(row+1<int(m.rails.size()))c.text(425,785,m.rails[row+1].title,32,c.muted);
    if((keys&Cross)&&!sidebar&&!r.entries.empty())return details(r.entries[column]);
   }
  }break;}
 }
 return {};
}
}
