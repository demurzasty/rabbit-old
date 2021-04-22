#include "graphics_device_vulkan.hpp"
#include "buffer_vulkan.hpp"
#include "shader_vulkan.hpp"
#include "shader_data_vulkan.hpp"
#include "texture_vulkan.hpp"

#include <rabbit/core/config.hpp>
#include <rabbit/core/version.hpp>
#include <rabbit/core/range.hpp>

#include <rabbit/generated/shaders/forward.vert.spv.h>
#include <rabbit/generated/shaders/forward.frag.spv.h>

#if RB_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

using namespace rb;

namespace {
    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        fprintf(stderr, "%s\n", pCallbackData->pMessage);
        return VK_FALSE;
    }
}

graphics_device_vulkan::graphics_device_vulkan(application_config& config, window& window) {
    RB_MAYBE_UNUSED VkResult result;

    result = volkInitialize();
    RB_ASSERT(result == VK_SUCCESS, "Cannot initialize volk library");

    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = config.window.title.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "RabBit";
    app_info.engineVersion = VK_MAKE_VERSION(RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);
    app_info.apiVersion = VK_API_VERSION_1_0;

    const char* enabled_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if RB_WINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
    };

    const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_info;
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.pNext = nullptr;
    debug_info.flags = 0;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = &debug_callback;
    debug_info.pUserData = nullptr;

    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = &debug_info;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = sizeof(validation_layers) / sizeof(*validation_layers);
    instance_info.ppEnabledLayerNames = validation_layers;
    instance_info.enabledExtensionCount = sizeof(enabled_extensions) / sizeof(*enabled_extensions);
    instance_info.ppEnabledExtensionNames = enabled_extensions;

    result = vkCreateInstance(&instance_info, nullptr, &_instance);
    RB_ASSERT(result == VK_SUCCESS, "Cannot create Vulkan instance");

    volkLoadInstance(_instance);

    // Query physical device count. We should pick one.
    std::uint32_t physical_device_count{ 0 };
    result = vkEnumeratePhysicalDevices(_instance, &physical_device_count, nullptr);
    RB_ASSERT(result == VK_SUCCESS, "Failed to query the number of physical devices");

    // No supported physical devices?
    RB_ASSERT(physical_device_count > 0, "Couldn't detect any physical device with Vulkan support");

    // Enumerate through physical devices to pick one.
    auto physical_devices = std::make_unique<VkPhysicalDevice[]>(physical_device_count);
    result = vkEnumeratePhysicalDevices(_instance, &physical_device_count, physical_devices.get());
    RB_ASSERT(result == VK_SUCCESS, "Failed to enumarate physical devices");

    // Pick first one. TODO: We should find best device.
    _physical_device = physical_devices[0];

#if RB_WINDOWS
    // Fill Win32 surface create informations.
    VkWin32SurfaceCreateInfoKHR surface_info;
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.pNext = nullptr;
    surface_info.flags = 0;
    surface_info.hinstance = GetModuleHandle(nullptr);
    surface_info.hwnd = window.native_handle();

    // Create new Vulkan surface.
    result = vkCreateWin32SurfaceKHR(_instance, &surface_info, nullptr, &_surface);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan surface");
#else
    RB_ASSERT(false, "Graphics platform is not implemented");
