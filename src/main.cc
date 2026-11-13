// SPDX-License-Identifier: GPL-3.0-or-later
// Plexium native title entry. Renderer and VLC port retain upstream attribution.
#include "platform.h"
#include "gfx.h"
#include "player.h"
#include "app/view.h"
#include "plex/artwork.h"
#include "plex/diagnostics.h"
#include "library.h"
#include "runtime/render_queue.h"
#include "runtime/shutdown.h"
#include "backends/imgui_impl_vulkan.h"
#include <memory>
#include <future>
#include <deque>
#include <mutex>
#include <atomic>
#include <cstdio>
namespace plex {Transport ps5_transport();}
// Diagnostic ABI retained for the platform crash reporter; no legacy UI is linked.
static_assert(std::atomic<unsigned>::is_always_lock_free, "Crash checkpoints require lock-free reads");
extern "C" unsigned plex_home_last_checkpoint(){return plex::home_last.load(std::memory_order_relaxed);}
namespace {
bool application_workers_retained=false;
#if !PLEX_STARTUP_ONLY
class NativePlayer final:public plexium::PlaybackPort {
 plexium::RenderQueue queue;
 public:
 void pump(){queue.pump();}
 void close(){queue.close();}
 private:
 int64_t invoke(std::function<int64_t()> fn)const{return queue.invoke(std::move(fn));}
 public:
 bool open(const std::string&url,int64_t offset)override {return invoke([url,offset]{bool opened=player_open(url,offset);if(opened)player_subtitles_off();return int64_t(opened);})==1;}
 void stop()override {invoke([]{player_stop();return int64_t(0);});}
 int64_t position()const override{return std::max<int64_t>(0,invoke([]{return player_time();}));}
 bool paused()const override{return invoke([]{return int64_t(player_paused());})==1;}
 void pause()override{invoke([]{player_toggle_pause();return int64_t(0);});}
 void seek(int64_t ms)override{invoke([ms]{player_seek(ms);return int64_t(0);});}
 void audio_next()override{invoke([]{auto list=player_audio_tracks();if(list.empty())return int64_t(0);auto found=std::find_if(list.begin(),list.end(),[](const auto&t){return t.id==player_audio_track();});size_t n=found==list.end()?0:(found-list.begin()+1)%list.size();player_set_audio_track(list[n].id);return int64_t(0);});}
 void subtitle_next()override{invoke([]{auto list=player_subtitle_tracks();if(list.empty())return int64_t(0);auto found=std::find_if(list.begin(),list.end(),[](const auto&t){return t.id==player_subtitle_track();});size_t n=found==list.end()?0:(found-list.begin()+1)%list.size();if(list[n].id<0)player_subtitles_off();else player_set_subtitle_track(list[n].id);return int64_t(0);});}
};
// Keep growing application models off the native entry thread's stack. Member
// order keeps the player alive until the worker and artwork have shut down.
struct Application {
 NativePlayer player;
 plexium::View view;
 plex::Artwork artwork{plex::ps5_transport()};
 plexium::Runtime runtime{std::make_unique<plexium::Session>(plex::ps5_transport(),plexium::data_path,player)};
 plexium::Model model;
 plexium::MediaContext access;
 std::string art_scope;
 plexium::Shutdown worker_shutdown{16};
 bool close(){
  using Status=plexium::ShutdownStatus;
  worker_shutdown.step(0,"ui-callbacks",true,[&]{view.image={};view.aspect={};view.avatar={};return Status::Completed;});
  worker_shutdown.step(1,"reject-render-uploads",true,[&]{textures_stop_uploads();ImGui_ImplVulkan_StopUploads();player.close();player_request_shutdown();return Status::Completed;});
  worker_shutdown.step(2,"session-cancel-join",true,[&]{return runtime.close()?Status::Completed:Status::TimedOut;});
  worker_shutdown.step(3,"artwork-cancel-join",true,[&]{return artwork.close()?Status::Completed:Status::TimedOut;});
  return worker_shutdown.successful();
 }
 ~Application(){close();}
};
struct ApplicationDeleter {
 void operator()(Application *app)const{
  if(app->close())delete app;
  else{application_workers_retained=true;std::fprintf(stderr,"shutdown: application worker ownership retained until process exit\n");}
 }
};
using ApplicationPtr=std::unique_ptr<Application,ApplicationDeleter>;
unsigned buttons(uint32_t p){using namespace plexium;unsigned out=0;const uint32_t raw[]={PAD_UP,PAD_DOWN,PAD_LEFT,PAD_RIGHT,PAD_CROSS,PAD_CIRCLE,PAD_L1,PAD_R1,PAD_OPTIONS,PAD_TRIANGLE};for(unsigned i=0;i<10;++i)if(p&raw[i])out|=1u<<i;return out;}
#endif
}
static std::atomic<unsigned> native_startup_phase{0};
extern "C" unsigned plex_native_startup_phase(){return native_startup_phase.load(std::memory_order_relaxed);}
extern "C" unsigned plex_native_shutdown_phase(){return plexium::last_shutdown_phase.load(std::memory_order_relaxed);}
static void startup(const char *phase){++native_startup_phase;std::fprintf(stderr,"Plexium startup: %s\n",phase);std::fflush(stderr);}
namespace {
VkResult ui_first_error=VK_SUCCESS;
#ifndef PLEX_SOURCE_DATE
#define PLEX_SOURCE_DATE "not-stamped"
#endif

void ui_result(VkResult result){if(result!=VK_SUCCESS && ui_first_error==VK_SUCCESS){ui_first_error=result;std::fprintf(stderr,"Plexium UI: first Vulkan error=%d\n",int(result));}}
// GPU completion must be established before destroying descriptors. A failed
// frame leaves uncertain driver allocations to process teardown.
struct NativeLifetime {
 bool graphics=false,context=false,backend=false,player=false,gpu_failed=false,finished=false;
 plexium::Shutdown coordinator;
 bool shutdown(){
  using Status=plexium::ShutdownStatus;
  if(finished)return !gpu_failed;finished=true;
  if(application_workers_retained)gpu_failed=true;
  coordinator.step(0,"reject-uploads",true,[&]{textures_stop_uploads();if(backend)ImGui_ImplVulkan_StopUploads();return Status::Completed;});
  auto media=coordinator.step(1,"media-callbacks-audio",player&&!application_workers_retained,[&]{return player_shutdown()?Status::Completed:player_shutdown_timed_out()?Status::TimedOut:Status::Failed;});
  if(media==Status::Failed||media==Status::TimedOut)gpu_failed=true;
  auto queue=coordinator.step(2,"gpu-fence-drain",!gpu_failed&&graphics,[&]{return gfx_shutdown()?Status::Completed:gfx_last_result()==VK_TIMEOUT?Status::TimedOut:Status::Failed;});
  if(queue==Status::Failed||queue==Status::TimedOut)gpu_failed=true;
  auto video=coordinator.step(3,"video-objects",!gpu_failed&&player,[&]{return video_shutdown()?Status::Completed:Status::Failed;});
  if(video==Status::Failed)gpu_failed=true;
  auto ui=coordinator.step(4,"textures-and-font",!gpu_failed&&backend,[&]{
   if(!textures_shutdown())return Status::Failed;
   ImGui_ImplVulkan_Shutdown();backend=false;return Status::Completed;
  });
  if(ui==Status::Failed)gpu_failed=true;
  auto renderer=coordinator.step(5,"renderer-and-wsi",!gpu_failed&&graphics,[&]{return gfx_destroy()?Status::Completed:Status::Failed;});
  if(renderer==Status::Failed)gpu_failed=true;
  coordinator.step(6,"ui-context",context&&!backend,[&]{ImGui::DestroyContext();context=false;return Status::Completed;});
  if(gpu_failed)startup("shutdown incomplete: retained resources; hardware hold remains");
  else startup("shutdown complete");
  return !gpu_failed&&coordinator.successful();
 }
 int finish(int result){bool clean=shutdown();return result?result:(clean?0:4);}
 ~NativeLifetime(){shutdown();}
};
bool exit_requested(const PadState &pad,double now,double &since){
 constexpr uint32_t chord=PAD_OPTIONS|PAD_CIRCLE;
 if((pad.buttons&chord)!=chord){since=now;return false;}
 return now-since>=1.0;
}
}
__attribute__((noinline)) static int run_native(){
 ui_first_error=VK_SUCCESS;application_workers_retained=false;native_startup_phase=0;plexium::last_shutdown_phase=0;

 NativeLifetime lifetime;
 plat_init();
 std::fprintf(stderr,"lifecycle: build=%s source-date=%s mode=%s\n",PLEX_BUILD_COMMIT,PLEX_SOURCE_DATE,PLEX_STARTUP_ONLY?"stage1":"full-client");
 startup("platform ready");lifetime.graphics=true;
 if(!gfx_init())return lifetime.finish(1);startup("graphics ready");
 IMGUI_CHECKVERSION();ImGui::CreateContext();lifetime.context=true;auto &io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={float(gfx.width),float(gfx.height)};
 std::string font=std::string(plat_data_dir())+"/fonts/Inter-Regular.ttf";if(FILE *file=std::fopen(font.c_str(),"rb")){std::fclose(file);io.Fonts->AddFontFromFileTTF(font.c_str(),32);}
 else {startup("font unavailable: using built-in font");io.Fonts->AddFontDefault();}
 ImGui_ImplVulkan_InitInfo init{};init.ApiVersion=VK_API_VERSION_1_1;init.Instance=gfx.instance;init.PhysicalDevice=gfx.gpu;init.Device=gfx.device;init.QueueFamily=gfx.queue_family;init.Queue=gfx.queue;init.DescriptorPoolSize=2048;init.MinImageCount=2;init.ImageCount=std::max(2u,gfx.image_count);init.PipelineInfoMain.RenderPass=gfx.render_pass;init.PipelineInfoMain.MSAASamples=VK_SAMPLE_COUNT_1_BIT;
 init.CheckVkResultFn=ui_result;
 if(!ImGui_ImplVulkan_Init(&init))return lifetime.finish(2);
 lifetime.backend=true;
 if(!textures_begin_context()){lifetime.gpu_failed=true;return lifetime.finish(2);}
 startup("UI ready");
#if PLEX_STARTUP_ONLY
 startup("stage 1: renderer/controller only");
 double last=plat_time(),exit_since=last;
 for(;;){
  double now=plat_time();PadState pad{};plat_pad_read(&pad);
  if(exit_requested(pad,now,exit_since))break;
  ImGui_ImplVulkan_NewFrame();io.DeltaTime=std::clamp(float(now-last),.001f,.1f);last=now;
  ImGui::NewFrame();
  ImGui::SetNextWindowPos({io.DisplaySize.x*.1f,io.DisplaySize.y*.2f});
  ImGui::SetNextWindowSize({io.DisplaySize.x*.8f,io.DisplaySize.y*.6f});
  ImGui::Begin("Plexium startup diagnostics",nullptr,ImGuiWindowFlags_NoDecoration);
  ImGui::TextUnformatted("Plexium - Stage 1 startup diagnostics");
  ImGui::TextUnformatted("Renderer and controller validation. Hold Options + Circle for one second to exit.");
  ImGui::Text("Controller: %s",pad.connected?"connected":"unavailable");
  ImGui::Text("Frames: %llu",(unsigned long long)gfx.frame_number);
  ImGui::End();ImGui::Render();
  auto cmd=gfx_begin_frame();if(!cmd){lifetime.gpu_failed=true;return lifetime.finish(4);}
  gfx_begin_pass(cmd);ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),cmd);
  if(ui_first_error!=VK_SUCCESS || !gfx_healthy()){lifetime.gpu_failed=ImGui_ImplVulkan_HasPendingUpload()||!gfx_healthy();return lifetime.finish(4);}
  if(!gfx_end_frame(cmd)){lifetime.gpu_failed=true;return lifetime.finish(4);}
  if(gfx.frame_number==3)plat_hide_splash();
 }
 return lifetime.finish(0);
