// SPDX-License-Identifier: GPL-3.0-or-later
#include "view.h"
#include "browse.h"
#include <cmath>
namespace plexium {
struct Canvas {
 std::function<ImTextureID(const plex::Item&,bool)> image;
 std::function<float(ImTextureID)> aspect;
 bool books=false;
 ImDrawList *d=ImGui::GetBackgroundDrawList();
 float s=ImGui::GetIO().DisplaySize.x/1920.f;
 ImU32 ink=IM_COL32(239,239,250,255),muted=IM_COL32(157,161,187,255),accent=IM_COL32(139,118,255,255);
 ImVec2 p(float x,float y){return {x*s,y*s};}
 void box(float x,float y,float w,float h,ImU32 color,float radius=12){d->AddRectFilled(p(x,y),p(x+w,y+h),color,radius*s);}
 void text(float x,float y,const std::string&t,float size=28,ImU32 color=0){d->AddText(ImGui::GetFont(),size*s,p(x,y),color?color:ink,t.c_str());}
 void wrap(float x,float y,const std::string&t,float width,float size=27){d->AddText(ImGui::GetFont(),size*s,p(x,y),muted,t.c_str(),nullptr,width*s);}
 void photo(float x,float y,float w,float h,ImTextureID texture){
  float ratio=aspect?aspect(texture):0;ImVec2 from{0,0},to{1,1};
  if(ratio>0){float target=w/h;if(ratio>target){from.x=(1-target/ratio)/2;to.x=1-from.x;}else{from.y=(1-ratio/target)/2;to.y=1-from.y;}}
  d->AddImage(ImTextureRef(texture),p(x,y),p(x+w,y+h),from,to);
 }
 void button(float x,float y,float w,const std::string&t,bool focused){box(x,y,w,62,focused?accent:IM_COL32(33,35,52,255));text(x+22,y+16,t,27,focused?IM_COL32(15,12,28,255):ink);}
 void brand(){
  auto shape=[&](std::initializer_list<ImVec2> points,ImU32 color){std::vector<ImVec2> scaled;for(auto point:points)scaled.push_back(p(72+point.x*52/512,62+point.y*52/512));d->AddConvexPolyFilled(scaled.data(),int(scaled.size()),color);};
  shape({{114,141},{256,76},{398,141},{398,371},{256,436},{114,371}},IM_COL32(41,36,71,255));
  shape({{151,163},{256,116},{361,163},{361,349},{256,396},{151,349}},accent);
  shape({{186,186},{256,154},{326,186},{326,326},{256,358},{186,326}},IM_COL32(12,13,24,255));
  shape({{236,205},{236,307},{303,256}},IM_COL32(215,210,255,255));
  text(143,63,"plexium",42);
 }
 void card(float x,float y,const plex::Item&i,bool selected){
  bool square=!books&&(i.type=="album"||i.type=="artist"||i.type=="track"||i.type=="playlist");
  float h=square?195:270;
  box(x,y,195,h,IM_COL32(29,30,46,255));auto texture=image?image(i,false):0;
  if(texture)photo(x,y,195,h,texture);else text(x+16,y+h*.42f,i.type=="category"?"BROWSE":"PLEXIUM",21,muted);
  if(selected)d->AddRect(p(x-5,y-5),p(x+200,y+h+5),accent,14*s,0,4*s);
  text(x,y+h+14,plex::utf8_prefix(i.title,26),23);
  std::string subtitle;
  if(i.type=="episode")subtitle="S"+std::to_string(i.season)+" · E"+std::to_string(i.episode);
  else if(!i.artist.empty())subtitle=i.artist;
  else if(i.year)subtitle=std::to_string(i.year);
  else if(i.type=="artist")subtitle=books?"Author":"Artist";
  else subtitle=i.type;
  if(i.type=="track"&&i.duration)subtitle+=" · "+std::to_string(i.duration/60000)+":"+(i.duration/1000%60<10?"0":"")+std::to_string(i.duration/1000%60);
  text(x,y+h+45,plex::utf8_prefix(subtitle,28),20,muted);
  if(i.duration>0&&i.offset>0){box(x,y+h-6,195,6,IM_COL32(55,55,70,255),0);box(x,y+h-6,195*std::clamp(double(i.offset)/i.duration,0.,1.),6,accent,0);}
 }

};
namespace {
Command command(Action a,std::string id="",std::string text=""){return {a,std::move(id),std::move(text)};}
Command details(const plex::Item &item){return command(Action::Details,item.id,item.type);}
}
// Cast/related detail rendering includes a full Item temporary. Keep that
// frame off the stack of Welcome/Profiles and the first hidden startup frames.
__attribute__((noinline)) std::optional<Command> View::details_frame(const Model&m,unsigned keys,Canvas&c){
  if(detail_focus!=m.detail.id){detail_focus=m.detail.id;row=0;column=0;detail_panel=3;detail_scroll=0;}
  if(image){auto poster=image(m.detail,false);auto back=image(m.detail,true);
   if(back){c.photo(400,180,1430,470,back);c.box(400,180,1430,470,IM_COL32(10,11,18,185));}
   float height=!m.books&&(m.detail.type=="artist"||m.detail.type=="album"||m.detail.type=="track")?195:293;
   c.box(425,235,195,height,IM_COL32(31,32,49,255));if(poster)c.photo(425,235,195,height,poster);
  }else c.box(425,235,195,293,IM_COL32(31,32,49,255));
  c.text(680,235,plex::utf8_prefix(m.detail.title,52),43);
  std::string facts=m.detail.type;
  if(m.detail.year)facts+=" · "+std::to_string(m.detail.year);
  if(m.detail.duration)facts+=" · "+std::to_string(m.detail.duration/60000)+" min";
  if(!m.detail.content_rating.empty())facts+=" · "+m.detail.content_rating;
  if(!m.detail.rating.empty())facts+=" · Rating "+m.detail.rating;
  c.text(685,303,plex::utf8_prefix(facts,100),25,c.muted);
  std::string genres;for(const auto &g:m.detail.genres){if(!genres.empty())genres+=" · ";genres+=g;}
  std::string context=genres;
  if(m.detail.type=="episode")context=m.detail.series+" · S"+std::to_string(m.detail.season)+" E"+std::to_string(m.detail.episode);
  else if((m.detail.type=="track"||m.detail.type=="album")&&!m.detail.artist.empty())context=m.detail.artist+(genres.empty()?"":" · "+genres);
  c.text(685,345,plex::utf8_prefix(context,100),24,c.muted);
  c.wrap(685,391,plex::utf8_prefix(m.detail.summary,530),1080,26);
  bool container=m.detail.type=="show"||m.detail.type=="season"||m.detail.type=="artist"||m.detail.type=="album"||m.detail.type=="collection"||m.detail.type=="playlist";
  if(keys&Down){if(row==0){row=1;column=0;}else if(detail_panel==3)++row;else if(detail_panel==1)detail_scroll+=70;}
  if(keys&Up){if(row>0){if(detail_panel==1&&detail_scroll>0)detail_scroll=std::max(0.f,detail_scroll-70);else --row;}column=0;}
  if(keys&Right)++column;if(keys&Left)--column;
  if(row==0)column=std::clamp(column,0,3);
  const std::string labels[]={container?(m.detail.type=="show"?"View seasons":m.detail.type=="season"?"View episodes":m.detail.type=="artist"?(m.books?"View books":"View albums"):m.detail.type=="collection"?"View collection":"View tracks"):m.detail.offset>0?"Resume":"Play","Full story","Cast","Related"};
  for(int i=0;i<4;++i)c.button(680+i*270,565,250,labels[i],row==0&&column==i);
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
    if(!m.detail.credits.empty()&&image){plex::Item person;person.type="person";person.thumb=m.detail.credits[i].thumb;auto t=image(person,false);if(t)c.d->AddImage(ImTextureRef(t),c.p(x,y),c.p(x+56,y+72));}
    c.text(x+65,y+8,plex::utf8_prefix(name,23),21);
    if(!m.detail.credits.empty())c.text(x+65,y+39,plex::utf8_prefix(m.detail.credits[i].role,28),17,c.muted);
   }
   if(count==0)c.text(425,760,"No cast information was supplied by this server.",25,c.muted);

  }else{
   if(!m.detail_rails.empty()){
    row=std::clamp(row,0,int(m.detail_rails.size()));const auto &rail=m.detail_rails[row>0?row-1:0];
    column=std::clamp(column,0,std::max(0,int(rail.entries.size())-1));int first=std::max(0,column-5);
    c.text(425,690,rail.title,32);if(image&&!rail.entries.empty())image(rail.entries[column],false);
    for(int i=first;i<std::min(int(rail.entries.size()),first+6);++i)c.card(425+(i-first)*230,745,rail.entries[i],row>0&&i==column);
    if(row>0&&(keys&Cross)&&!rail.entries.empty())return details(rail.entries[column]);
   }else c.text(425,720,m.busy?"Loading more information…":"No related titles were supplied by this server.",25,c.muted);
  }
 return {};
}
std::optional<Command> View::frame(const Model&m,unsigned keys,float dt){
 Canvas c;c.image=image;c.aspect=aspect;c.books=m.books;if(m.screen!=Screen::Playback)c.box(0,0,1920,1080,IM_COL32(10,11,18,255),0);
 if(m.screen!=Screen::Playback)c.d->AddRectFilledMultiColor(c.p(600,0),c.p(1920,800),IM_COL32(34,24,62,255),IM_COL32(18,30,57,255),IM_COL32(10,11,18,255),IM_COL32(10,11,18,255));
 if(m.screen==Screen::Playback)c.box(0,0,1920,1080,IM_COL32(0,0,0,0),0);
 if(m.screen!=Screen::Playback)c.brand();
 if(previous!=m.screen){focus[std::to_string(int(previous))]={row,column};auto f=focus.find(std::to_string(int(m.screen)));row=f==focus.end()?-1:f->second.first;column=f==focus.end()?0:f->second.second;sidebar=false;previous=m.screen;entry.clear();playback_idle=0;playback_controls=true;}
 if(m.screen==Screen::Library){auto identity=m.library_id+":"+m.library_tab+":"+m.library_path+":"+std::to_string(m.page_start);if(identity!=listing_focus){listing_focus=identity;row=m.library_id.empty()?0:-1;column=0;auto tabs=library_tabs(m.library_type,m.books);for(size_t i=0;i<tabs.size();++i)if(tabs[i].first==m.library_tab)column=int(i);}}
 if(m.busy&&m.screen!=Screen::Playback){c.text(1550,80,"Loading…",25,c.muted);keys&=Circle;}
 if(!m.message.empty()){c.box(420,850,1400,82,IM_COL32(60,32,48,255));c.wrap(447,865,m.message,1320,24);}
 if(keys&Circle){if(m.screen==Screen::Playback)return command(Action::Stop);if(m.screen==Screen::Linking)return command(Action::Cancel);return command(Action::Back);}
 switch(m.screen){
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
  if(!pin&&!m.entries.empty()){c.text(1000,330,"Results",30);int y=400;for(const auto&i:m.entries){c.text(1000,y,i.title,24);y+=40;if(y>730)break;}c.text(1000,770,"Results navigation pending",22,c.muted);}
  break;}
 case Screen::Settings:
  c.text(420,235,"Settings & About",55);c.text(425,340,"Plexium · "+std::string(title_id),30);
  c.text(425,800,"Unofficial Plex client; not affiliated with or endorsed by Plex",22,c.muted);
  c.wrap(425,415,"Original quality · verified HTTPS · subtitles off by default.\nThis development checkpoint requires physical PS5 acceptance.",1160,28);
  c.button(425,650,350,"Sign out",true);if(keys&Cross)return command(Action::SignOut);break;
 case Screen::Details:return details_frame(m,keys,c);
 case Screen::Playback:{
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
   c.text(95,945,std::to_string(m.detail.offset/1000)+" / "+std::to_string(m.detail.duration/1000)+" seconds",23);
   if(m.buffering)c.text(1490,945,"Buffering…",24,c.muted);
  }
  if(keys&Cross)return command(Action::Pause);if(keys&Left)return command(Action::Seek,"back");if(keys&Right)return command(Action::Seek,"forward");if(keys&L1)return command(Action::Audio);if(keys&R1)return command(Action::Subtitle);break;}
 case Screen::Home:
 case Screen::Library:{
  int n=int(m.navigation.size());if(n==0)break;side=std::clamp(side,0,n-1);
  if(keys&L1){side=(side+n-1)%n;sidebar=true;}if(keys&R1){side=(side+1)%n;sidebar=true;}
  if(keys&Options)sidebar=!sidebar;
  if(sidebar){if(keys&Down)side=(side+1)%n;if(keys&Up)side=(side+n-1)%n;
   if(keys&Cross){const auto&id=m.navigation[side].id;if(id=="home")return command(Action::Home);if(id=="search")return command(Action::Search);if(id=="free-tv")return command(Action::FreeTV);if(id=="settings")return command(Action::Settings);if(id=="profiles")return command(Action::Home,"profiles");if(id=="servers")return command(Action::Server);return command(Action::Library,id.substr(8));}
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
    for(size_t i=0;i<tabs.size();++i)c.button(425+i*235,225,220,tabs[i].second,!sidebar&&row<0&&column==int(i));
    if(row<0&&!sidebar&&(keys&Cross))return command(Action::Library,m.library_id,tabs[column].first);
   }
   if(m.library_tab=="recommended"&&has_tabs){
    row=std::clamp(row,-1,std::max(-1,int(m.rails.size())-1));
    if(!m.rails.empty()){
     const auto &rail=m.rails[std::max(0,row)];if(row>=0)column=std::clamp(column,0,std::max(0,int(rail.entries.size())-1));
     int chosen=row<0?0:column,first=std::max(0,chosen-5);c.text(425,330,rail.title,33);
     if(image&&!rail.entries.empty())image(rail.entries[chosen],false);
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