#endif

    std::uint32_t graphics_family{ UINT32_MAX };
    std::uint32_t present_family{ UINT32_MAX };

    std::uint32_t queue_family_count{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, nullptr);

    auto queue_families = std::make_unique<VkQueueFamilyProperties[]>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, queue_families.get());

    for (auto index : rb::make_range(0u, queue_family_count)) {
        if (queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = index;
        }

        VkBool32 present_support{ VK_FALSE };
        vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, index, _surface, &present_support);

        if (present_support == VK_TRUE) {
            present_family = index;
        }

        if (graphics_family < UINT32_MAX && present_family < UINT32_MAX) {
        //    break;
        }
    }

    // Fill queue priorities array.
    float queue_prorities[] = { 1.0f };


    // Fill graphics device queue create informations.
    VkDeviceQueueCreateInfo device_graphics_queue_info;
    device_graphics_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_graphics_queue_info.pNext = nullptr;
    device_graphics_queue_info.flags = 0;
    device_graphics_queue_info.queueFamilyIndex = graphics_family;
    device_graphics_queue_info.queueCount = 1;
    device_graphics_queue_info.pQueuePriorities = queue_prorities;

    // Fill present queue create informations.
    VkDeviceQueueCreateInfo device_present_queue_info;
    device_present_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_present_queue_info.pNext = nullptr;
    device_present_queue_info.flags = 0;
    device_present_queue_info.queueFamilyIndex = present_family;
    device_present_queue_info.queueCount = 1;
    device_present_queue_info.pQueuePriorities = queue_prorities;

    VkDeviceQueueCreateInfo device_queue_infos[] = {
        device_graphics_queue_info,
        device_present_queue_info
    };

    // Fill logical device extensions array.
    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // Fill device create informations.
    VkDeviceCreateInfo device_info;
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = nullptr;
    device_info.flags = 0;
    device_info.enabledLayerCount = sizeof(validation_layers) / sizeof(*validation_layers);
    device_info.ppEnabledLayerNames = validation_layers;
    device_info.enabledExtensionCount = sizeof(device_extensions) / sizeof(*device_extensions);
    device_info.ppEnabledExtensionNames = device_extensions;
    device_info.pEnabledFeatures = nullptr;
    device_info.queueCreateInfoCount = sizeof(device_queue_infos) / sizeof(*device_queue_infos);
    device_info.pQueueCreateInfos = device_queue_infos;

    // Create new Vulkan logical device using physical one.
    result = vkCreateDevice(_physical_device, &device_info, nullptr, &_device);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan logical device");

    // Gets logical device queues.
    vkGetDeviceQueue(_device, graphics_family, 0, &_graphics_queue);
    vkGetDeviceQueue(_device, present_family, 0, &_present_queue);

    // Create Vulkan memory allocator
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.instance = _instance;
    allocator_info.physicalDevice = _physical_device;
    allocator_info.device = _device;

    result = vmaCreateAllocator(&allocator_info, &_allocator);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan memory allocator");

    // Query surface format count of picked physical device.
    std::uint32_t surface_format_count{ 0 };
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &surface_format_count, nullptr);
    RB_ASSERT(result == VK_SUCCESS, "Failed to query surface format count");

    // Enumarate all surface formats.
    auto surface_formats = std::make_unique<VkSurfaceFormatKHR[]>(surface_format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &surface_format_count, surface_formats.get());
    RB_ASSERT(result == VK_SUCCESS, "Failed to enumerate surface formats");

    // Choose surface color format.
    if (surface_format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        _surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        _surface_format.format = surface_formats[0].format;
    }

    _surface_format.colorSpace = surface_formats[0].colorSpace;

    // Query surface capabilities.
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &surface_capabilities);
    RB_ASSERT(result == VK_SUCCESS, "Failed to retrieve physical device surface capabilities");

    // Store swapchain extent
    _swapchain_extent = surface_capabilities.currentExtent;

    std::uint32_t present_mode_count{ 0 };
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, nullptr);
    RB_ASSERT(result == VK_SUCCESS, "Failed to query present mode count");

    auto present_modes = std::make_unique<VkPresentModeKHR[]>(present_mode_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, present_modes.get());
    RB_ASSERT(result == VK_SUCCESS, "Failed to enumerate present mode count");

    _present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;

    for (auto index : rb::make_range(0u, present_mode_count)) {
        if (present_modes[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
            _present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    auto image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
        image_count = surface_capabilities.maxImageCount;
    }

    std::uint32_t queue_indices[] = { graphics_family, present_family };

    VkSwapchainCreateInfoKHR swapchain_info;
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.pNext = nullptr;
    swapchain_info.flags = 0;
    swapchain_info.surface = _surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = _surface_format.format;
    swapchain_info.imageColorSpace = _surface_format.colorSpace;
    swapchain_info.imageExtent = _swapchain_extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphics_family != present_family) {
        swapchain_info.queueFamilyIndexCount = sizeof(queue_indices) / sizeof(*queue_indices);
        swapchain_info.pQueueFamilyIndices = queue_indices;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    } else {
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapchain_info.presentMode = _present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.oldSwapchain = nullptr;

    // Create Vulkan swapchain.
    result = vkCreateSwapchainKHR(_device, &swapchain_info, nullptr, &_swapchain);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create swapchain");

    // Query swapchain image count.
    result = vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, nullptr);
    RB_ASSERT(result == VK_SUCCESS, "Failed to query swapchain image count");

    // Get swapchain images list.
    _images.resize(image_count);
    result = vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, _images.data());
    RB_ASSERT(result == VK_SUCCESS, "Failed to enumerate swapchain images");

    _image_views.resize(_images.size());
    for (auto index : rb::make_range<std::size_t>(0u, _images.size())) {
        VkImageViewCreateInfo image_view_info;
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.pNext = nullptr;
        image_view_info.flags = 0;
        image_view_info.image = _images[index];
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = _surface_format.format;
        image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount = 1;

        result = vkCreateImageView(_device, &image_view_info, nullptr, &_image_views[index]);
        RB_ASSERT(result == VK_SUCCESS, "Failed to create image view");
    }

    const VkFormat depth_formats[] = {
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    VkImageCreateInfo depth_image_info{};
    depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_info.extent.width = _swapchain_extent.width;
    depth_image_info.extent.height = _swapchain_extent.height;
    depth_image_info.extent.depth = 1;
    depth_image_info.mipLevels = 1;
    depth_image_info.arrayLayers = 1;
    depth_image_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo depth_allocation_info = {};
	depth_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    result = vmaCreateImage(_allocator, &depth_image_info, &depth_allocation_info, &_depth_image, &_depth_image_allocation, nullptr);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan depth image");

    VkImageViewCreateInfo depth_image_view_info{};
    depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_info.image = _depth_image;
    depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_info.subresourceRange.baseMipLevel = 0;
    depth_image_view_info.subresourceRange.levelCount = 1;
    depth_image_view_info.subresourceRange.baseArrayLayer = 0;
    depth_image_view_info.subresourceRange.layerCount = 1;

    result = vkCreateImageView(_device, &depth_image_view_info, nullptr, &_depth_image_view);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan depth image view");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = _surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_reference;

    VkSubpassDependency subpass_dependency{};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = sizeof(attachments) / sizeof(*attachments);
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &subpass_dependency;

    result = vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create render pass");

    _framebuffers.resize(_image_views.size());
    for (auto index : rb::make_range<std::size_t>(0u, _images.size())) {
        VkImageView framebuffer_attachments[] = { _image_views[index], _depth_image_view };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _render_pass;
        framebuffer_info.attachmentCount = sizeof(framebuffer_attachments) / sizeof(*framebuffer_attachments);
        framebuffer_info.pAttachments = framebuffer_attachments;
        framebuffer_info.width = _swapchain_extent.width;
        framebuffer_info.height = _swapchain_extent.height;
        framebuffer_info.layers = 1;

        result = vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_framebuffers[index]);
        RB_ASSERT(result == VK_SUCCESS, "Failed to create framebuffer");
    }

    VkDescriptorPoolSize descriptor_pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info{};
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.flags = 0;
	descriptor_pool_info.maxSets = 10;
	descriptor_pool_info.poolSizeCount = sizeof(descriptor_pool_sizes) / sizeof(*descriptor_pool_sizes);
	descriptor_pool_info.pPoolSizes = descriptor_pool_sizes;

    result = vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_descriptor_pool);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool");

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = graphics_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    result = vkCreateCommandPool(_device, &pool_info, nullptr, &_command_pool);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create command pool");

    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.commandPool = _command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(_device, &command_buffer_alloc_info, &_command_buffer);
    RB_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffer");

    VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.pNext = nullptr;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    result = vkCreateFence(_device, &fence_info, nullptr, &_render_fence);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create fence");

    VkSemaphoreCreateInfo semaphore_info;
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.pNext = nullptr;
	semaphore_info.flags = 0;

	result = vkCreateSemaphore(_device, &semaphore_info, nullptr, &_render_semaphore);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create render semaphore");

	result = vkCreateSemaphore(_device, &semaphore_info, nullptr, &_present_semaphore);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create present semaphore");

    result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, nullptr, &_image_index);
    RB_ASSERT(result == VK_SUCCESS, "Failed to reset acquire next swapchain image");
}

