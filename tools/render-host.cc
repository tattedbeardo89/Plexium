// SPDX-License-Identifier: GPL-3.0-or-later
// Software rasterizer for the actual shared native ImGui draw lists; no browser or mock HTML.
#include "app/view.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iostream>
#include "../tests/check.h"
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
 plex::Item item;item.id="1";item.type="movie";item.title="Synthetic fixture film";item.summary="Rendered from the shared native application view. This frame uses synthetic metadata; it does not demonstrate a live Plex session or PS5 playback.";item.year=2026;item.duration=7200000;item.offset=120000;item.media=plex::Json::array({{{"videoCodec","h264"},{"width",1920},{"height",1080}},{{"videoCodec","hevc"},{"width",1280},{"height",720}}});
 m.rails={{"continueWatching","Continue Watching",true,{item}},{"recentlyAddedMovies","Recently Added Movies",false,{item}}};frame("home",m);
 m.screen=Screen::Library;m.entries={item,item,item,item,item,item,item};frame("library",m);m.screen=Screen::Details;m.detail=item;frame("details",m);
 View detail_controls;
 auto detail_input=[&](unsigned keys){ImGui::NewFrame();auto action=detail_controls.frame(m,keys,.016f);ImGui::Render();return action;};
 TEST_REQUIRE(detail_input(L1)->action==Action::Version);TEST_REQUIRE(detail_input(R1)->action==Action::Quality);
 m.screen=Screen::Profiles;m.profiles={{"1","","Fixture owner","",false,true,false},{"2","","Fixture child","",true,false,true}};frame("profiles",m);
 Model recommended=m;recommended.screen=Screen::Library;recommended.navigation=navigation({section});recommended.library_id="1";recommended.library_type="movie";recommended.library_title="Fixture Movies";recommended.library_tab="recommended";recommended.rails={{"popular","Recommended for you",false,{item,item,item}}};frame("recommended",recommended);
 Model music=recommended;music.library_type="artist";music.library_title="Fixture Music";auto album=item;album.type="album";album.title="Fixture album";album.artist="Fixture artist";music.rails={{"albums","Recently Added Albums",false,{album,album}}};frame("music",music);
 Model guide_model;guide_model.screen=Screen::Guide;guide_model.server_name="Fixture server";guide_model.profile_name="Fixture profile";
 guide_model.guide.begins=1794484800;guide_model.guide.ends=1794492000;guide_model.guide.total=8;guide_model.guide.configured=true;
 for(int i=0;i<6;++i){plex::GuideChannel channel;channel.id=std::to_string(i);channel.number=std::to_string(i+1);channel.title="Fixture channel";channel.programs={{"Fixture current program","Episode title","Plex-supplied fixture description",1794483900,1794488400},{"Fixture upcoming program","","",1794488400,1794492000}};guide_model.guide.channels.push_back(channel);}
 frame("guide",guide_model);
 Model tracks=m;tracks.screen=Screen::Tracks;tracks.detail=item;
 tracks.audio_tracks={{"10","eng","","aac","English · AAC · Stereo","",2,false,true},{"11","spa","","aac","Spanish · AAC · Stereo","",2,false,false}};
 tracks.subtitle_tracks={{"20","eng","","srt","English · SRT · Embedded","",0,true,false}};tracks.tracks.audio="10";frame("tracks",tracks);
 Model settings=m;settings.screen=Screen::Settings;frame("settings",settings);
 Model next=m;next.screen=Screen::Details;next.detail=item;next.upcoming={item};next.autoplay_remaining=8;frame("next-episode",next);
 View track_controls;ImGui::NewFrame();auto track_action=track_controls.frame(tracks,Cross,.016f);ImGui::Render();TEST_REQUIRE(track_action&&track_action->action==Action::SelectAudio&&track_action->id=="10");
 Model favorite_guide=guide_model;favorite_guide.guide.favorites_only=true;favorite_guide.guide.channels[0].favorite=true;frame("guide-favorites",favorite_guide);
 Model mini_guide=guide_model;mini_guide.guide.mini=true;frame("guide-mini",mini_guide);
 Model recordings=guide_model;recordings.screen=Screen::Recordings;plex::Item recording;recording.title="Fixture scheduled series";recording.summary="Scheduled through Plex DVR";recordings.recording_rows={recording};frame("recordings",recordings);
 View guide_view;
 auto guide_input=[&](unsigned keys){ImGui::NewFrame();auto action=guide_view.frame(guide_model,keys,.016f);ImGui::Render();return action;};
 TEST_REQUIRE(guide_input(Cross)->action==Action::TuneChannel);
 TEST_REQUIRE(guide_input(Square)->action==Action::FavoriteChannel);TEST_REQUIRE(guide_input(L1|R1)->action==Action::FavoriteFilter);TEST_REQUIRE(guide_input(Square|Triangle)->action==Action::MiniGuide);TEST_REQUIRE(guide_input(Square|Options)->action==Action::Recordings);
 TEST_REQUIRE(guide_input(Triangle)->id=="now"&&guide_input(R1)->id=="later"&&guide_input(L1)->id=="earlier");
 for(int i=0;i<5;++i)guide_input(Down);TEST_REQUIRE(guide_input(Down)->id=="next");
 guide_model.guide.offset=6;TEST_REQUIRE(guide_input(Up)->id=="previous");
 guide_model.guide.channels.clear();TEST_REQUIRE(!guide_input(Cross));TEST_REQUIRE(guide_input(Options)->id=="refresh");
 io.DisplaySize={3840,2160};guide_input(0);TEST_REQUIRE(ImGui::GetDrawData()->TotalVtxCount>0);io.DisplaySize={1280,720};
 std::cout<<"PASS: actual native guide draw lists, six-channel page, page boundaries, time navigation, Jump to Now, empty guide and 4K layout\n";
 // Input behavior is exercised against the same view, including initial draw and focus movement.
 View view;Model blank;ImGui::NewFrame();auto result=view.frame(blank,Cross,1.f/60);ImGui::Render();if(!result||result->action!=Action::Link)return 3;

 Model home;home.screen=Screen::Home;home.navigation=navigation({section});auto second=item;second.id="2";home.rails={{"continueWatching","Continue Watching",true,{item,second}}};
 auto input_frame=[&](Model state,unsigned keys){ImGui::NewFrame();auto result=view.frame(state,keys,1.f/60);ImGui::Render();return result;};
 input_frame(home,Down);input_frame(home,Right);auto chosen=input_frame(home,Cross);TEST_REQUIRE(chosen&&chosen->action==Action::Details&&chosen->id=="2");
 auto detail=home;detail.screen=Screen::Details;detail.detail=second;input_frame(detail,0);auto back=input_frame(detail,Circle);TEST_REQUIRE(back&&back->action==Action::Back);
 chosen=input_frame(home,Cross);TEST_REQUIRE(chosen&&chosen->id=="2");
 input_frame(home,R1);chosen=input_frame(home,Cross);TEST_REQUIRE(chosen&&chosen->action==Action::Search);
 io.DisplaySize={3840,2160};input_frame(home,0);TEST_REQUIRE(ImGui::GetDrawData()->TotalVtxCount>0);io.DisplaySize={1280,720};
 std::cout<<"PASS: rail movement, correct item activation, details back, focus restoration, unified R1 registry, 4K draw construction\n";
 // Regressions for the reported console issues, using the actual shared view.
 View controls;Model playback;playback.screen=Screen::Playback;playback.detail=item;
 auto controls_frame=[&](unsigned keys,float dt){ImGui::NewFrame();auto action=controls.frame(playback,keys,dt);ImGui::Render();return action;};
 controls_frame(0,.016f);TEST_REQUIRE(controls.controls_visible());controls_frame(0,5.f);TEST_REQUIRE(!controls.controls_visible());TEST_REQUIRE(ImGui::GetDrawData()->TotalVtxCount==0);
 TEST_REQUIRE(controls_frame(Right,.016f)->action==Action::Seek&&controls.controls_visible());
 playback.paused=true;controls_frame(0,6.f);TEST_REQUIRE(controls.controls_visible());playback.paused=false;playback.buffering=true;controls_frame(0,6.f);TEST_REQUIRE(controls.controls_visible());
 View portraits;int avatars=0;portraits.avatar=[&](const std::string&){++avatars;return ImTextureID(0);};
 ImGui::NewFrame();portraits.frame(m,0,.016f);ImGui::Render();TEST_REQUIRE(avatars==2);
 View rich;Model metadata;metadata.screen=Screen::Details;metadata.detail=item;metadata.detail.credits={{"Actor One","Lead","/actor/1"},{"Actor Two","Supporting","/actor/2"}};
 int posters=0,backgrounds=0,cast_images=0;rich.image=[&](const plex::Item &i,bool wide){if(i.type=="person")++cast_images;else if(wide)++backgrounds;else ++posters;return ImTextureID(0);};
 auto rich_frame=[&](unsigned keys){ImGui::NewFrame();auto action=rich.frame(metadata,keys,.016f);ImGui::Render();return action;};
 rich_frame(0);TEST_REQUIRE(posters&&backgrounds);rich_frame(Right);rich_frame(Right);rich_frame(Cross);TEST_REQUIRE(cast_images>=2);
 std::cout<<"PASS: video controls auto-hide/wake/pause, native avatar callback, detail poster/backdrop and multiple cast portraits\n";
 auto begin=std::chrono::steady_clock::now();for(int i=0;i<100;++i){ImGui::NewFrame();view.frame(m,0,1.f/60);ImGui::Render();}
 std::cout<<"PASS: expanded shared-native rendered frames; Cross activates sign-in; 100 frame construction iterations in "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()<<" ms (host only)\n";
 ImGui::DestroyContext();
}
