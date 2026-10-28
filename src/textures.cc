// SPDX-License-Identifier: GPL-3.0-or-later
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"
#pragma GCC diagnostic pop
#include "library.h"
#include "gfx.h"
#include "backends/imgui_impl_vulkan.h"
#include <vector>
#include <cstring>
namespace {
struct Texture {VkImage image{};VkDeviceMemory memory{};VkImageView view{};VkDescriptorSet set{};};
std::vector<Texture> textures;
VkSampler sampler{};
bool gpu_failed=false;
}
ImTextureID library_upload_rgba(const unsigned char*rgba,int width,int height){
 if(gpu_failed||!rgba||width<=0||height<=0||width>1920||height>1080)return 0;
 if(!sampler){VkSamplerCreateInfo si{};si.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;si.magFilter=si.minFilter=VK_FILTER_LINEAR;si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;if(vkCreateSampler(gfx.device,&si,nullptr,&sampler)!=VK_SUCCESS)return 0;}
 Texture t;size_t size=size_t(width)*height*4;
 if(!gfx_image(width,height,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,&t.image,&t.memory,&t.view))return 0;
 VkBuffer buffer{};VkDeviceMemory memory{};void *mapped=nullptr;
 if(!gfx_buffer(size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,&buffer,&memory,&mapped)){
  vkDestroyImageView(gfx.device,t.view,nullptr);vkDestroyImage(gfx.device,t.image,nullptr);vkFreeMemory(gfx.device,t.memory,nullptr);return 0;
 }
 memcpy(mapped,rgba,size);auto cmd=gfx_one_shot_begin();
 if(cmd){gfx_barrier(cmd,t.image,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,0,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT);
  VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={uint32_t(width),uint32_t(height),1};
  vkCmdCopyBufferToImage(cmd,buffer,t.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
  gfx_barrier(cmd,t.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT);
 }
 bool pending=false;bool ok=cmd&&gfx_one_shot_end(cmd,&pending);
 if(pending){if(vkDeviceWaitIdle(gfx.device)!=VK_SUCCESS){gpu_failed=true;return 0;}ok=false;}
 vkDestroyBuffer(gfx.device,buffer,nullptr);vkFreeMemory(gfx.device,memory,nullptr);
 if(ok)t.set=ImGui_ImplVulkan_AddTexture(sampler,t.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
 if(!t.set){vkDestroyImageView(gfx.device,t.view,nullptr);vkDestroyImage(gfx.device,t.image,nullptr);vkFreeMemory(gfx.device,t.memory,nullptr);return 0;}
 textures.push_back(t);return (ImTextureID)t.set;
}
void library_free_image(ImTextureID id){
 for(auto i=textures.begin();i!=textures.end();++i)if((ImTextureID)i->set==id){if(vkDeviceWaitIdle(gfx.device)!=VK_SUCCESS)return;ImGui_ImplVulkan_RemoveTexture(i->set);vkDestroyImageView(gfx.device,i->view,nullptr);vkDestroyImage(gfx.device,i->image,nullptr);vkFreeMemory(gfx.device,i->memory,nullptr);textures.erase(i);return;}
}
void textures_shutdown(){while(!textures.empty()){size_t n=textures.size();library_free_image((ImTextureID)textures.back().set);if(textures.size()==n)return;}if(sampler){vkDestroySampler(gfx.device,sampler,nullptr);sampler={};}}
