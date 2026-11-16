// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "../src/textures.cc"
Gfx gfx{};
namespace {
unsigned destroyed=0,unmapped=0,descriptors=0;
bool timeout=false;
char staging[256];
template<class T> VKAPI_ATTR void VKAPI_CALL destroy(VkDevice,T,const VkAllocationCallbacks*){++destroyed;}
VKAPI_ATTR void VKAPI_CALL unmap(VkDevice,VkDeviceMemory){++unmapped;}
VKAPI_ATTR VkResult VKAPI_CALL create_sampler(VkDevice,const VkSamplerCreateInfo*,const VkAllocationCallbacks*,VkSampler *out){*out=reinterpret_cast<VkSampler>(1);return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL copy(VkCommandBuffer,VkBuffer,VkImage,VkImageLayout,uint32_t,const VkBufferImageCopy*){}
}
bool gfx_image(uint32_t,uint32_t,VkFormat,VkImageUsageFlags,VkImage *image,VkDeviceMemory *memory,VkImageView *view,uint32_t){*image=reinterpret_cast<VkImage>(2);*memory=reinterpret_cast<VkDeviceMemory>(3);*view=reinterpret_cast<VkImageView>(4);return true;}
bool gfx_buffer(VkDeviceSize,VkBufferUsageFlags,VkMemoryPropertyFlags,VkBuffer *buffer,VkDeviceMemory *memory,void **mapped){*buffer=reinterpret_cast<VkBuffer>(5);*memory=reinterpret_cast<VkDeviceMemory>(6);*mapped=staging;return true;}
VkCommandBuffer gfx_one_shot_begin(){return reinterpret_cast<VkCommandBuffer>(1);}
bool gfx_one_shot_end(VkCommandBuffer,bool *pending){*pending=timeout;return !timeout;}
bool gfx_wait_queue(){return !timeout;}
void gfx_barrier(VkCommandBuffer,VkImage,VkImageLayout,VkImageLayout,VkPipelineStageFlags,VkAccessFlags,VkPipelineStageFlags,VkAccessFlags){}
VkDescriptorSet ImGui_ImplVulkan_AddTexture(VkSampler,VkImageView,VkImageLayout){return reinterpret_cast<VkDescriptorSet>(7);}
void ImGui_ImplVulkan_RemoveTexture(VkDescriptorSet value){TEST_REQUIRE(value);++descriptors;}
int main(){
 vkCreateSampler=create_sampler;vkCmdCopyBufferToImage=copy;vkUnmapMemory=unmap;
 vkDestroyBuffer=destroy<VkBuffer>;vkFreeMemory=destroy<VkDeviceMemory>;
 vkDestroyImageView=destroy<VkImageView>;vkDestroyImage=destroy<VkImage>;vkDestroySampler=destroy<VkSampler>;
 unsigned char rgba[16]={};
 for(unsigned cycle=0;cycle<32;++cycle){
  TEST_REQUIRE(textures_begin_context());auto id=library_upload_rgba(rgba,2,2);TEST_REQUIRE(id && textures.size()==1);
  TEST_REQUIRE(textures.back().context==context_generation);
  textures_stop_uploads();TEST_REQUIRE(!library_upload_rgba(rgba,2,2));
  TEST_REQUIRE(textures_shutdown() && textures.empty());TEST_REQUIRE(textures_shutdown());
 }
 TEST_REQUIRE(unmapped==32 && descriptors==32);
 TEST_REQUIRE(textures_begin_context());timeout=true;auto before=destroyed;
 TEST_REQUIRE(!library_upload_rgba(rgba,2,2) && gpu_failed && textures.size()==1);
 TEST_REQUIRE(textures.back().staging && textures.back().staging_memory && destroyed==before);
 TEST_REQUIRE(!textures_shutdown() && !textures_begin_context() && destroyed==before);
 puts("PASS: production artwork staging ownership, repeated context teardown, closed gate, timeout retention and no unsafe destruction");
}
