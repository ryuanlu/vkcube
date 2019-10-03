#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include "vkhelper.h"

#define SKIP_MESSAGE_TIMES	(60)
#define TEXTURE_IMAGE_FILE	"cthead.bin"
#define TEXTURE_IMAGE_WIDTH	(256)
#define TEXTURE_IMAGE_HEIGHT	(256)
#define TEXTURE_IMAGE_SIZE	(TEXTURE_IMAGE_WIDTH * TEXTURE_IMAGE_HEIGHT * 4)


struct hook_context
{
	vkhelper_device*	device;
	vkhelper_renderpass*	renderpass;
	vkhelper_image*		texture;

	VkShaderModule		vshader;
	VkShaderModule		fshader;
	VkDescriptorSetLayout	setlayout;
	VkPipelineLayout	pipelinelayout;
	VkPipeline		pipeline;
	VkDescriptorPool	desc_pool;
	VkDescriptorSet		desc_set;
	VkSampler		sampler;

	uint32_t		index;
};

static struct hook_context*	context = NULL;

extern unsigned char blit_vert_spv[];
extern unsigned int blit_vert_spv_len;

extern unsigned char blit_frag_spv[];
extern unsigned int blit_frag_spv_len;


struct hook_context* hook_init(VkPhysicalDevice phydevice, VkDevice device, int queuefamily)
{
	struct hook_context*	hook = NULL;
	hook = calloc(1, sizeof(struct hook_context));

	hook->device = vkhelper_device_create_with_vkdevice(phydevice, device, queuefamily);
	hook->vshader = vkhelper_shadermodule_create(hook->device, blit_vert_spv, blit_vert_spv_len);
	hook->fshader = vkhelper_shadermodule_create(hook->device, blit_frag_spv, blit_frag_spv_len);
	hook->index = -1;

	vkCreateDescriptorSetLayout
	(
		device,
		&(VkDescriptorSetLayoutCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &(VkDescriptorSetLayoutBinding)
			{
				.binding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			},
		},
		NULL, &hook->setlayout
	);

	vkCreatePipelineLayout
	(
		device,
		&(VkPipelineLayoutCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &hook->setlayout,
		},
		NULL, &hook->pipelinelayout
	);

	vkCreateSampler
	(
		device,
		&(VkSamplerCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = 16,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		},
		NULL, &hook->sampler
	);

	vkCreateDescriptorPool
	(
		device,
		&(VkDescriptorPoolCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.maxSets = 1,
			.poolSizeCount = 1,
			.pPoolSizes = (VkDescriptorPoolSize[])
			{
				{
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1
				},
			},
		},
		NULL, &hook->desc_pool
	);

	vkAllocateDescriptorSets
	(
		device,
		&(VkDescriptorSetAllocateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = hook->desc_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &hook->setlayout,
		},
		&hook->desc_set
	);

	char* image_data = NULL;
	FILE* fp = fopen(TEXTURE_IMAGE_FILE, "r");
	image_data = calloc(1, TEXTURE_IMAGE_SIZE);
	fread(image_data, TEXTURE_IMAGE_SIZE, 1, fp);
	fclose(fp);

	hook->texture = vkhelper_image_create(hook->device, image_data, TEXTURE_IMAGE_WIDTH, TEXTURE_IMAGE_HEIGHT);

	free(image_data);

	return hook;
}

void hook_destroy(struct hook_context* hook)
{
	VkDevice device = vkhelper_device_get_vkdevice(hook->device);

	vkDestroyPipeline(device, hook->pipeline, NULL);
	vkDestroyPipelineLayout(device, hook->pipelinelayout, NULL);
	vkDestroySampler(device, hook->sampler, NULL);
	vkDestroyDescriptorSetLayout(device, hook->setlayout, NULL);
	vkDestroyDescriptorPool(device, hook->desc_pool, NULL);
	vkhelper_renderpass_destroy(hook->device, hook->renderpass);
	vkhelper_image_destroy(hook->device, hook->texture);
	vkDestroyShaderModule(device, hook->vshader, NULL);
	vkDestroyShaderModule(device, hook->fshader, NULL);
	vkhelper_device_destroy(hook->device);

	free(hook);
}


/* Hook functions */

VKAPI_ATTR VkResult VKAPI_CALL __hook__vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
	VkResult result;
	fprintf(stderr, "__hook__vkCreateDevice\n");
	result = vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

	if(!context)
		context = hook_init(physicalDevice, *pDevice, pCreateInfo->pQueueCreateInfos->queueFamilyIndex);

	return result;
}