graphics_device_vulkan::~graphics_device_vulkan() {
    vkWaitForFences(_device, 1, &_render_fence, true, 1000000000);

    vkDeviceWaitIdle(_device);

    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);

    vkDestroyRenderPass(_device, _render_pass, nullptr);

    vmaDestroyImage(_allocator, _depth_image, _depth_image_allocation);

    for (auto framebuffer : _framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }

    vkFreeCommandBuffers(_device, _command_pool, 1, &_command_buffer);

    for (auto image_view : _image_views) {
        vkDestroyImageView(_device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    vkDestroySemaphore(_device, _present_semaphore, nullptr);
    vkDestroySemaphore(_device, _render_semaphore, nullptr);
    vkDestroyFence(_device, _render_fence, nullptr);

    vkFreeCommandBuffers(_device, _command_pool, 1, &_command_buffer);
    vkDestroyCommandPool(_device, _command_pool, nullptr);
    vmaDestroyAllocator(_allocator);
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

std::shared_ptr<buffer> graphics_device_vulkan::make_buffer(const buffer_desc& desc) {
    return std::make_shared<buffer_vulkan>(_physical_device, _device, _allocator, desc);
}

std::shared_ptr<mesh> graphics_device_vulkan::make_mesh(const mesh_desc& desc) {
    return std::make_shared<mesh>(desc);
}

std::shared_ptr<material> graphics_device_vulkan::make_material(const material_desc& desc) {
    return std::make_shared<material>(desc);
}

std::shared_ptr<shader> graphics_device_vulkan::make_shader(const shader_desc& desc) {
    return std::make_shared<shader_vulkan>(_device, _render_pass, _swapchain_extent, desc);
}

std::shared_ptr<shader> graphics_device_vulkan::make_shader(builtin_shader builtin_shader) {
        // material_desc material_desc;
    // material_desc.vertex_desc = {
    //     { vertex_attribute::position, vertex_format::vec3f() },
    //     { vertex_attribute::texcoord, vertex_format::vec2f() },
    //     { vertex_attribute::normal, vertex_format::vec3f() },
    // };
    // material_desc.vertex_bytecode = forward_vert;
    // material_desc.fragment_bytecode = forward_frag;
    // material_desc.bindings = {
    //     { material_binding_type::uniform_buffer, shader_stage_flags::vertex, 0, 1 },
    //     { material_binding_type::uniform_buffer, shader_stage_flags::vertex, 1, 1 },
    //     { material_binding_type::texture, shader_stage_flags::fragment, 2, 1 },
    // };

    shader_desc desc;
    desc.vertex_desc  = {
        { vertex_attribute::position, vertex_format::vec3f() },
        { vertex_attribute::texcoord, vertex_format::vec2f() },
        { vertex_attribute::normal, vertex_format::vec3f() },
    };
    desc.vertex_bytecode = forward_vert;
    desc.fragment_bytecode = forward_frag;
    desc.bindings = {
        { shader_binding_type::uniform_buffer, shader_stage_flags::vertex, 0, 1 },
        { shader_binding_type::uniform_buffer, shader_stage_flags::vertex, 1, 1 },
        { shader_binding_type::uniform_buffer, shader_stage_flags::fragment, 2, 1 },
        { shader_binding_type::texture, shader_stage_flags::fragment, 3, 1 },
    };

    return make_shader(desc);
}

std::shared_ptr<shader_data> graphics_device_vulkan::make_shader_data(const shader_data_desc& desc) {
    return std::make_shared<shader_data_vulkan>(_device, desc);
}

std::shared_ptr<texture> graphics_device_vulkan::make_texture(const texture_desc& desc) {
    return std::make_shared<texture_vulkan>(_device, _graphics_queue, _command_pool, _allocator, desc);
}

void graphics_device_vulkan::begin() {
    // Wait until the GPU has finished executing the last commands. Timeout of 1 second.
    RB_MAYBE_UNUSED auto result = vkWaitForFences(_device, 1, &_render_fence, true, 1000000000);
    RB_ASSERT(result == VK_SUCCESS, "Failed to wait for render fence");

	result = vkResetFences(_device, 1, &_render_fence);
    RB_ASSERT(result == VK_SUCCESS, "Failed to reset render fence");

    // Now that we are sure that the commands finished executing,
    // we can safely reset the command buffer to begin recording again.
    result = vkResetCommandBuffer(_command_buffer, 0);
    RB_ASSERT(result == VK_SUCCESS, "Failed to reset command buffer");

    // Begin the command buffer recording.
    // We will use this command buffer exactly once, so we want to let Vulkan know that.
    VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
    begin_info.pInheritanceInfo = nullptr;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(_command_buffer, &begin_info);
    RB_ASSERT(result == VK_SUCCESS, "Failed to begin command buffer");
}

void graphics_device_vulkan::end() {
	// Finalize the command buffer (we can no longer add commands, but it can now be executed)
	RB_MAYBE_UNUSED auto result = vkEndCommandBuffer(_command_buffer);
    RB_ASSERT(result == VK_SUCCESS, "Failed to end command buffer");

    // Prepare the submission to the queue.
	// We want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	// We will signal the _renderSemaphore, to signal that rendering has finished

	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	submit.pWaitDstStageMask = &wait_stage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &_present_semaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &_render_semaphore;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &_command_buffer;

	// Submit command buffer to the queue and execute it.
	// Render fence will now block until the graphic commands finish execution
	result = vkQueueSubmit(_graphics_queue, 1, &submit, _render_fence);
    RB_ASSERT(result == VK_SUCCESS, "Failed to queue submit");
}

void graphics_device_vulkan::begin_render_pass() {
	VkClearValue clear_values[2];
	clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clear_values[1].depthStencil = { 1.0f, 0 };

	// Start the main renderpass.
	// We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = nullptr;
	render_pass_begin_info.renderPass = _render_pass;
	render_pass_begin_info.renderArea.offset = { 0, 0 };
	render_pass_begin_info.renderArea.extent = _swapchain_extent;
	render_pass_begin_info.framebuffer = _framebuffers[_image_index];
	render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
	render_pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void graphics_device_vulkan::end_render_pass() {
    // Finalize the render pass
	vkCmdEndRenderPass(_command_buffer);
}

void graphics_device_vulkan::update_buffer(const std::shared_ptr<buffer>& buffer, const void* data, std::size_t offset, std::size_t size) {
    auto native_buffer = std::static_pointer_cast<buffer_vulkan>(buffer)->buffer();
    vkCmdUpdateBuffer(_command_buffer, native_buffer, offset, size, data);
}

void graphics_device_vulkan::set_viewport(const vec4i& rect) {
    VkViewport viewport;
    viewport.x = static_cast<float>(rect.x);
    viewport.y = static_cast<float>(rect.y);
    viewport.width = static_cast<float>(rect.z);
    viewport.height = static_cast<float>(rect.w);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(_command_buffer, 0, 1, &viewport);
}

void graphics_device_vulkan::draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<shader>& shader, const std::shared_ptr<shader_data>& shader_data) {
    RB_ASSERT(mesh, "Mesh is not provided");
    RB_ASSERT(shader, "Shader is not provided");
    RB_ASSERT(shader_data, "Shader data is not provided");

    auto native_shader = std::static_pointer_cast<shader_vulkan>(shader);
    auto native_shader_data = std::static_pointer_cast<shader_data_vulkan>(shader_data);
    // auto native_resource_heap = std::static_pointer_cast<resource_heap_vulkan>(resource_heap);

    vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, native_shader->pipeline());

    vkCmdBindDescriptorSets(_command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        native_shader->pipeline_layout(),
        0,
        1,
        &native_shader_data->descriptor_sets()[0],
        0,
        nullptr);

    auto buffer = std::static_pointer_cast<buffer_vulkan>(mesh->vertex_buffer())->buffer();
    VkDeviceSize offset = 0;

    vkCmdBindVertexBuffers(_command_buffer, 0, 1, &buffer, &offset);
    vkCmdDraw(_command_buffer, static_cast<std::uint32_t>(mesh->vertex_buffer()->count()), 1, 0, 0);
}

void graphics_device_vulkan::present() {
    RB_MAYBE_UNUSED VkResult result;

    VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;

	present_info.pSwapchains = &_swapchain;
	present_info.swapchainCount = 1;

	present_info.pWaitSemaphores = &_render_semaphore;
	present_info.waitSemaphoreCount = 1;

	present_info.pImageIndices = &_image_index;

	result = vkQueuePresentKHR(_graphics_queue, &present_info);
    RB_ASSERT(result == VK_SUCCESS, "Failed to queue present");

    result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, nullptr, &_image_index);
    RB_ASSERT(result == VK_SUCCESS, "Failed to reset acquire next swapchain image");

    _current_frame++;
}