#else
 lifetime.player=true;if(!player_init())return lifetime.finish(3);startup("player ready");
 ApplicationPtr application(new Application);
 auto &player=application->player;auto &view=application->view;auto &artwork=application->artwork;
 auto &runtime=application->runtime;auto &model=application->model;auto &access=application->access;auto &art_scope=application->art_scope;
 if(!runtime.ready()){std::fprintf(stderr,"Plexium startup: session worker failed (%d)\n",runtime.start_error());application.reset();return lifetime.finish(5);}
 startup("session ready");runtime.send({plexium::Action::Restore,{},{}});
 double last=plat_time(),poll=last,timeline=last,exit_since=last;uint32_t previous=0;
 for(;;){
  player.pump();
  if(player_decoder_cleanup_failed()){lifetime.gpu_failed=true;application.reset();return lifetime.finish(4);}
  player_tick(); // Retain VLC stop/queued-open, subtitle and decoder health servicing.
  double now=plat_time();float dt=std::clamp(float(now-last),.001f,.1f);last=now;PadState pad{};plat_pad_read(&pad);
  if(exit_requested(pad,now,exit_since))break;
  unsigned pressed=buttons(pad.buttons&~previous);previous=pad.buttons;
  runtime.snapshot_into(model,access);
  if(model.screen==plexium::Screen::Playback){model.paused=player_paused();model.buffering=player_buffering();}
  if(art_scope!=access.scope){artwork.clear();art_scope=access.scope;artwork.set_scope(std::string(plexium::data_path)+"/art",access.scope);artwork.set_avatar_scope(std::string(plexium::data_path)+"/avatars",access.account_token);}
  artwork.update();
  if(!gfx_healthy()){lifetime.gpu_failed=true;application.reset();return lifetime.finish(4);}
  view.image=[&](const plex::Item &item,bool landscape){return artwork.media(access.base,access.token,item,landscape);};
  view.aspect=[&](ImTextureID texture){return artwork.aspect(texture);};
  view.avatar=[&](const std::string &url){return artwork.avatar(url,access.account_token);};plat_pad_set_private(view.sensitive(model));
  if(model.screen==plexium::Screen::Linking&&now>=poll&&!model.busy){runtime.send({plexium::Action::Poll,{},{}});poll=now+2;}
  if(model.screen==plexium::Screen::Playback&&now>=timeline&&!model.busy){runtime.send({plexium::Action::Tick,{},{}});timeline=now+10;}
  if(model.screen==plexium::Screen::Playback&&player_ended()&&!model.busy)runtime.send({plexium::Action::Stop,"ended",{}});
  ImGui_ImplVulkan_NewFrame();io.DisplaySize={float(gfx.width),float(gfx.height)};io.DeltaTime=dt;ImGui::NewFrame();
  auto action=view.frame(model,pressed,dt);if(action){if(action->action==plexium::Action::Stop){action->text=std::to_string(player_time());player_stop();}runtime.send(*action);}ImGui::Render();
  auto cmd=gfx_begin_frame();if(!cmd){lifetime.gpu_failed=true;application.reset();return lifetime.finish(4);}
  video_upload(cmd);if(!gfx_healthy()){lifetime.gpu_failed=true;application.reset();return lifetime.finish(4);}
  bool playing=model.screen==plexium::Screen::Playback&&model.detail.type!="track";
  video_prepare(cmd,playing?float(gfx.width):0,playing?float(gfx.height):0);
  if(!gfx_healthy()){lifetime.gpu_failed=true;application.reset();return lifetime.finish(4);}
  gfx_begin_pass(cmd);
  if(playing){float aspect=video_picture_aspect();float w=float(gfx.width),h=aspect>0?w/aspect:float(gfx.height);if(h>gfx.height){h=float(gfx.height);w=h*aspect;}video_draw(cmd,(gfx.width-w)/2,(gfx.height-h)/2,w,h);}
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),cmd);
  if(ui_first_error!=VK_SUCCESS || !gfx_healthy()){lifetime.gpu_failed=ImGui_ImplVulkan_HasPendingUpload()||!gfx_healthy();application.reset();return lifetime.finish(4);}if(!gfx_end_frame(cmd)){lifetime.gpu_failed=true;application.reset();return lifetime.finish(4);}
  if(gfx.frame_number==3)plat_hide_splash();
 }
 application.reset();return lifetime.finish(0);
#endif
}

int main(){int result=run_native();std::fprintf(stderr,"Plexium exit: status=%d\n",result);plat_quit(result);return result;}
