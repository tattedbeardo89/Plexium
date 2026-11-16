#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
// Execute the actual Stage 1 entry with deterministic platform/GPU failures.
#define PLEX_STARTUP_ONLY 1
#define main plexium_native_main
#include "../src/main.cc"
#undef main
#include <vector>
#include <string>
#include <iostream>
Gfx gfx{};
namespace {
std::vector<std::string> events;
int failure=0,frames=0,exit_status=-1;
double clock_now=0;
}
extern "C" {
void plat_init(){events.push_back("platform");}
const char *plat_data_dir(){return failure==6?"/nonexistent-plexium-font-fixture":"assets";}
void plat_quit(int status){exit_status=status;events.push_back("exit");}
double plat_time(){return clock_now+=.6;}
bool plat_pad_read(PadState *p){p->connected=true;p->buttons=PAD_OPTIONS|PAD_CIRCLE;return true;}
void plat_hide_splash(){}
}
bool gfx_init(){events.push_back("graphics");gfx.width=1280;gfx.height=720;gfx.image_count=2;return failure!=1;}
VkResult gfx_last_result(){return VK_ERROR_DEVICE_LOST;}
bool player_shutdown_timed_out(){return false;}
bool gfx_healthy(){return true;}
void ImGui_ImplVulkan_StopUploads(){}
bool ImGui_ImplVulkan_HasPendingUpload(){return false;}
bool gfx_shutdown(){events.push_back("graphics-wait");return failure!=5;}
bool gfx_destroy(bool){events.push_back("graphics-destroy");return true;}
VkCommandBuffer gfx_begin_frame(){return failure==3?VK_NULL_HANDLE:reinterpret_cast<VkCommandBuffer>(1);}
void gfx_begin_pass(VkCommandBuffer){}
bool gfx_end_frame(VkCommandBuffer){++frames;return failure!=4;}
bool textures_shutdown(){events.push_back("textures-stop");return true;}
void textures_stop_uploads(){}
bool textures_begin_context(){return true;}
bool player_init(){TEST_REQUIRE(false && "Stage 1 must not initialize media");return false;}
bool player_shutdown(){TEST_REQUIRE(false && "Stage 1 must not stop uninitialized media");return false;}
bool video_shutdown(){TEST_REQUIRE(false);return false;}
bool ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo*){events.push_back("ui");if(failure==2)return false;ImGui::GetIO().BackendRendererUserData=&events;return true;}
void ImGui_ImplVulkan_NewFrame(){unsigned char *pixels;int w,h;ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);}
void ImGui_ImplVulkan_RenderDrawData(ImDrawData*,VkCommandBuffer,VkPipeline){}
void ImGui_ImplVulkan_Shutdown(){events.push_back("ui-stop");ImGui::GetIO().BackendRendererUserData=nullptr;}
int main(){
 for(failure=0;failure<=6;++failure){
  events.clear();clock_now=0;frames=0;
  int status=plexium_native_main();
  TEST_REQUIRE(status==(failure==0||failure==6?0:failure==1?1:failure==2?2:4));
  TEST_REQUIRE(exit_status==status);
  TEST_REQUIRE(events.front()=="platform" && events.back()=="exit");
  if(failure>=3&&failure<=5){TEST_REQUIRE(ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData);ImGui::GetIO().BackendRendererUserData=nullptr;ImGui::DestroyContext();}
  else TEST_REQUIRE(!ImGui::GetCurrentContext());
  auto count=[&](const char *event){return std::count(events.begin(),events.end(),event);};
  TEST_REQUIRE(count("graphics-wait")==int(failure<3||failure>=5));
  TEST_REQUIRE(count("ui-stop")==int(failure==0||failure==6));
  TEST_REQUIRE(count("textures-stop")==int(failure==0||failure==6));
  TEST_REQUIRE(count("graphics-destroy")==int(failure<3||failure==6));
  if(failure==0||failure==6)TEST_REQUIRE(std::find(events.begin(),events.end(),"ui-stop")<std::find(events.begin(),events.end(),"graphics-destroy"));
 }
 std::cout<<"PASS: actual startup-only native entry, partial initialization, frame failures, exit chord, context cleanup\n";
}
