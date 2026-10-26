// SPDX-License-Identifier: GPL-3.0-or-later
// Software rasterizer for the actual shared native ImGui draw lists; no browser or mock HTML.
#include "app/view.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <cassert>
using namespace plexium;
struct Raster {
 int width=1280,height=720,tw=0,th=0;
 unsigned char *atlas=nullptr;
 std::vector<unsigned char> pixels=std::vector<unsigned char>(1280*720*3);
 Raster(){ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&atlas,&tw,&th);ImGui::GetIO().Fonts->SetTexID(1);}
 float edge(ImVec2 a,ImVec2 b,float x,float y){return (x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x);}
 void draw(ImDrawData *data){
  std::fill(pixels.begin(),pixels.end(),0);
  for(auto *list:data->CmdLists)for(const auto &cmd:list->CmdBuffer){
   if(cmd.UserCallback)continue;
   for(unsigned i=0;i+2<cmd.ElemCount;i+=3){
    auto a=list->VtxBuffer[list->IdxBuffer[cmd.IdxOffset+i]+cmd.VtxOffset];
    auto b=list->VtxBuffer[list->IdxBuffer[cmd.IdxOffset+i+1]+cmd.VtxOffset];
    auto c=list->VtxBuffer[list->IdxBuffer[cmd.IdxOffset+i+2]+cmd.VtxOffset];
    float area=edge(a.pos,b.pos,c.pos.x,c.pos.y);if(std::abs(area)<.0001f)continue;
    int x0=std::max({0,int(std::floor(std::min({a.pos.x,b.pos.x,c.pos.x}))),int(cmd.ClipRect.x)});
    int y0=std::max({0,int(std::floor(std::min({a.pos.y,b.pos.y,c.pos.y}))),int(cmd.ClipRect.y)});
    int x1=std::min({width,int(std::ceil(std::max({a.pos.x,b.pos.x,c.pos.x}))),int(cmd.ClipRect.z)});
    int y1=std::min({height,int(std::ceil(std::max({a.pos.y,b.pos.y,c.pos.y}))),int(cmd.ClipRect.w)});
    for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){
     float wa=edge(b.pos,c.pos,x+.5f,y+.5f)/area,wb=edge(c.pos,a.pos,x+.5f,y+.5f)/area,wc=1-wa-wb;
     if(wa<0||wb<0||wc<0)continue;
     int tx=std::clamp(int((wa*a.uv.x+wb*b.uv.x+wc*c.uv.x)*tw),0,tw-1);
     int ty=std::clamp(int((wa*a.uv.y+wb*b.uv.y+wc*c.uv.y)*th),0,th-1);
     auto channel=[&](int shift){return wa*((a.col>>shift)&255)+wb*((b.col>>shift)&255)+wc*((c.col>>shift)&255);};
     float alpha=channel(IM_COL32_A_SHIFT)/255.f*atlas[(ty*tw+tx)*4+3]/255.f;
     int shifts[]={IM_COL32_R_SHIFT,IM_COL32_G_SHIFT,IM_COL32_B_SHIFT};
     for(int k=0;k<3;++k){auto &p=pixels[(y*width+x)*3+k];p=static_cast<unsigned char>(p*(1-alpha)+channel(shifts[k])*alpha);}
    }
   }
  }
 }
 void save(const std::string&path){std::ofstream out(path,std::ios::binary);out<<"P6\n"<<width<<" "<<height<<"\n255\n";out.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());}
};
int main(int argc,char **argv){
 if(argc!=2)return 2;std::filesystem::create_directories(argv[1]);ImGui::CreateContext();auto &io=ImGui::GetIO();io.DisplaySize={1280,720};io.DeltaTime=1.f/60;io.IniFilename=nullptr;io.LogFilename=nullptr;
 io.Fonts->AddFontFromFileTTF("assets/fonts/Inter-Regular.ttf",24);Raster raster;
 auto frame=[&](const char*name,const Model&m,unsigned keys=0){View view;ImGui::NewFrame();view.frame(m,keys,1.f/60);ImGui::Render();raster.draw(ImGui::GetDrawData());raster.save(std::string(argv[1])+"/"+name+".ppm");};
 Model m;frame("welcome",m);m.screen=Screen::Linking;m.code="ABCD";m.expires=120;frame("linking",m);
 m.screen=Screen::Home;m.profile_name="Fixture profile";m.server_name="Fixture server";
 plex::Item section;section.id="1";section.title="Movies";section.type="movie";m.navigation=navigation({section});
 plex::Item item;item.id="1";item.type="movie";item.title="Synthetic fixture film";item.summary="Rendered from the shared native application view. This frame uses synthetic metadata; it does not demonstrate a live Plex session or PS5 playback.";item.year=2026;item.duration=7200000;item.offset=120000;
 m.rails={{"continueWatching","Continue Watching",true,{item}},{"recentlyAddedMovies","Recently Added Movies",false,{item}}};frame("home",m);
 m.screen=Screen::Library;m.entries={item,item,item,item,item,item,item};frame("library",m);m.screen=Screen::Details;m.detail=item;frame("details",m);
 m.screen=Screen::Profiles;m.profiles={{"1","","Fixture owner","",false,true,false},{"2","","Fixture child","",true,false,true}};frame("profiles",m);
 // Input behavior is exercised against the same view, including initial draw and focus movement.
 View view;Model blank;ImGui::NewFrame();auto result=view.frame(blank,Cross,1.f/60);ImGui::Render();if(!result||result->action!=Action::Link)return 3;

 Model home;home.screen=Screen::Home;home.navigation=navigation({section});auto second=item;second.id="2";home.rails={{"continueWatching","Continue Watching",true,{item,second}}};
 auto input_frame=[&](Model state,unsigned keys){ImGui::NewFrame();auto result=view.frame(state,keys,1.f/60);ImGui::Render();return result;};
 input_frame(home,Down);input_frame(home,Right);auto chosen=input_frame(home,Cross);assert(chosen&&chosen->action==Action::Details&&chosen->id=="2");
 auto detail=home;detail.screen=Screen::Details;detail.detail=second;input_frame(detail,0);auto back=input_frame(detail,Circle);assert(back&&back->action==Action::Back);
 chosen=input_frame(home,Cross);assert(chosen&&chosen->id=="2");
 input_frame(home,R1);chosen=input_frame(home,Cross);assert(chosen&&chosen->action==Action::Search);
 io.DisplaySize={3840,2160};input_frame(home,0);assert(ImGui::GetDrawData()->TotalVtxCount>0);io.DisplaySize={1280,720};
 std::cout<<"PASS: rail movement, correct item activation, details back, focus restoration, unified R1 registry, 4K draw construction\n";
 auto begin=std::chrono::steady_clock::now();for(int i=0;i<100;++i){ImGui::NewFrame();view.frame(m,0,1.f/60);ImGui::Render();}
 std::cout<<"PASS: 6 shared-native rendered frames; Cross activates sign-in; 100 frame construction iterations in "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()<<" ms (host only)\n";
 ImGui::DestroyContext();
}
