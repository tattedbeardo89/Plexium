// SPDX-License-Identifier: GPL-3.0-or-later
// Execute the production frame code with a fault-injecting Vulkan dispatch.
#define VOLK_IMPLEMENTATION
#include "../src/gfx.cc"
#include <cassert>
static VkResult wait_result, begin_result, submit_result, present_result;
static int waits, resets, submits, presents;
static VKAPI_ATTR VkResult VKAPI_CALL wait_mock(VkDevice,uint32_t,const VkFence*,VkBool32,uint64_t timeout){assert(timeout==frame_timeout_ns);++waits;return wait_result;}
static VKAPI_ATTR VkResult VKAPI_CALL pool_mock(VkDevice,VkCommandPool,VkCommandPoolResetFlags){return VK_SUCCESS;}
static VKAPI_ATTR VkResult VKAPI_CALL begin_mock(VkCommandBuffer,const VkCommandBufferBeginInfo*){return begin_result;}
static VKAPI_ATTR VkResult VKAPI_CALL reset_mock(VkDevice,uint32_t,const VkFence*){++resets;return VK_SUCCESS;}
static VKAPI_ATTR void VKAPI_CALL pass_mock(VkCommandBuffer){}
static VKAPI_ATTR VkResult VKAPI_CALL end_mock(VkCommandBuffer){return VK_SUCCESS;}
static VKAPI_ATTR VkResult VKAPI_CALL submit_mock(VkQueue,uint32_t,const VkSubmitInfo*,VkFence){++submits;return submit_result;}
static VKAPI_ATTR VkResult VKAPI_CALL acquire_mock(VkDevice,VkSwapchainKHR,uint64_t timeout,VkSemaphore,VkFence,uint32_t *index){assert(timeout==frame_timeout_ns);*index=0;return VK_SUCCESS;}
static VKAPI_ATTR VkResult VKAPI_CALL present_mock(VkQueue,const VkPresentInfoKHR*){++presents;return present_result;}
static void reset_test(){
 gfx={};gfx.offscreen=false;frames[0].cmd=reinterpret_cast<VkCommandBuffer>(uintptr_t(1));frames[0].number=0;rendered.resize(1);pass_now=PASS_NONE;frame_failed=false;
 wait_result=begin_result=submit_result=present_result=VK_SUCCESS;waits=resets=submits=presents=0;
 vkWaitForFences=wait_mock;vkResetCommandPool=pool_mock;vkBeginCommandBuffer=begin_mock;vkResetFences=reset_mock;vkCmdEndRenderPass=pass_mock;vkEndCommandBuffer=end_mock;vkQueueSubmit=submit_mock;vkAcquireNextImageKHR=acquire_mock;vkQueuePresentKHR=present_mock;
}
int main(){
 reset_test();wait_result=VK_TIMEOUT;assert(!gfx_begin_frame());assert(resets==0&&submits==0);assert(!gfx_begin_frame());assert(waits==1);
 reset_test();begin_result=VK_ERROR_DEVICE_LOST;assert(!gfx_begin_frame());assert(resets==0);
 reset_test();auto cmd=gfx_begin_frame();assert(cmd&&resets==0);submit_result=VK_ERROR_DEVICE_LOST;assert(!gfx_end_frame(cmd));assert(resets==1&&submits==1&&presents==0&&gfx.frame_number==0);assert(!gfx_begin_frame());
 reset_test();cmd=gfx_begin_frame();present_result=VK_ERROR_OUT_OF_DATE_KHR;assert(!gfx_end_frame(cmd));assert(submits==1&&presents==1);assert(!gfx_begin_frame());
 reset_test();cmd=gfx_begin_frame();assert(gfx_end_frame(cmd));assert(gfx.frame_number==1&&resets==1&&submits==1&&presents==1);
 puts("PASS: real frame loop stops on timeout/device loss/submit/present errors; success advances once");
}
