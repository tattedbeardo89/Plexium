// SPDX-License-Identifier: GPL-3.0-or-later
// Execute the real vendored backend with a deterministic Vulkan allocator.
#include "check.h"
#define VOLK_IMPLEMENTATION
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "../third_party/imgui/backends/imgui_impl_vulkan.cpp"
#include <map>
namespace {
std::map<uintptr_t,char> live;
unsigned attempts=0,fail_at=0,reports=0,next_handle=0;
bool font_timeout=false;
VkResult outcome(){++attempts;return attempts==fail_at?(font_timeout?VK_TIMEOUT:VK_ERROR_OUT_OF_DEVICE_MEMORY):VK_SUCCESS;}
template<class T> VkResult allocate(T *out,char kind){
 auto rc=outcome();if(rc!=VK_SUCCESS)return rc;
 *out=reinterpret_cast<T>(uintptr_t(++next_handle));live[next_handle]=kind;return VK_SUCCESS;
}
template<class T> void release(T value){TEST_REQUIRE(value && live.erase(reinterpret_cast<uintptr_t>(value))==1);}
void report(VkResult rc){if(rc!=VK_SUCCESS){TEST_REQUIRE(rc==VK_ERROR_OUT_OF_DEVICE_MEMORY || rc==VK_TIMEOUT);++reports;}}
#define CREATE(name,Info,Type,kind) static VKAPI_ATTR VkResult VKAPI_CALL name(VkDevice,const Info*,const VkAllocationCallbacks*,Type *out){return allocate(out,kind);}
#define DESTROY(name,Type) static VKAPI_ATTR void VKAPI_CALL name(VkDevice,Type value,const VkAllocationCallbacks*){release(value);}
CREATE(layout,VkDescriptorSetLayoutCreateInfo,VkDescriptorSetLayout,'l')
CREATE(pool,VkDescriptorPoolCreateInfo,VkDescriptorPool,'p')
CREATE(sampler,VkSamplerCreateInfo,VkSampler,'s')
CREATE(pipeline_layout,VkPipelineLayoutCreateInfo,VkPipelineLayout,'L')
CREATE(shader,VkShaderModuleCreateInfo,VkShaderModule,'S')
CREATE(command_pool,VkCommandPoolCreateInfo,VkCommandPool,'c')
DESTROY(destroy_layout,VkDescriptorSetLayout)
DESTROY(destroy_sampler,VkSampler)
DESTROY(destroy_pipeline_layout,VkPipelineLayout)
DESTROY(destroy_shader,VkShaderModule)
DESTROY(destroy_pipeline,VkPipeline)
DESTROY(destroy_command_pool,VkCommandPool)
static VKAPI_ATTR void VKAPI_CALL destroy_pool(VkDevice,VkDescriptorPool value,const VkAllocationCallbacks*){
 release(value);for(auto i=live.begin();i!=live.end();)if(i->second=='d')i=live.erase(i);else ++i;
}
static VKAPI_ATTR VkResult VKAPI_CALL descriptors(VkDevice,const VkDescriptorSetAllocateInfo*,VkDescriptorSet *out){return allocate(out,'d');}
static VKAPI_ATTR VkResult VKAPI_CALL commands(VkDevice,const VkCommandBufferAllocateInfo*,VkCommandBuffer *out){return allocate(out,'C');}
static VKAPI_ATTR void VKAPI_CALL free_commands(VkDevice,VkCommandPool,uint32_t count,const VkCommandBuffer *values){for(unsigned i=0;i<count;++i)release(values[i]);}
static VKAPI_ATTR VkResult VKAPI_CALL pipeline(VkDevice,VkPipelineCache,uint32_t,const VkGraphicsPipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline *out){
 auto rc=allocate(out,'P');
 // Vulkan permits partial pipeline creation even when the aggregate call fails.
 if(rc!=VK_SUCCESS){*out=reinterpret_cast<VkPipeline>(uintptr_t(++next_handle));live[next_handle]='P';}
 return rc;
}
static VKAPI_ATTR void VKAPI_CALL properties(VkPhysicalDevice,VkPhysicalDeviceProperties *out){*out={};out->limits.nonCoherentAtomSize=64;}
static VKAPI_ATTR void VKAPI_CALL update(VkDevice,uint32_t count,const VkWriteDescriptorSet *writes,uint32_t,const VkCopyDescriptorSet*){
 for(unsigned i=0;i<count;++i)TEST_REQUIRE(live.count(reinterpret_cast<uintptr_t>(writes[i].dstSet)));
}
CREATE(image,VkImageCreateInfo,VkImage,'I')
CREATE(image_view,VkImageViewCreateInfo,VkImageView,'V')
CREATE(buffer,VkBufferCreateInfo,VkBuffer,'B')
CREATE(fence,VkFenceCreateInfo,VkFence,'F')
DESTROY(destroy_image,VkImage)
DESTROY(destroy_view,VkImageView)
DESTROY(destroy_buffer,VkBuffer)
DESTROY(destroy_memory,VkDeviceMemory)
DESTROY(destroy_fence,VkFence)
static VKAPI_ATTR VkResult VKAPI_CALL memory(VkDevice,const VkMemoryAllocateInfo*,const VkAllocationCallbacks*,VkDeviceMemory *out){return allocate(out,'M');}
static VKAPI_ATTR void VKAPI_CALL requirements_image(VkDevice,VkImage,VkMemoryRequirements *out){*out={256,64,1};}
static VKAPI_ATTR void VKAPI_CALL requirements_buffer(VkDevice,VkBuffer,VkMemoryRequirements *out){*out={256,64,1};}
static VKAPI_ATTR void VKAPI_CALL memory_properties(VkPhysicalDevice,VkPhysicalDeviceMemoryProperties *out){*out={};out->memoryTypeCount=1;out->memoryTypes[0].propertyFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;}
static VKAPI_ATTR VkResult VKAPI_CALL bind_image(VkDevice,VkImage,VkDeviceMemory,VkDeviceSize){return outcome();}
static VKAPI_ATTR VkResult VKAPI_CALL bind_buffer(VkDevice,VkBuffer,VkDeviceMemory,VkDeviceSize){return outcome();}
static VKAPI_ATTR VkResult VKAPI_CALL map_memory(VkDevice,VkDeviceMemory,VkDeviceSize,VkDeviceSize,VkMemoryMapFlags,void **out){static char pixels[1024];auto rc=outcome();if(rc==VK_SUCCESS)*out=pixels;return rc;}
static VKAPI_ATTR void VKAPI_CALL unmap_memory(VkDevice,VkDeviceMemory){}
static VKAPI_ATTR VkResult VKAPI_CALL flush(VkDevice,uint32_t,const VkMappedMemoryRange*){return outcome();}
static VKAPI_ATTR VkResult VKAPI_CALL reset_pool(VkDevice,VkCommandPool,VkCommandPoolResetFlags){return outcome();}
static VKAPI_ATTR VkResult VKAPI_CALL begin_command(VkCommandBuffer,const VkCommandBufferBeginInfo*){return outcome();}
static VKAPI_ATTR VkResult VKAPI_CALL end_command(VkCommandBuffer){return outcome();}
static VKAPI_ATTR VkResult VKAPI_CALL submit(VkQueue,uint32_t,const VkSubmitInfo*,VkFence){return outcome();}
static VKAPI_ATTR VkResult VKAPI_CALL wait_fence(VkDevice,uint32_t,const VkFence*,VkBool32,uint64_t timeout){TEST_REQUIRE(timeout==5000000000ULL);return outcome();}
static VKAPI_ATTR void VKAPI_CALL barrier(VkCommandBuffer,VkPipelineStageFlags,VkPipelineStageFlags,VkDependencyFlags,uint32_t,const VkMemoryBarrier*,uint32_t,const VkBufferMemoryBarrier*,uint32_t,const VkImageMemoryBarrier*){}
static VKAPI_ATTR void VKAPI_CALL copy(VkCommandBuffer,VkBuffer,VkImage,VkImageLayout,uint32_t,const VkBufferImageCopy*){}
static VKAPI_ATTR VkResult VKAPI_CALL free_sets(VkDevice,VkDescriptorPool,uint32_t count,const VkDescriptorSet *sets){for(unsigned i=0;i<count;++i)release(sets[i]);return VK_SUCCESS;}

}
int main(){
 vkCreateDescriptorSetLayout=layout;vkCreateDescriptorPool=pool;vkCreateSampler=sampler;vkCreatePipelineLayout=pipeline_layout;
 vkCreateShaderModule=shader;vkCreateGraphicsPipelines=pipeline;vkCreateCommandPool=command_pool;
 vkAllocateDescriptorSets=descriptors;vkAllocateCommandBuffers=commands;vkFreeCommandBuffers=free_commands;vkUpdateDescriptorSets=update;
 vkDestroyDescriptorSetLayout=destroy_layout;vkDestroyDescriptorPool=destroy_pool;vkDestroySampler=destroy_sampler;
 vkDestroyPipelineLayout=destroy_pipeline_layout;vkDestroyShaderModule=destroy_shader;vkDestroyPipeline=destroy_pipeline;vkDestroyCommandPool=destroy_command_pool;
 vkGetPhysicalDeviceProperties=properties;
 for(fail_at=0;fail_at<=13;++fail_at){
  attempts=reports=0;TEST_REQUIRE(live.empty());ImGui::CreateContext();
  ImGui_ImplVulkan_InitInfo init{};init.ApiVersion=VK_API_VERSION_1_1;
  init.Instance=reinterpret_cast<VkInstance>(1);init.PhysicalDevice=reinterpret_cast<VkPhysicalDevice>(1);
  init.Device=reinterpret_cast<VkDevice>(1);init.Queue=reinterpret_cast<VkQueue>(1);
  init.MinImageCount=init.ImageCount=2;init.DescriptorPoolSize=2048;
  init.PipelineInfoMain.RenderPass=reinterpret_cast<VkRenderPass>(1);init.PipelineInfoMain.MSAASamples=VK_SAMPLE_COUNT_1_BIT;init.CheckVkResultFn=report;
  bool ok=ImGui_ImplVulkan_Init(&init);TEST_REQUIRE(ok==(fail_at==0));
  if(ok){TEST_REQUIRE(attempts==13 && reports==0);ImGui_ImplVulkan_Shutdown();}
  else TEST_REQUIRE(attempts==fail_at && reports==1);
  TEST_REQUIRE(!ImGui::GetIO().BackendRendererUserData && live.empty());
  ImGui::DestroyContext();
 }
 vkCreateImage=image;vkCreateImageView=image_view;vkCreateBuffer=buffer;vkAllocateMemory=memory;vkCreateFence=fence;
 vkDestroyImage=destroy_image;vkDestroyImageView=destroy_view;vkDestroyBuffer=destroy_buffer;vkFreeMemory=destroy_memory;vkDestroyFence=destroy_fence;
 vkGetImageMemoryRequirements=requirements_image;vkGetBufferMemoryRequirements=requirements_buffer;vkGetPhysicalDeviceMemoryProperties=memory_properties;
 vkBindImageMemory=bind_image;vkBindBufferMemory=bind_buffer;vkMapMemory=map_memory;vkUnmapMemory=unmap_memory;vkFlushMappedMemoryRanges=flush;
 vkResetCommandPool=reset_pool;vkBeginCommandBuffer=begin_command;vkEndCommandBuffer=end_command;vkQueueSubmit=submit;vkWaitForFences=wait_fence;
 vkCmdPipelineBarrier=barrier;vkCmdCopyBufferToImage=copy;vkFreeDescriptorSets=free_sets;
 for(unsigned failed_operation=0;failed_operation<=17;++failed_operation){
  fail_at=0;attempts=reports=0;ImGui::CreateContext();
  ImGui_ImplVulkan_InitInfo init{};init.ApiVersion=VK_API_VERSION_1_1;
  init.Instance=reinterpret_cast<VkInstance>(1);init.PhysicalDevice=reinterpret_cast<VkPhysicalDevice>(1);init.Device=reinterpret_cast<VkDevice>(1);init.Queue=reinterpret_cast<VkQueue>(1);
  init.MinImageCount=init.ImageCount=2;init.DescriptorPoolSize=2048;init.PipelineInfoMain.RenderPass=reinterpret_cast<VkRenderPass>(1);init.PipelineInfoMain.MSAASamples=VK_SAMPLE_COUNT_1_BIT;init.CheckVkResultFn=report;
  TEST_REQUIRE(ImGui_ImplVulkan_Init(&init));auto base=live.size();
  attempts=reports=0;fail_at=failed_operation==17?16:failed_operation;font_timeout=failed_operation==17;
  ImTextureData tex;tex.Create(ImTextureFormat_RGBA32,2,2);memset(tex.Pixels,0,16);
  ImGui_ImplVulkan_UpdateTexture(&tex);
  TEST_REQUIRE((tex.Status==ImTextureStatus_OK)==(failed_operation==0));
  auto *bd=ImGui_ImplVulkan_GetBackendData();
  if(failed_operation){TEST_REQUIRE(bd->UploadFailed && reports==1 && live.size()>=base);auto calls=attempts;
   ImGui_ImplVulkan_UpdateTexture(&tex);TEST_REQUIRE(attempts==calls); // no unsafe retry/reset
  }
  TEST_REQUIRE(ImGui_ImplVulkan_HasPendingUpload()==(failed_operation>=15));
  if(failed_operation>=15)TEST_REQUIRE(bd->UploadFence && bd->UploadBuffer && bd->UploadMemory);
  // Fixture establishes completion before final disposal; production only
  // reaches shutdown after its owning graphics drain succeeds.
  fail_at=0;font_timeout=false;
  ImGui_ImplVulkan_StopUploads();auto calls=attempts;ImGui_ImplVulkan_UpdateTexture(&tex);TEST_REQUIRE(attempts==calls);
  ImGui_ImplVulkan_DestroyTexture(&tex);ImGui_ImplVulkan_Shutdown();ImGui::DestroyContext();TEST_REQUIRE(live.empty());
 }
 for(unsigned operation=1;operation<=9;++operation){
  fail_at=0;attempts=reports=0;ImGui::CreateContext();
  ImGui_ImplVulkan_InitInfo init{};init.ApiVersion=VK_API_VERSION_1_1;
  init.Instance=reinterpret_cast<VkInstance>(1);init.PhysicalDevice=reinterpret_cast<VkPhysicalDevice>(1);init.Device=reinterpret_cast<VkDevice>(1);init.Queue=reinterpret_cast<VkQueue>(1);
  init.MinImageCount=init.ImageCount=2;init.DescriptorPoolSize=2048;init.PipelineInfoMain.RenderPass=reinterpret_cast<VkRenderPass>(1);init.PipelineInfoMain.MSAASamples=VK_SAMPLE_COUNT_1_BIT;init.CheckVkResultFn=report;
  TEST_REQUIRE(ImGui_ImplVulkan_Init(&init));
  attempts=reports=0;fail_at=operation;
  {
  ImDrawList list(ImGui::GetDrawListSharedData());list.VtxBuffer.push_back(ImDrawVert{});list.IdxBuffer.push_back(0);
  ImDrawData draw;draw.DisplaySize={640,480};draw.FramebufferScale={1,1};draw.TotalVtxCount=draw.TotalIdxCount=1;draw.CmdLists.push_back(&list);
  ImGui_ImplVulkan_RenderDrawData(&draw,reinterpret_cast<VkCommandBuffer>(1));
  TEST_REQUIRE(ImGui_ImplVulkan_GetBackendData()->UploadFailed && reports==1 && attempts==operation);
  TEST_REQUIRE(!ImGui_ImplVulkan_HasPendingUpload());
  auto calls=attempts;ImGui_ImplVulkan_RenderDrawData(&draw,reinterpret_cast<VkCommandBuffer>(1));TEST_REQUIRE(attempts==calls);
  }
  fail_at=0;ImGui_ImplVulkan_Shutdown();ImGui::DestroyContext();TEST_REQUIRE(live.empty());
 }
 puts("PASS: production vertex/index allocation, bind, map and flush failures stop before invalid draws");
 puts("PASS: real font upload allocations/binds/map/flush/record/submit/fence timeout, retained ownership, no retry after failure and closed upload gate");
 puts("PASS: real ImGui Vulkan initialization, 13 allocation failures, partial pipeline cleanup, no invalid descriptor use or attached backend leaks");
}
