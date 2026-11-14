// SPDX-License-Identifier: GPL-3.0-or-later
// Actual player implementation with controlled VLC/platform boundaries. This
// exercises ownership, not VLC decoding, PS5 audio or a native driver.
#include "check.h"
#define VOLK_IMPLEMENTATION
#include "../src/player.cc"
#include <future>
Gfx gfx{};
namespace {
int stops=0,player_releases=0,media_releases=0,instance_releases=0,audio_closes=0;
bool audio_ok=true,block_stop=false;
std::promise<void> stop_entered,stop_release;
std::shared_future<void> stop_barrier;
unsigned gpu_objects=0;
template<class T> VKAPI_ATTR void VKAPI_CALL destroy_gpu(VkDevice,T value,const VkAllocationCallbacks*){TEST_REQUIRE(value);++gpu_objects;}
VKAPI_ATTR void VKAPI_CALL unmap_gpu(VkDevice,VkDeviceMemory){++gpu_objects;}
VKAPI_ATTR VkResult VKAPI_CALL sampler_failure(VkDevice,const VkSamplerCreateInfo*,const VkAllocationCallbacks*,VkSampler*){return VK_ERROR_OUT_OF_DEVICE_MEMORY;}
}
VkShaderModule gfx_shader(const uint32_t*,size_t){return VK_NULL_HANDLE;}
static unsigned picture_allocations=0;
bool gfx_buffer(VkDeviceSize bytes,VkBufferUsageFlags,VkMemoryPropertyFlags,VkBuffer *buffer,VkDeviceMemory *memory,void **mapped){
 static uint8_t storage[SLOTS][16384];TEST_REQUIRE(bytes<=16384 && picture_allocations<SLOTS);
 *buffer=reinterpret_cast<VkBuffer>(uintptr_t(picture_allocations+1));*memory=reinterpret_cast<VkDeviceMemory>(uintptr_t(picture_allocations+1));*mapped=storage[picture_allocations++];return true;
}
extern "C" {
void libvlc_media_player_stop(libvlc_media_player_t*){++stops;if(block_stop){stop_entered.set_value();stop_barrier.wait();}}
void libvlc_media_player_release(libvlc_media_player_t*){++player_releases;}
void libvlc_media_release(libvlc_media_t*){++media_releases;}
void libvlc_release(libvlc_instance_t*){++instance_releases;}
void libvlc_log_unset(libvlc_instance_t*){}
bool plat_audio_close(){++audio_closes;return audio_ok;}
}
int main(){
 vkCreateSampler=sampler_failure;
 TEST_REQUIRE(!create_pipeline());native_phase=PlayerPhase::Initializing;
 TEST_REQUIRE(player_shutdown() && video_shutdown());
 player_closing=false;stopping=false;int16_t tiny[2]={};
 amem_play(nullptr,tiny,4u*AUDIO_RATE+1,0);amem_play(nullptr,nullptr,2,0);
 TEST_REQUIRE(audio_queue.empty());
 vkDestroyBuffer=destroy_gpu<VkBuffer>;vkFreeMemory=destroy_gpu<VkDeviceMemory>;vkUnmapMemory=unmap_gpu;
 vkDestroyPipeline=destroy_gpu<VkPipeline>;vkDestroyPipelineLayout=destroy_gpu<VkPipelineLayout>;
 vkDestroyImage=destroy_gpu<VkImage>;vkDestroyImageView=destroy_gpu<VkImageView>;vkDestroyFramebuffer=destroy_gpu<VkFramebuffer>;
 vkDestroyDescriptorPool=destroy_gpu<VkDescriptorPool>;vkDestroyDescriptorSetLayout=destroy_gpu<VkDescriptorSetLayout>;
 vkDestroySampler=destroy_gpu<VkSampler>;vkDestroyRenderPass=destroy_gpu<VkRenderPass>;vkDestroyQueryPool=destroy_gpu<VkQueryPool>;
 std::vector<uint8_t> input(1024*1024,0x5a),output(input.size());
 for(int cycle=0;cycle<32;++cycle){
  native_phase=PlayerPhase::Ready;player_closing=false;release_complete=false;copy_pool.closing=false;
  TEST_REQUIRE(auxiliary_jobs.reopen());
  vlc=reinterpret_cast<libvlc_instance_t*>(1);mp=reinterpret_cast<libvlc_media_player_t*>(2);
  queued_media=reinterpret_cast<libvlc_media_t*>(3);last_media=reinterpret_cast<libvlc_media_t*>(4);
  std::fill(output.begin(),output.end(),0);
  vlc_ps5_copy_plane_shift(output.data(),1024,input.data(),1024,1024,1024,0);
  TEST_REQUIRE(output==input);
  auto released=player_releases;
  TEST_REQUIRE(player_shutdown() && player_phase()==PlayerPhase::Released);
  TEST_REQUIRE(copy_pool.workers==0 && !copy_pool.started);
  TEST_REQUIRE(!vlc && !mp && !queued_media && !last_media && player_releases==released+1);
  TEST_REQUIRE(player_shutdown() && player_releases==released+1);
  TEST_REQUIRE(player_closing.load());
  // Real video cleanup only becomes legal after player/callback release.
  pipeline=reinterpret_cast<VkPipeline>(1);
  slots[0].buffer=reinterpret_cast<VkBuffer>(2);slots[0].memory=reinterpret_cast<VkDeviceMemory>(3);slots[0].mapped=reinterpret_cast<uint8_t*>(1);
  auto old=gpu_objects;TEST_REQUIRE(video_shutdown() && gpu_objects==old+4);
  TEST_REQUIRE(video_shutdown() && gpu_objects==old+4);
 }
 TEST_REQUIRE(stops==32 && player_releases==32 && media_releases==64 && instance_releases==32);
 // Actual format callback bounds allocation and creates its fallback before
 // decode callbacks can use it. Shutdown rejects new format allocations.
 native_phase=PlayerPhase::Ready;player_closing=false;release_complete=false;TEST_REQUIRE(auxiliary_jobs.reopen());
 unsigned w=0,h=2,pitches[3]={},lines[3]={};char chroma[5]="I420";void *opaque=nullptr;
 TEST_REQUIRE(vmem_setup(&opaque,chroma,&w,&h,pitches,lines)==0 && picture_allocations==0);
 w=2;TEST_REQUIRE(vmem_setup(&opaque,chroma,&w,&h,pitches,lines)==SLOTS && scratch_picture);
 for(auto &slot:slots){slot.state=GPU;slot.gpu_frame=1000;}
 void *out[3]={};auto picture_id=vmem_lock(nullptr,out);TEST_REQUIRE(reinterpret_cast<intptr_t>(picture_id)==-1 && scratch_picture_busy);
 TEST_REQUIRE(out[0]==scratch_picture);vmem_unlock(nullptr,picture_id,out);TEST_REQUIRE(!scratch_picture_busy);
 player_request_shutdown();TEST_REQUIRE(vmem_setup(&opaque,chroma,&w,&h,pitches,lines)==0 && picture_allocations==SLOTS);
 TEST_REQUIRE(player_shutdown() && video_shutdown() && !scratch_picture && scratch_picture_size==0);
 // An uninterruptible library stop returns a real timeout, retaining its
 // owner. No player/media release may happen until the stop actually returns.
 native_phase=PlayerPhase::Playing;player_closing=false;release_complete=false;
 TEST_REQUIRE(auxiliary_jobs.reopen());vlc=reinterpret_cast<libvlc_instance_t*>(1);mp=reinterpret_cast<libvlc_media_player_t*>(2);
 block_stop=true;stop_barrier=stop_release.get_future().share();
 TEST_REQUIRE(!player_shutdown() && player_shutdown_timed_out());TEST_REQUIRE(stop_entered.get_future().wait_for(std::chrono::seconds(5))==std::future_status::ready);
 TEST_REQUIRE(player_releases==32 && mp && !video_shutdown());
 block_stop=false;stop_release.set_value();
 TEST_REQUIRE(player_shutdown() && player_releases==33);
 // Audio close errors retain the lifecycle for a safe retry after media is
 // already released; release functions must not be called twice.
 native_phase=PlayerPhase::Stopped;release_complete=false;TEST_REQUIRE(auxiliary_jobs.reopen());audio_ok=false;
 TEST_REQUIRE(!player_shutdown() && !player_shutdown_timed_out());auto released=player_releases;
 audio_ok=true;TEST_REQUIRE(player_shutdown() && player_releases==released);
 // An active audio output job must finish before its port can be closed.
 native_phase=PlayerPhase::Playing;player_closing=false;release_complete=false;TEST_REQUIRE(auxiliary_jobs.reopen());
 std::promise<void> audio_entered,audio_release;auto audio_barrier=audio_release.get_future().share();
 TEST_REQUIRE(audio_thread.start([&]{audio_entered.set_value();audio_barrier.wait();})==0);
 TEST_REQUIRE(audio_entered.get_future().wait_for(std::chrono::seconds(5))==std::future_status::ready);
 auto closed_ports=audio_closes;
 TEST_REQUIRE(!player_shutdown() && player_shutdown_timed_out() && audio_closes==closed_ports);
 int16_t samples[4]={};amem_play(nullptr,samples,2,0);TEST_REQUIRE(audio_queue.empty());
 audio_release.set_value();TEST_REQUIRE(player_shutdown() && audio_closes==closed_ports+1);
 puts("PASS: production player repeated release, queued media, stop timeout retention/retry, audio close failure/retry, callback ownership and actual video object disposal");
}
