// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#define VOLK_IMPLEMENTATION
#include "../src/gfx.cc"
#include <set>
namespace {
std::set<uintptr_t> owned;
unsigned limit=0,planned=0;
template<class T> T resource(){auto id=++planned;if(id>limit)return VK_NULL_HANDLE;owned.insert(id);return reinterpret_cast<T>(uintptr_t(id));}
template<class T> void release(T value){TEST_REQUIRE(value && owned.erase(reinterpret_cast<uintptr_t>(value))==1);}
template<class T> VKAPI_ATTR void VKAPI_CALL destroy(VkDevice,T value,const VkAllocationCallbacks*){release(value);}
VKAPI_ATTR void VKAPI_CALL device(VkDevice value,const VkAllocationCallbacks*){
 release(value);TEST_REQUIRE(owned.size()==2); // instance and surface survive the device
}
VKAPI_ATTR void VKAPI_CALL instance(VkInstance value,const VkAllocationCallbacks*){release(value);TEST_REQUIRE(owned.empty());}
VKAPI_ATTR void VKAPI_CALL destroy_surface(VkInstance,VkSurfaceKHR value,const VkAllocationCallbacks*){release(value);}
VKAPI_ATTR void VKAPI_CALL unmap(VkDevice,VkDeviceMemory value){TEST_REQUIRE(owned.count(reinterpret_cast<uintptr_t>(value)));}
void populate(bool offscreen){
 gfx.instance=resource<VkInstance>();surface=resource<VkSurfaceKHR>();gfx.device=resource<VkDevice>();gfx.offscreen=offscreen;
 hdr.pipeline=resource<VkPipeline>();hdr.layout=resource<VkPipelineLayout>();hdr.pool=resource<VkDescriptorPool>();
 hdr.set_layout=resource<VkDescriptorSetLayout>();hdr.sampler=resource<VkSampler>();
 hdr.ui_fb=resource<VkFramebuffer>();hdr.fb=resource<VkFramebuffer>();framebuffers={resource<VkFramebuffer>()};
 hdr.ui_view=resource<VkImageView>();hdr.view=resource<VkImageView>();views={resource<VkImageView>()};
 hdr.ui_image=resource<VkImage>();hdr.image=resource<VkImage>();
 if(offscreen)images={resource<VkImage>()};else images={reinterpret_cast<VkImage>(uintptr_t(999))};
 hdr.ui_memory=resource<VkDeviceMemory>();hdr.memory=resource<VkDeviceMemory>();
 offscreen_memory=resource<VkDeviceMemory>();readback_memory=resource<VkDeviceMemory>();
 readback=resource<VkBuffer>();if(readback_memory)readback_mapped=reinterpret_cast<void*>(1);
 gfx.render_pass=resource<VkRenderPass>();hdr.ui_pass=resource<VkRenderPass>();hdr.output_pass=resource<VkRenderPass>();
 for(auto &f:frames){f.pool=resource<VkCommandPool>();f.fence=resource<VkFence>();f.acquired=resource<VkSemaphore>();}
 setup_pool=resource<VkCommandPool>();rendered={resource<VkSemaphore>()};swapchain=resource<VkSwapchainKHR>();
}
}
int main(){
 vkDestroyPipeline=destroy<VkPipeline>;vkDestroyPipelineLayout=destroy<VkPipelineLayout>;
 vkDestroyDescriptorPool=destroy<VkDescriptorPool>;vkDestroyDescriptorSetLayout=destroy<VkDescriptorSetLayout>;
 vkDestroySampler=destroy<VkSampler>;vkDestroyFramebuffer=destroy<VkFramebuffer>;vkDestroyImageView=destroy<VkImageView>;
 vkDestroyImage=destroy<VkImage>;vkDestroyBuffer=destroy<VkBuffer>;vkFreeMemory=destroy<VkDeviceMemory>;
 vkDestroyRenderPass=destroy<VkRenderPass>;vkDestroyCommandPool=destroy<VkCommandPool>;vkDestroyFence=destroy<VkFence>;
 vkDestroySemaphore=destroy<VkSemaphore>;vkDestroySwapchainKHR=destroy<VkSwapchainKHR>;vkDestroyDevice=device;
 vkDestroyInstance=instance;vkDestroySurfaceKHR=destroy_surface;vkUnmapMemory=unmap;
 for(bool offscreen:{false,true})for(limit=0;limit<=40;++limit){
  planned=0;populate(offscreen);gfx_destroy();TEST_REQUIRE(owned.empty());
  TEST_REQUIRE(!gfx.instance && !gfx.device && !surface && !swapchain && images.empty() && !hdr.output_pass);
  gfx_destroy();TEST_REQUIRE(owned.empty());
 }
 puts("PASS: production renderer teardown across partial allocations, HDR/offscreen ownership, WSI image exclusion, child-before-device order and idempotence");
}
