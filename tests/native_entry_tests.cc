// SPDX-License-Identifier: GPL-3.0-or-later
// Execute the actual Stage 1 entry with deterministic platform/GPU failures.
#define PLEX_STARTUP_ONLY 1
#define main plexium_native_main
#include "../src/main.cc"
#undef main
#include <cassert>
#include <vector>
#include <string>
#include <iostream>
Gfx gfx{};
namespace {
std::vector<std::string> events;
int failure=0,frames=0;
double clock_now=0;
}
extern "C" {
void plat_init(){events.push_back("platform");}
const char *plat_data_dir(){return "assets";}
void plat_quit(){events.push_back("exit");}
double plat_time(){return clock_now+=.6;}
bool plat_pad_read(PadState *p){p->connected=true;p->buttons=PAD_OPTIONS|PAD_CIRCLE;return true;}
void plat_hide_splash(){}
}
bool gfx_init(){events.push_back("graphics");gfx.width=1280;gfx.height=720;gfx.image_count=2;return failure!=1;}
bool gfx_shutdown(){events.push_back("graphics-wait");return true;}
VkCommandBuffer gfx_begin_frame(){return failure==3?VK_NULL_HANDLE:reinterpret_cast<VkCommandBuffer>(1);}
void gfx_begin_pass(VkCommandBuffer){}
bool gfx_end_frame(VkCommandBuffer){++frames;return failure!=4;}
void textures_shutdown(){events.push_back("textures-stop");}
bool player_init(){assert(false && "Stage 1 must not initialize media");return false;}
void player_shutdown(){assert(false && "Stage 1 must not stop uninitialized media");}
bool ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo*){events.push_back("ui");if(failure==2)return false;ImGui::GetIO().BackendRendererUserData=&events;return true;}
void ImGui_ImplVulkan_NewFrame(){unsigned char *pixels;int w,h;ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);}
void ImGui_ImplVulkan_RenderDrawData(ImDrawData*,VkCommandBuffer,VkPipeline){}
void ImGui_ImplVulkan_Shutdown(){events.push_back("ui-stop");ImGui::GetIO().BackendRendererUserData=nullptr;}
int main(){
 for(failure=0;failure<=4;++failure){
  events.clear();clock_now=0;frames=0;
  int status=plexium_native_main();
  assert(status==(failure==0?0:failure==1?1:failure==2?2:4));
  assert(events.front()=="platform" && events.back()=="exit");
  if(failure>=3){assert(ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData);ImGui::GetIO().BackendRendererUserData=nullptr;ImGui::DestroyContext();}
  else assert(!ImGui::GetCurrentContext());
  auto count=[&](const char *event){return std::count(events.begin(),events.end(),event);};
  assert(count("graphics-wait")==int(failure<3));
  assert(count("ui-stop")==int(failure==0));
  assert(count("textures-stop")==int(failure==0));
 }
 std::cout<<"PASS: actual startup-only native entry, partial initialization, frame failures, exit chord, context cleanup\n";
}
