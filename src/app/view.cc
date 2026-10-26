// SPDX-License-Identifier: GPL-3.0-or-later
#include "view.h"
#include <cmath>
namespace plexium {
namespace {
struct Canvas {
 std::function<ImTextureID(const plex::Item&,bool)> image;
 ImDrawList *d=ImGui::GetBackgroundDrawList();
 float s=ImGui::GetIO().DisplaySize.x/1920.f;
 ImU32 ink=IM_COL32(239,239,250,255),muted=IM_COL32(157,161,187,255),accent=IM_COL32(139,118,255,255);
 ImVec2 p(float x,float y){return {x*s,y*s};}
 void box(float x,float y,float w,float h,ImU32 color,float radius=12){d->AddRectFilled(p(x,y),p(x+w,y+h),color,radius*s);}
 void text(float x,float y,const std::string&t,float size=28,ImU32 color=0){d->AddText(ImGui::GetFont(),size*s,p(x,y),color?color:ink,t.c_str());}
 void wrap(float x,float y,const std::string&t,float width,float size=27){d->AddText(ImGui::GetFont(),size*s,p(x,y),muted,t.c_str(),nullptr,width*s);}
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
  box(x,y,195,293,IM_COL32(29,30,46,255));
  // Textured art is a separate platform concern. Never substitute an episode still for its series poster.
  auto texture=image?image(i,false):0;
  if(texture)d->AddImage(ImTextureRef(texture),p(x,y),p(x+195,y+293));else text(x+22,y+106,i.type=="episode"?"EPISODE":"PLEXIUM",21,muted);
  if(selected)d->AddRect(p(x-5,y-5),p(x+200,y+298),accent,14*s,0,4*s);
  auto title=plex::utf8_prefix(i.title,25);text(x,y+310,title,23);
  if(i.type=="episode")text(x,y+342,"S"+std::to_string(i.season)+" · E"+std::to_string(i.episode),20,muted);
  else if(i.year)text(x,y+342,std::to_string(i.year),20,muted);
  if(i.duration>0&&i.offset>0){box(x,y+285,195,6,IM_COL32(55,55,70,255),0);box(x,y+285,195*std::clamp(double(i.offset)/i.duration,0.,1.),6,accent,0);}
 }
};
Command command(Action a,std::string id="",std::string text=""){return {a,std::move(id),std::move(text)};}
}
std::optional<Command> View::frame(const Model&m,unsigned keys,float dt){
 (void)dt;Canvas c;c.image=image;if(m.screen!=Screen::Playback)c.box(0,0,1920,1080,IM_COL32(10,11,18,255),0);
 if(m.screen!=Screen::Playback)c.d->AddRectFilledMultiColor(c.p(600,0),c.p(1920,800),IM_COL32(34,24,62,255),IM_COL32(18,30,57,255),IM_COL32(10,11,18,255),IM_COL32(10,11,18,255));
 if(m.screen==Screen::Playback)c.box(0,0,1920,1080,IM_COL32(0,0,0,0),0);
 if(m.screen!=Screen::Playback)c.brand();
 if(previous!=m.screen){focus[std::to_string(int(previous))]={row,column};auto f=focus.find(std::to_string(int(m.screen)));row=f==focus.end()?-1:f->second.first;column=f==focus.end()?0:f->second.second;sidebar=false;previous=m.screen;entry.clear();}
 if(m.busy){c.text(1550,80,"Loading…",25,c.muted);keys&=Circle;}
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
   c.button(185,350+(i-start)*85,720,label+(profiles&&m.profiles[i].protected_user?"  · PIN":""),i==column);}
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
 case Screen::Details:{
  c.box(425,235,230,345,IM_COL32(31,32,49,255));c.text(710,245,m.detail.title,51);
  c.text(715,325,m.detail.type+"   "+std::to_string(m.detail.year)+"   "+std::to_string(m.detail.duration/60000)+" min",25,c.muted);
  c.wrap(715,386,m.detail.summary,1020,28);
  bool container=m.detail.type=="show"||m.detail.type=="season"||m.detail.type=="artist"||m.detail.type=="album";
  c.button(715,650,410,container?"View episodes / seasons":m.detail.offset>0?"Resume":"Play",true);
  if(!m.detail.cast.empty()){c.text(425,760,"Cast",27);c.text(425,807,m.detail.cast.front(),26,c.muted);}
  if(keys&Cross)return command(container?Action::Children:Action::Play,m.detail.id);break;}
 case Screen::Playback:
  c.box(0,805,1920,275,IM_COL32(10,11,18,235),0);c.text(95,840,m.detail.title,40);
  c.text(95,902,"Cross  Pause / Resume     Left / Right  Seek 10s     L1  Audio     R1  Subtitles     Circle  Stop",25,c.muted);
  c.text(95,945,std::to_string(m.detail.offset/1000)+" / "+std::to_string(m.detail.duration/1000)+" seconds",23);
  if(keys&Cross)return command(Action::Pause);if(keys&Left)return command(Action::Seek,"back");if(keys&Right)return command(Action::Seek,"forward");if(keys&L1)return command(Action::Audio);if(keys&R1)return command(Action::Subtitle);break;
 case Screen::Home:
 case Screen::Library:{
  int n=int(m.navigation.size());if(n==0)break;side=std::clamp(side,0,n-1);
  if(keys&L1){side=(side+n-1)%n;sidebar=true;}if(keys&R1){side=(side+1)%n;sidebar=true;}
  if(keys&Options)sidebar=!sidebar;
  if(sidebar){if(keys&Down)side=(side+1)%n;if(keys&Up)side=(side+n-1)%n;
   if(keys&Cross){const auto&id=m.navigation[side].id;if(id=="home")return command(Action::Home);if(id=="search")return command(Action::Search);if(id=="settings")return command(Action::Settings);if(id=="profiles")return command(Action::Home,"profiles");if(id=="servers")return command(Action::Server);return command(Action::Library,id.substr(8));}
   if(keys&Right)sidebar=false;
  }else {if(keys&Down)++row;if(keys&Up)--row;if(keys&Right)++column;if(keys&Left){if(column==0)sidebar=true;else --column;}}
  int navstart=std::max(0,side-8);for(int i=navstart;i<std::min(n,navstart+10);++i)c.button(70,200+(i-navstart)*67,285,m.navigation[i].label,sidebar&&side==i);
  c.text(425,155,m.profile_name+"  /  "+m.server_name,23,c.muted);
  if(m.screen==Screen::Library){
   row=std::clamp(row,0,std::max(0,(int(m.entries.size())-1)/6));column=std::clamp(column,0,5);
   int selected=row*6+column;if(selected>=int(m.entries.size())){selected=std::max(0,int(m.entries.size())-1);column=selected%6;}
   int firstrow=std::max(0,row-1);c.text(425,220,"Your library",48);
   for(int i=firstrow*6;i<std::min(int(m.entries.size()),(firstrow+2)*6);++i)c.card(425+(i%6)*230,320+(i/6-firstrow)*430,m.entries[i],!sidebar&&i==selected);
   if(m.entries.empty())c.text(425,380,m.busy?"Loading library…":"No items in this library",32,c.muted);
   if((keys&Cross)&&!sidebar&&!m.entries.empty())return command(Action::Details,m.entries[selected].id);
  }else{
   row=std::clamp(row,-1,std::max(-1,int(m.rails.size())-1));
   if(row<0){
    const plex::Item *hero=nullptr;for(const auto&r:m.rails)if(!r.entries.empty()){hero=&r.entries.front();break;}
    if(hero&&image){auto t=image(*hero,true);if(t){c.d->AddImage(ImTextureRef(t),c.p(400,210),c.p(1830,680));c.box(400,210,1430,470,IM_COL32(10,11,18,180));}}
    c.text(425,270,hero?hero->title:"Welcome to your cinema.",54);
    c.wrap(430,365,hero?hero->summary:"Your authorized media will appear here. Choose a library from the sidebar.",1100,29);
    c.button(430,570,330,"More info",!sidebar);if(hero&&(keys&Cross)&&!sidebar)return command(Action::Details,hero->id);
    if(!m.rails.empty()){c.text(425,700,m.rails[0].title,32);for(size_t i=0;i<std::min(size_t(6),m.rails[0].entries.size());++i)c.card(425+i*230,760,m.rails[0].entries[i],false);}
   }else{
    const auto&r=m.rails[row];column=std::clamp(column,0,std::max(0,int(r.entries.size())-1));int first=std::max(0,column-5);
    c.text(425,240,r.title,43);for(int i=first;i<std::min(int(r.entries.size()),first+6);++i)c.card(425+(i-first)*230,340,r.entries[i],!sidebar&&i==column);
    if(row+1<int(m.rails.size()))c.text(425,785,m.rails[row+1].title,32,c.muted);
    if((keys&Cross)&&!sidebar&&!r.entries.empty())return command(Action::Details,r.entries[column].id);
   }
  }break;}
 }
 return {};
}
}
