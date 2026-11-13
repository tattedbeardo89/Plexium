// SPDX-License-Identifier: GPL-3.0-or-later
// Execute the real vendored backend with a deterministic Vulkan allocator.
#include "check.h"
#define VOLK_IMPLEMENTATION
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "../third_party/imgui/backends/imgui_impl_vulkan.cpp"
#include <map>
namespace {
std::map<uintptr_t,char> live;
unsigned attempts=0,fail_at=0,reports=0;
template<class T> VkResult allocate(T *out,char kind){
 ++attempts;if(attempts==fail_at)return VK_ERROR_OUT_OF_DEVICE_MEMORY;
 *out=reinterpret_cast<T>(uintptr_t(attempts));live[attempts]=kind;return VK_SUCCESS;
}
template<class T> void release(T value){TEST_REQUIRE(value && live.erase(reinterpret_cast<uintptr_t>(value))==1);}
void report(VkResult rc){if(rc!=VK_SUCCESS){TEST_REQUIRE(rc==VK_ERROR_OUT_OF_DEVICE_MEMORY);++reports;}}
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
 if(rc!=VK_SUCCESS){*out=reinterpret_cast<VkPipeline>(uintptr_t(attempts));live[attempts]='P';}
 return rc;
}
static VKAPI_ATTR void VKAPI_CALL properties(VkPhysicalDevice,VkPhysicalDeviceProperties *out){*out={};out->limits.nonCoherentAtomSize=64;}
static VKAPI_ATTR void VKAPI_CALL update(VkDevice,uint32_t count,const VkWriteDescriptorSet *writes,uint32_t,const VkCopyDescriptorSet*){
 for(unsigned i=0;i<count;++i)TEST_REQUIRE(live.count(reinterpret_cast<uintptr_t>(writes[i].dstSet)));
}
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
 puts("PASS: real ImGui Vulkan initialization, 13 allocation failures, partial pipeline cleanup, no invalid descriptor use or attached backend leaks");
}