VKAPI_ATTR void VKAPI_CALL __hook__vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
	fprintf(stderr, "__hook__vkDestroyDevice\n");

	if(context)
	{
		hook_destroy(context);
		context = NULL;
	}
	vkDestroyDevice(device, NULL);
}

VKAPI_ATTR VkResult VKAPI_CALL __hook__vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
	static int counter = 0;
	VkResult result;
	result = vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);

	if(counter == 0)
		fprintf(stderr, "__hook__vkAcquireNextImageKHR index = %d (skip %d times)\n", *pImageIndex, SKIP_MESSAGE_TIMES);

	context->index = *pImageIndex;
	vkhelper_swapchain_set_semaphore(context->device, vkhelper_device_get_swapchain(context->device), semaphore);

	++counter;
	counter %= SKIP_MESSAGE_TIMES;

	return result;
}

VKAPI_ATTR VkResult VKAPI_CALL __hook__vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
	static int counter = 0;

	VkCommandBuffer cmdbuf;

	if(counter == 0)
		fprintf(stderr, "__hook__vkQueuePresentKHR (skip %d times)\n", SKIP_MESSAGE_TIMES);

	cmdbuf = vkhelper_surface_begin_cmdbuf(context->device, context->index, VK_TRUE);
	vkDeviceWaitIdle(vkhelper_device_get_vkdevice(context->device));

	vkUpdateDescriptorSets
	(
		vkhelper_device_get_vkdevice(context->device),
		1,
		&(VkWriteDescriptorSet)
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = context->desc_set,
			.dstBinding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.pImageInfo = &(VkDescriptorImageInfo)
			{
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.imageView = vkhelper_image_get_vkimageview(context->texture),
				.sampler = context->sampler,
			},
		},
		0, NULL
	);

	vkhelper_begin_renderpass(cmdbuf, context->renderpass, context->index);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline);
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipelinelayout, 0, 1, &context->desc_set, 0, NULL);

	vkCmdSetViewport(cmdbuf, 0, 1, &(VkViewport) { .width = 720, .height = 720,});
	vkCmdSetScissor(cmdbuf, 0, 1, &(VkRect2D) { .extent.width = 720, .extent.height = 720,});

	vkCmdDraw(cmdbuf, 6, 1, 0, 0);

	vkCmdEndRenderPass(cmdbuf);
	vkhelper_surface_end_cmdbuf(context->device, context->index);

	vkhelper_queue_submit(context->device, context->index);

	++counter;
	counter %= SKIP_MESSAGE_TIMES;

	return vkQueuePresentKHR(queue, pPresentInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL __hook__vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
	VkResult result;
	vkhelper_swapchain* swapchain;

	fprintf(stderr, "__hook__vkCreateSwapchainKHR: w:%d, h:%d, format: %d\n", pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height, pCreateInfo->imageFormat);

	result = vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
	swapchain = vkhelper_swapchain_create_with_vkswapchain(context->device, *pSwapchain, pCreateInfo);
	vkhelper_device_set_swapchain(context->device, swapchain);

	if(!context->renderpass)
	{
		context->renderpass = vkhelper_renderpass_create
		(
			context->device,
			&(VkRenderPassCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.attachmentCount = 1,
				.pAttachments = &(VkAttachmentDescription)
				{
					.format = pCreateInfo->imageFormat,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				},
				.subpassCount = 1,
				.pSubpasses = &(VkSubpassDescription)
				{
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.colorAttachmentCount = 1,
					.pColorAttachments = &(VkAttachmentReference)
					{
						.attachment = 0,
						.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					},
				},

			}
		);

		context->pipeline = vkhelper_create_graphics_pipeline
		(
			context->device, context->vshader, context->fshader,
			&(VkPipelineVertexInputStateCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			},
			context->pipelinelayout,
			context->renderpass
		);
	}else
	{
		vkhelper_renderpass_validate_swapchain(context->device, context->renderpass);
	}

	return result;
}

VKAPI_ATTR void VKAPI_CALL __hook__vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
	fprintf(stderr, "__hook__vkDestroySwapchainKHR\n");

	vkhelper_swapchain_destroy(context->device, vkhelper_device_get_swapchain(context->device));
	vkDestroySwapchainKHR(device, swapchain, pAllocator);
}