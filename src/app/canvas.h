// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "view.h"
#include "catalog.h"
namespace plexium {
struct Canvas {
 std::function<ImTextureID(const plex::Item&,bool)> image;
 std::function<float(ImTextureID)> aspect;
 bool books=false;float origin_x=0,origin_y=0;
 void configure(const Model&m){
  float fit=std::min(ImGui::GetIO().DisplaySize.x/1920.f,ImGui::GetIO().DisplaySize.y/1080.f);s=fit*(1-2*std::clamp(m.preferences.safe_area,0,5)/100.f);origin_x=(ImGui::GetIO().DisplaySize.x-1920*s)/2;origin_y=(ImGui::GetIO().DisplaySize.y-1080*s)/2;
  if(m.preferences.theme==1)accent=IM_COL32(75,170,245,255);else if(m.preferences.theme==2){accent=IM_COL32(255,215,80,255);muted=IM_COL32(210,212,225,255);}
 }
 ImDrawList *d=ImGui::GetBackgroundDrawList();
 float s=ImGui::GetIO().DisplaySize.x/1920.f;
 ImU32 ink=IM_COL32(239,239,250,255),muted=IM_COL32(157,161,187,255),accent=IM_COL32(139,118,255,255);
 ImVec2 p(float x,float y){return {origin_x+x*s,origin_y+y*s};}
 void box(float x,float y,float w,float h,ImU32 color,float radius=12){d->AddRectFilled(p(x,y),p(x+w,y+h),color,radius*s);}
 void text(float x,float y,const std::string&t,float size=28,ImU32 color=0){d->AddText(ImGui::GetFont(),size*s,p(x,y),color?color:ink,t.c_str());}
 void fit(float x,float y,float width,const std::string&t,float size=28,ImU32 color=0){
  auto font=ImGui::GetFont();auto measure=[&](const std::string &v){return font->CalcTextSizeA(size*s,1e6f,0,v.c_str()).x;};
  if(measure(t)<=width*s){text(x,y,t,size,color);return;}
  size_t lo=0,hi=t.size();while(lo<hi){auto mid=(lo+hi+1)/2;auto candidate=plex::utf8_prefix(t,mid)+"…";if(measure(candidate)<=width*s)lo=mid;else hi=mid-1;}text(x,y,plex::utf8_prefix(t,lo)+"…",size,color);
 }
 void wrap(float x,float y,const std::string&t,float width,float size=27){d->AddText(ImGui::GetFont(),size*s,p(x,y),muted,t.c_str(),nullptr,width*s);}
 void photo(float x,float y,float w,float h,ImTextureID texture){
  float ratio=aspect?aspect(texture):0;ImVec2 from{0,0},to{1,1};
  if(ratio>0){float target=w/h;if(ratio>target){from.x=(1-target/ratio)/2;to.x=1-from.x;}else{from.y=(1-ratio/target)/2;to.y=1-from.y;}}
  d->AddImage(ImTextureRef(texture),p(x,y),p(x+w,y+h),from,to);
 }
 void button(float x,float y,float w,const std::string&t,bool focused){box(x,y,w,62,focused?accent:IM_COL32(33,35,52,255));fit(x+22,y+16,std::max(0.f,w-44),t,27,focused?IM_COL32(15,12,28,255):ink);}
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
  fit(x,y+h+14,195,i.title,23);
  std::string subtitle;
  if(i.type=="episode")subtitle="S"+std::to_string(i.season)+" · E"+std::to_string(i.episode);
  else if(!i.artist.empty())subtitle=i.artist;
  else if(i.year)subtitle=std::to_string(i.year);
  else if(i.type=="artist")subtitle=books?"Author":"Artist";
  else subtitle=i.type;
  if(i.type=="track"&&i.duration)subtitle+=" · "+std::to_string(i.duration/60000)+":"+(i.duration/1000%60<10?"0":"")+std::to_string(i.duration/1000%60);
  fit(x,y+h+45,195,subtitle,20,muted);
  if(i.watched)text(x+8,y+8,"Watched",18,ink);
  if(i.duration>0&&i.offset>0){box(x,y+h-6,195,6,IM_COL32(55,55,70,255),0);box(x,y+h-6,195*std::clamp(double(i.offset)/i.duration,0.,1.),6,accent,0);}
 }

};
}
