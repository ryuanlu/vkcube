#ifndef	__HOOK_H__
#define	__HOOK_H__

#include <vulkan/vulkan.h>

VKAPI_ATTR VkResult VKAPI_CALL __hook__vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
VKAPI_ATTR void VKAPI_CALL __hook__vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR VkResult VKAPI_CALL __hook__vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);
VKAPI_ATTR void VKAPI_CALL __hook__vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL __hook__vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);
VKAPI_ATTR VkResult VKAPI_CALL __hook__vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);

#ifdef	ENABLE_HOOK
#define	vkCreateDevice		__hook__vkCreateDevice
#define	vkDestroyDevice		__hook__vkDestroyDevice
#define	vkCreateSwapchainKHR	__hook__vkCreateSwapchainKHR
#define	vkDestroySwapchainKHR	__hook__vkDestroySwapchainKHR
#define	vkAcquireNextImageKHR	__hook__vkAcquireNextImageKHR
#define	vkQueuePresentKHR	__hook__vkQueuePresentKHR
#endif

#endif	/* __HOOK_H__ */