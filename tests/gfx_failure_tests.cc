#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
// Execute the production frame code with a fault-injecting Vulkan dispatch.
#define VOLK_IMPLEMENTATION
#include "../src/gfx.cc"
static VkResult wait_result, begin_result, submit_result, present_result;
static int waits, resets, submits, presents;
static int surface_failure=0,swap_creates=0;
static VKAPI_ATTR VkResult VKAPI_CALL formats_mock(VkPhysicalDevice,VkSurfaceKHR,uint32_t *count,VkSurfaceFormatKHR *out){
 if(surface_failure==1)return VK_ERROR_SURFACE_LOST_KHR;
 if(!out){*count=surface_failure==2?0:surface_failure==3?1000000:1;return VK_SUCCESS;}
 if(surface_failure==4)return VK_INCOMPLETE;
 *count=1;out[0]={VK_FORMAT_B8G8R8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL caps_mock(VkPhysicalDevice,VkSurfaceKHR,VkSurfaceCapabilitiesKHR *out){
 if(surface_failure==5)return VK_ERROR_SURFACE_LOST_KHR;
 *out={};out->minImageCount=2;out->currentExtent={1280,720};
 out->supportedCompositeAlpha=surface_failure==6?0:VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR|VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL swap_mock(VkDevice,const VkSwapchainCreateInfoKHR *info,const VkAllocationCallbacks*,VkSwapchainKHR*){
 ++swap_creates;TEST_REQUIRE(info->compositeAlpha==VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR);return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}
static VKAPI_ATTR VkResult VKAPI_CALL idle_mock(VkDevice){return wait_result;}
static VKAPI_ATTR VkResult VKAPI_CALL wait_mock(VkDevice,uint32_t,const VkFence*,VkBool32,uint64_t timeout){TEST_REQUIRE(timeout==frame_timeout_ns);++waits;return wait_result;}
static VKAPI_ATTR VkResult VKAPI_CALL pool_mock(VkDevice,VkCommandPool,VkCommandPoolResetFlags){return VK_SUCCESS;}
static VKAPI_ATTR VkResult VKAPI_CALL begin_mock(VkCommandBuffer,const VkCommandBufferBeginInfo*){return begin_result;}
static VKAPI_ATTR VkResult VKAPI_CALL reset_mock(VkDevice,uint32_t,const VkFence*){++resets;return VK_SUCCESS;}
static VKAPI_ATTR void VKAPI_CALL pass_mock(VkCommandBuffer){}
static VKAPI_ATTR VkResult VKAPI_CALL end_mock(VkCommandBuffer){return VK_SUCCESS;}
static VKAPI_ATTR VkResult VKAPI_CALL submit_mock(VkQueue,uint32_t,const VkSubmitInfo*,VkFence){++submits;return submit_result;}
static VKAPI_ATTR VkResult VKAPI_CALL acquire_mock(VkDevice,VkSwapchainKHR,uint64_t timeout,VkSemaphore,VkFence,uint32_t *index){TEST_REQUIRE(timeout==frame_timeout_ns);*index=0;return VK_SUCCESS;}
static VKAPI_ATTR VkResult VKAPI_CALL present_mock(VkQueue,const VkPresentInfoKHR*){++presents;return present_result;}
static void reset_test(){
 gfx={};gfx.offscreen=false;frames[0].cmd=reinterpret_cast<VkCommandBuffer>(uintptr_t(1));frames[0].number=0;rendered.resize(1);pass_now=PASS_NONE;frame_failed=false;
 wait_result=begin_result=submit_result=present_result=VK_SUCCESS;waits=resets=submits=presents=0;
 vkWaitForFences=wait_mock;vkResetCommandPool=pool_mock;vkBeginCommandBuffer=begin_mock;vkResetFences=reset_mock;vkCmdEndRenderPass=pass_mock;vkEndCommandBuffer=end_mock;vkQueueSubmit=submit_mock;vkAcquireNextImageKHR=acquire_mock;vkQueuePresentKHR=present_mock;
}
int main(){
 reset_test();wait_result=VK_TIMEOUT;TEST_REQUIRE(!gfx_begin_frame());TEST_REQUIRE(resets==0&&submits==0);TEST_REQUIRE(!gfx_begin_frame());TEST_REQUIRE(waits==1);
 reset_test();begin_result=VK_ERROR_DEVICE_LOST;TEST_REQUIRE(!gfx_begin_frame());TEST_REQUIRE(resets==0);
 reset_test();auto cmd=gfx_begin_frame();TEST_REQUIRE(cmd&&resets==0);submit_result=VK_ERROR_DEVICE_LOST;TEST_REQUIRE(!gfx_end_frame(cmd));TEST_REQUIRE(resets==1&&submits==1&&presents==0&&gfx.frame_number==0);TEST_REQUIRE(!gfx_begin_frame());
 reset_test();cmd=gfx_begin_frame();present_result=VK_ERROR_OUT_OF_DATE_KHR;TEST_REQUIRE(!gfx_end_frame(cmd));TEST_REQUIRE(submits==1&&presents==1);TEST_REQUIRE(!gfx_begin_frame());
 reset_test();cmd=gfx_begin_frame();TEST_REQUIRE(gfx_end_frame(cmd));TEST_REQUIRE(gfx.frame_number==1&&resets==1&&submits==1&&presents==1);
 reset_test();gfx.device=reinterpret_cast<VkDevice>(uintptr_t(1));vkDeviceWaitIdle=idle_mock;wait_result=VK_ERROR_DEVICE_LOST;TEST_REQUIRE(!gfx_shutdown());wait_result=VK_SUCCESS;TEST_REQUIRE(gfx_shutdown());
 vkGetPhysicalDeviceSurfaceFormatsKHR=formats_mock;vkGetPhysicalDeviceSurfaceCapabilitiesKHR=caps_mock;vkCreateSwapchainKHR=swap_mock;
 for(surface_failure=1;surface_failure<=7;++surface_failure){swap_creates=0;TEST_REQUIRE(!create_swapchain());TEST_REQUIRE(swap_creates==int(surface_failure==7));}
 puts("PASS: real frame loop stops on timeout/device loss/submit/present errors; success advances once");
}
