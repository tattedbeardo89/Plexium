// SPDX-License-Identifier: GPL-3.0-or-later
// Plexium native title entry. Renderer and VLC port retain upstream attribution.
#include "platform.h"
#include "gfx.h"
#include "player.h"
#include "app/view.h"
#include "plex/artwork.h"
#include "plex/diagnostics.h"
#include "library.h"
#include "backends/imgui_impl_vulkan.h"
#include <memory>
#include <future>
#include <deque>
#include <mutex>
#include <atomic>
namespace plex {Transport ps5_transport();}
// Diagnostic ABI retained for the platform crash reporter; no legacy UI is linked.
static_assert(std::atomic<unsigned>::is_always_lock_free, "Crash checkpoints require lock-free reads");
extern "C" unsigned plex_home_last_checkpoint(){return plex::home_last.load(std::memory_order_relaxed);}
namespace {
class NativePlayer final:public plexium::PlaybackPort {
 mutable std::mutex mutex;
 mutable std::deque<std::packaged_task<int64_t()>> pending;
 // VLC control and GPU-facing state stay on the original render thread.
 int64_t invoke(std::function<int64_t()> fn) const {
  auto state=std::make_shared<std::atomic<int>>(0); // queued, executing, cancelled
  std::packaged_task<int64_t()> task([fn=std::move(fn),state]{int queued=0;if(!state->compare_exchange_strong(queued,1))return int64_t(-1);return fn();});
  auto result=task.get_future();
  {std::lock_guard<std::mutex> lock(mutex);if(pending.size()>=8)return -1;pending.push_back(std::move(task));}
  if(result.wait_for(std::chrono::seconds(2))!=std::future_status::ready){int queued=0;if(state->compare_exchange_strong(queued,2))return -1;}
  return result.get();
 }
 public:
 void pump(){std::deque<std::packaged_task<int64_t()>> tasks;{std::lock_guard<std::mutex> lock(mutex);tasks.swap(pending);}for(auto &task:tasks)task();}
 bool open(const std::string&url,int64_t offset)override {return invoke([url,offset]{bool opened=player_open(url,offset);if(opened)player_subtitles_off();return int64_t(opened);})==1;}
 void stop()override {invoke([]{player_stop();return int64_t(0);});}
 int64_t position()const override{return std::max<int64_t>(0,invoke([]{return player_time();}));}
 bool paused()const override{return invoke([]{return int64_t(player_paused());})==1;}
 void pause()override{invoke([]{player_toggle_pause();return int64_t(0);});}
 void seek(int64_t ms)override{invoke([ms]{player_seek(ms);return int64_t(0);});}
 void audio_next()override{invoke([]{auto list=player_audio_tracks();if(list.empty())return int64_t(0);auto found=std::find_if(list.begin(),list.end(),[](const auto&t){return t.id==player_audio_track();});size_t n=found==list.end()?0:(found-list.begin()+1)%list.size();player_set_audio_track(list[n].id);return int64_t(0);});}
 void subtitle_next()override{invoke([]{auto list=player_subtitle_tracks();if(list.empty())return int64_t(0);auto found=std::find_if(list.begin(),list.end(),[](const auto&t){return t.id==player_subtitle_track();});size_t n=found==list.end()?0:(found-list.begin()+1)%list.size();if(list[n].id<0)player_subtitles_off();else player_set_subtitle_track(list[n].id);return int64_t(0);});}
};
unsigned buttons(uint32_t p){using namespace plexium;unsigned out=0;const uint32_t raw[]={PAD_UP,PAD_DOWN,PAD_LEFT,PAD_RIGHT,PAD_CROSS,PAD_CIRCLE,PAD_L1,PAD_R1,PAD_OPTIONS};for(unsigned i=0;i<9;++i)if(p&raw[i])out|=1u<<i;return out;}
}
int main(){
 plat_init();if(!gfx_init())return 1;
 IMGUI_CHECKVERSION();ImGui::CreateContext();auto &io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={float(gfx.width),float(gfx.height)};
 std::string font=std::string(plat_data_dir())+"/fonts/Inter-Regular.ttf";io.Fonts->AddFontFromFileTTF(font.c_str(),32);
 ImGui_ImplVulkan_InitInfo init{};init.ApiVersion=VK_API_VERSION_1_1;init.Instance=gfx.instance;init.PhysicalDevice=gfx.gpu;init.Device=gfx.device;init.QueueFamily=gfx.queue_family;init.Queue=gfx.queue;init.DescriptorPoolSize=2048;init.MinImageCount=2;init.ImageCount=std::max(2u,gfx.image_count);init.PipelineInfoMain.RenderPass=gfx.render_pass;init.PipelineInfoMain.MSAASamples=VK_SAMPLE_COUNT_1_BIT;
 if(!ImGui_ImplVulkan_Init(&init))return 2;
 if(!player_init())return 3;
 NativePlayer player;plexium::View view;plex::Artwork artwork(plex::ps5_transport());std::string art_scope;
 {
 auto session=std::make_unique<plexium::Session>(plex::ps5_transport(),plexium::data_path,player);
 plexium::Runtime runtime(std::move(session));runtime.send({plexium::Action::Restore,{},{}});
 double last=plat_time(),poll=last,timeline=last;uint32_t previous=0;
 for(;;){
  player.pump();
  player_tick(); // Retain VLC stop/queued-open, subtitle and decoder health servicing.
  double now=plat_time();float dt=std::clamp(float(now-last),.001f,.1f);last=now;PadState pad{};plat_pad_read(&pad);
  unsigned pressed=buttons(pad.buttons&~previous);previous=pad.buttons;
  auto model=runtime.snapshot();auto access=runtime.media_context();
  if(model.screen==plexium::Screen::Playback){model.paused=player_paused();model.buffering=player_buffering();}
  if(art_scope!=access.scope){artwork.clear();art_scope=access.scope;artwork.set_scope(std::string(plexium::data_path)+"/art",access.scope);artwork.set_avatar_scope(std::string(plexium::data_path)+"/avatars",access.account_token);}
  artwork.update();
  view.image=[&](const plex::Item &item,bool landscape){return artwork.media(access.base,access.token,item,landscape);};
  view.aspect=[&](ImTextureID texture){return artwork.aspect(texture);};
  view.avatar=[&](const std::string &url){return artwork.avatar(url,access.account_token);};plat_pad_set_private(view.sensitive(model));
  if(model.screen==plexium::Screen::Linking&&now>=poll&&!model.busy){runtime.send({plexium::Action::Poll,{},{}});poll=now+2;}
  if(model.screen==plexium::Screen::Playback&&now>=timeline&&!model.busy){runtime.send({plexium::Action::Tick,{},{}});timeline=now+10;}
  if(model.screen==plexium::Screen::Playback&&player_ended()&&!model.busy)runtime.send({plexium::Action::Stop,"ended",{}});
  ImGui_ImplVulkan_NewFrame();io.DisplaySize={float(gfx.width),float(gfx.height)};io.DeltaTime=dt;ImGui::NewFrame();
  auto action=view.frame(model,pressed,dt);if(action){if(action->action==plexium::Action::Stop){action->text=std::to_string(player_time());player_stop();}runtime.send(*action);}ImGui::Render();
  auto cmd=gfx_begin_frame();if(!cmd)continue;
  video_upload(cmd);bool playing=model.screen==plexium::Screen::Playback&&model.detail.type!="track";
  video_prepare(cmd,playing?float(gfx.width):0,playing?float(gfx.height):0);gfx_begin_pass(cmd);
  if(playing){float aspect=video_picture_aspect();float w=float(gfx.width),h=aspect>0?w/aspect:float(gfx.height);if(h>gfx.height){h=float(gfx.height);w=h*aspect;}video_draw(cmd,(gfx.width-w)/2,(gfx.height-h)/2,w,h);}
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),cmd);gfx_end_frame(cmd);
  if(gfx.frame_number==3)plat_hide_splash();
 }
 }
 artwork.clear();textures_shutdown();player_shutdown();ImGui_ImplVulkan_Shutdown();ImGui::DestroyContext();gfx_shutdown();plat_quit();return 0;
}
