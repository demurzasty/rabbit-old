#include "graphics_base_vulkan.hpp"
#include "viewport_vulkan.hpp"
#include "texture_vulkan.hpp"
#include "environment_vulkan.hpp"
#include "material_vulkan.hpp"
#include "mesh_vulkan.hpp"
#include "shaders_vulkan.hpp"
#include "utils_vulkan.hpp"

#include <rabbit/graphics/glsl.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <random>

using namespace rb;

// TODO: Refactor redundant code.
// TODO: Reuse quad vert shader module where possible.
// TODO: Use secondary command buffer to bake shadow map.

namespace {
#if _DEBUG
    const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
#endif

    VKAPI_ATTR
    VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {
        std::fprintf(stderr, "%s\n", pCallbackData->pMessage);
        return VK_FALSE;
    }
}


graphics_base_vulkan::graphics_base_vulkan() {
    _initialize_volk();
    _create_instance();
    _choose_physical_device();
    _create_surface();
    _create_device();
    _create_allocator();
    _query_surface();
    _create_swapchain();
    _create_command_pool();
    _create_synchronization_objects();
    _create_command_buffers();
}

graphics_base_vulkan::~graphics_base_vulkan() {
    vkQueueWaitIdle(_graphics_queue);
    vkQueueWaitIdle(_present_queue);
    vkDeviceWaitIdle(_device);

    vkDestroySemaphore(_device, _present_semaphore, nullptr);
    vkDestroySemaphore(_device, _render_semaphore, nullptr);
    vkDestroyCommandPool(_device, _command_pool, nullptr);

    vkDestroyRenderPass(_device, _render_pass, nullptr);

    for (auto framebuffer : _framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }

    for (auto image_view : _image_views) {
        vkDestroyImageView(_device, image_view, nullptr);
    }

    vkDestroyImageView(_device, _depth_image_view, nullptr);
    vmaDestroyImage(_allocator, _depth_image, _depth_image_allocation);

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    vmaDestroyAllocator(_allocator);
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

std::shared_ptr<texture> graphics_base_vulkan::make_texture(const texture_desc& desc) {
    return std::make_shared<texture_vulkan>(_device, _physical_device_properties, _graphics_queue, _command_pool, _allocator, desc);
}

std::shared_ptr<environment> graphics_base_vulkan::make_environment(const environment_desc& desc) {
    return nullptr;
}

std::shared_ptr<material> graphics_base_vulkan::make_material(const material_desc& desc) {
    return std::make_shared<material_vulkan>(_device, _allocator, desc);
}

void graphics_base_vulkan::present() {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_present_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 0;
    submit_info.pCommandBuffers = nullptr;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_render_semaphore;

    RB_VK(vkQueueSubmit(_graphics_queue, 1, &submit_info, VK_NULL_HANDLE), "Failed to queue submit");

    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;

    // We should wait for rendering execution.
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &_render_semaphore;

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain;

    present_info.pImageIndices = &_image_index;

    present_info.pResults = nullptr;

    RB_VK(vkQueuePresentKHR(_graphics_queue, &present_info), "Failed to queue present");
    RB_VK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, VK_NULL_HANDLE, &_image_index), "Failed to reset acquire next swapchain image");
}

void graphics_base_vulkan::_initialize_volk() {
    RB_VK(volkInitialize(), "Cannot initialize volk library.");
}

void graphics_base_vulkan::_create_instance() {
    const auto [major, minor, patch] = settings::app_version;

    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = settings::window_title.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(major, minor, patch);
    app_info.pEngineName = "RabBit";
    app_info.engineVersion = VK_MAKE_VERSION(RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);
    app_info.apiVersion = VK_API_VERSION_1_2;

    const char* enabled_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if RB_WINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
    };

#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_info;
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.pNext = nullptr;
    debug_info.flags = 0;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = &debug_callback;
    debug_info.pUserData = nullptr;
#endif

    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef _DEBUG
    instance_info.pNext = &debug_info;
#else
    instance_info.pNext = nullptr;
#endif
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
#ifdef _DEBUG
    instance_info.enabledLayerCount = sizeof(validation_layers) / sizeof(*validation_layers);
    instance_info.ppEnabledLayerNames = validation_layers;
#else
    instance_info.enabledLayerCount = 0;
    instance_info.ppEnabledLayerNames = nullptr;
#endif
    instance_info.enabledExtensionCount = sizeof(enabled_extensions) / sizeof(*enabled_extensions);
    instance_info.ppEnabledExtensionNames = enabled_extensions;

    RB_VK(vkCreateInstance(&instance_info, nullptr, &_instance), "Cannot create Vulkan instance.");

    volkLoadInstance(_instance);
}

void graphics_base_vulkan::_choose_physical_device() {
    // Query physical device count. We should pick one.
    std::uint32_t physical_device_count{ 0 };
    RB_VK(vkEnumeratePhysicalDevices(_instance, &physical_device_count, nullptr),
        "Failed to query the number of physical devices.");

    // No supported physical devices?
    RB_ASSERT(physical_device_count > 0, "Couldn't detect any physical device with Vulkan support");

    // Enumerate through physical devices to pick one.
    auto physical_devices = std::make_unique<VkPhysicalDevice[]>(physical_device_count);
    RB_VK(vkEnumeratePhysicalDevices(_instance, &physical_device_count, physical_devices.get()),
        "Failed to enumarate physical devices");

    // TODO: We should find best device.
    _physical_device = physical_devices[0];

    vkGetPhysicalDeviceProperties(_physical_device, &_physical_device_properties);
    
    const auto counts = _physical_device_properties.limits.framebufferColorSampleCounts &
        _physical_device_properties.limits.framebufferDepthSampleCounts;

    //if (counts & VK_SAMPLE_COUNT_64_BIT) { _supported_samples = VK_SAMPLE_COUNT_64_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_32_BIT) { _supported_samples = VK_SAMPLE_COUNT_32_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_16_BIT) { _supported_samples = VK_SAMPLE_COUNT_16_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_8_BIT) { _supported_samples = VK_SAMPLE_COUNT_8_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_4_BIT) { _supported_samples = VK_SAMPLE_COUNT_4_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_2_BIT) { _supported_samples = VK_SAMPLE_COUNT_2_BIT; }
}

void graphics_base_vulkan::_create_surface() {
#if RB_WINDOWS
    // Fill Win32 surface create informations.
    VkWin32SurfaceCreateInfoKHR surface_info;
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.pNext = nullptr;
    surface_info.flags = 0;
    surface_info.hinstance = GetModuleHandle(nullptr);
    surface_info.hwnd = window::native_handle();

    // Create new Vulkan surface.
    RB_VK(vkCreateWin32SurfaceKHR(_instance, &surface_info, nullptr, &_surface), "Failed to create Vulkan surface.");
#else
    RB_ASSERT(false, "Graphics for that platform is not implemented");
#endif
}

void graphics_base_vulkan::_create_device() {
    std::uint32_t queue_family_count{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, nullptr);

    auto queue_families = std::make_unique<VkQueueFamilyProperties[]>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, queue_families.get());

    for (std::uint32_t index{ 0 }; index < queue_family_count; ++index) {
        if (queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            _graphics_family = index;
        }

        VkBool32 present_support{ VK_FALSE };
        vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, index, _surface, &present_support);

        if (present_support == VK_TRUE) {
            _present_family = index;
        }

        if (_graphics_family < UINT32_MAX && _present_family < UINT32_MAX) {
            // TODO: Should break?
            // break;
        }
    }

    // Fill queue priorities array.
    float queue_prorities[] = { 1.0f };

    // Fill graphics device queue create informations.
    VkDeviceQueueCreateInfo device_graphics_queue_info;
    device_graphics_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_graphics_queue_info.pNext = nullptr;
    device_graphics_queue_info.flags = 0;
    device_graphics_queue_info.queueFamilyIndex = _graphics_family;
    device_graphics_queue_info.queueCount = 1;
    device_graphics_queue_info.pQueuePriorities = queue_prorities;

    // Fill present queue create informations.
    VkDeviceQueueCreateInfo device_present_queue_info;
    device_present_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_present_queue_info.pNext = nullptr;
    device_present_queue_info.flags = 0;
    device_present_queue_info.queueFamilyIndex = _present_family;
    device_present_queue_info.queueCount = 1;
    device_present_queue_info.pQueuePriorities = queue_prorities;

    VkDeviceQueueCreateInfo device_queue_infos[] = {
        device_graphics_queue_info,
        device_present_queue_info
    };

    // Fill logical device extensions array.
    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, // Need swapchain to present render result onto a screen.
    };

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(_physical_device, &supported_features);

    // Fill device create informations.
    VkDeviceCreateInfo device_info;
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = nullptr;
    device_info.flags = 0;
#ifdef _DEBUG
    device_info.enabledLayerCount = sizeof(validation_layers) / sizeof(*validation_layers);
    device_info.ppEnabledLayerNames = validation_layers;
#else
    device_info.enabledLayerCount = 0;
    device_info.ppEnabledLayerNames = nullptr;
#endif
    device_info.enabledExtensionCount = sizeof(device_extensions) / sizeof(*device_extensions);
    device_info.ppEnabledExtensionNames = device_extensions;
    device_info.pEnabledFeatures = &supported_features;
    device_info.queueCreateInfoCount = sizeof(device_queue_infos) / sizeof(*device_queue_infos);
    device_info.pQueueCreateInfos = device_queue_infos;

    // Create new Vulkan logical device using physical one.
    RB_VK(vkCreateDevice(_physical_device, &device_info, nullptr, &_device), "Failed to create Vulkan logical device.");

    // Gets logical device queues.
    vkGetDeviceQueue(_device, _graphics_family, 0, &_graphics_queue);
    vkGetDeviceQueue(_device, _present_family, 0, &_present_queue);
}

void graphics_base_vulkan::_create_allocator() {
    // Create Vulkan memory allocator
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.instance = _instance;
    allocator_info.physicalDevice = _physical_device;
    allocator_info.device = _device;

    RB_VK(vmaCreateAllocator(&allocator_info, &_allocator), "Failed to create Vulkan memory allocator");
}

void graphics_base_vulkan::_query_surface() {
    // Query surface format count of picked physical device.
    std::uint32_t surface_format_count{ 0 };
    RB_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &surface_format_count, nullptr),
        "Failed to query surface format count");

    // Enumarate all surface formats.
    //auto surface_formats = std::make_unique<VkSurfaceFormatKHR[]>(surface_format_count);
    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    RB_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &surface_format_count, surface_formats.data()),
        "Failed to enumerate surface formats");

    // Choose surface color format.
    if (surface_format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        _surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else {
        _surface_format.format = surface_formats[0].format;
    }

    _surface_format.colorSpace = surface_formats[0].colorSpace;

    // Query surface capabilities.
    VkSurfaceCapabilitiesKHR surface_capabilities;
    RB_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &surface_capabilities),
        "Failed to retrieve physical device surface capabilities");

    // Store swapchain extent
    _swapchain_extent = surface_capabilities.currentExtent;
}

void graphics_base_vulkan::_create_swapchain() {
    // Get window size.
    const auto window_size = window::size();

    // Query surface capabilities.
    VkSurfaceCapabilitiesKHR surface_capabilities;
    RB_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &surface_capabilities),
        "Failed to retrieve physical device surface capabilities");

    std::uint32_t present_mode_count{ 0 };
    RB_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, nullptr),
        "Failed to query present mode count");

    auto present_modes = std::make_unique<VkPresentModeKHR[]>(present_mode_count);
    RB_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, present_modes.get()),
        "Failed to enumerate present mode count");

    _present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (!settings::vsync) {
        _present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

        // Search for better solution.
        for (std::uint32_t index{ 0 }; index < present_mode_count; ++index) {
            if (present_modes[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
                _present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    auto image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
        image_count = surface_capabilities.maxImageCount;
    }

    std::uint32_t queue_indices[] = { _graphics_family, _present_family };

    VkSwapchainCreateInfoKHR swapchain_info;
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.pNext = nullptr;
    swapchain_info.flags = 0;
    swapchain_info.surface = _surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = _surface_format.format;
    swapchain_info.imageColorSpace = _surface_format.colorSpace;
    swapchain_info.imageExtent = { window_size.x, window_size.y };
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (_graphics_family != _present_family) {
        swapchain_info.queueFamilyIndexCount = sizeof(queue_indices) / sizeof(*queue_indices);
        swapchain_info.pQueueFamilyIndices = queue_indices;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    }
    else {
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapchain_info.presentMode = _present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    // Create Vulkan swapchain.
    RB_VK(vkCreateSwapchainKHR(_device, &swapchain_info, nullptr, &_swapchain), "Failed to create swapchain");

    // Query swapchain image count.
    RB_VK(vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, nullptr), "Failed to query swapchain image count");

    // Get swapchain images list.
    _images.resize(image_count);
    RB_VK(vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, _images.data()), "Failed to enumerate swapchain images");

    _image_views.resize(_images.size());
    for (std::size_t index{ 0 }; index < _images.size(); ++index) {
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
        RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_image_views[index]), "Failed to create image view");
    }

    const auto depth_format = _get_supported_depth_format();

    const VkFormat depth_formats[] = {
        depth_format
    };

    VkImageCreateInfo depth_image_info{};
    depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_info.extent.width = window_size.x;
    depth_image_info.extent.height = window_size.y;
    depth_image_info.extent.depth = 1;
    depth_image_info.mipLevels = 1;
    depth_image_info.arrayLayers = 1;
    depth_image_info.format = depth_format; // VK_FORMAT_D24_UNORM_S8_UINT;
    depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo depth_allocation_info = {};
    depth_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &depth_image_info, &depth_allocation_info, &_depth_image, &_depth_image_allocation, nullptr),
        "Failed to create Vulkan depth image");

    VkImageViewCreateInfo depth_image_view_info{};
    depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_info.image = _depth_image;
    depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_info.format = depth_format;
    depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_info.subresourceRange.baseMipLevel = 0;
    depth_image_view_info.subresourceRange.levelCount = 1;
    depth_image_view_info.subresourceRange.baseArrayLayer = 0;
    depth_image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &depth_image_view_info, nullptr, &_depth_image_view),
        "Failed to create Vulkan depth image view");

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
    depth_attachment.format = depth_format;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolve_reference{};
    resolve_reference.attachment = 1;
    resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_reference;
    subpass.pResolveAttachments = nullptr;

    VkSubpassDependency subpass_dependencies[2]{};
    subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[0].dstSubpass = 0;
    subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_dependencies[1].srcSubpass = 0;
    subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[0].dstSubpass = 0;
    subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; // Both stages might have access the depth-buffer, so need both in src/dstStageMask;;
    subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_dependencies[1].srcSubpass = 0;
    subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


    subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[0].dstSubpass = 0;
    subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; // Both stages might have access the depth-buffer, so need both in src/dstStageMask;;
    subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_dependencies[1].srcSubpass = 0;
    subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = sizeof(attachments) / sizeof(*attachments);
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = subpass_dependencies;

    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass),
        "Failed to create render pass.");

    _framebuffers.resize(_image_views.size());
    for (std::size_t index{ 0 }; index < _images.size(); ++index) {
        VkImageView framebuffer_attachments[] = { _image_views[index], _depth_image_view };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _render_pass;
        framebuffer_info.attachmentCount = sizeof(framebuffer_attachments) / sizeof(*framebuffer_attachments);
        framebuffer_info.pAttachments = framebuffer_attachments;
        framebuffer_info.width = window_size.x;
        framebuffer_info.height = window_size.y;
        framebuffer_info.layers = 1;

        RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_framebuffers[index]),
            "Failed to create framebuffer.");
    }
}

void graphics_base_vulkan::_create_command_pool() {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = _graphics_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    RB_VK(vkCreateCommandPool(_device, &pool_info, nullptr, &_command_pool), "Failed to create command pool.");
}

void graphics_base_vulkan::_create_synchronization_objects() {
    VkSemaphoreCreateInfo semaphore_info;
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;

    RB_VK(vkCreateSemaphore(_device, &semaphore_info, VK_NULL_HANDLE, &_render_semaphore), "Failed to create render semaphore");
    RB_VK(vkCreateSemaphore(_device, &semaphore_info, VK_NULL_HANDLE, &_present_semaphore), "Failed to create present semaphore");
    RB_VK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, VK_NULL_HANDLE, &_image_index),
        "Failed to reset acquire next swapchain image");
}

void graphics_base_vulkan::_create_command_buffers() {
    VkCommandBufferAllocateInfo command_buffer_alloc_info;
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = _command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = max_command_buffers;

    RB_VK(vkAllocateCommandBuffers(_device, &command_buffer_alloc_info, _command_buffers),
        "Failed to allocate command buffer");

    VkFenceCreateInfo fence_info;
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& fence : _fences) {
        RB_VK(vkCreateFence(_device, &fence_info, nullptr, &fence), "Failed to create fence");
    }
}

VkFormat graphics_base_vulkan::_get_supported_depth_format() {
    VkFormat depth_formats[]{
       // VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
     //   VK_FORMAT_D16_UNORM_S8_UINT,
     //   VK_FORMAT_D16_UNORM
    };

    for (auto& format : depth_formats) {
        if (_is_format_supported(format, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat graphics_base_vulkan::_get_supported_shadow_format() {
    VkFormat depth_formats[]{
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
    };

    for (auto& format : depth_formats) {
        if (_is_format_supported(format, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

bool graphics_base_vulkan::_is_format_supported(VkFormat format, VkFormatFeatureFlags flags) {
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(_physical_device, format, &format_props);
    return (format_props.optimalTilingFeatures & flags) == flags;
}

void graphics_base_vulkan::_create_buffer(VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, std::uint32_t size, VkBuffer* buffer, VmaAllocation* allocation) {
    VkBufferCreateInfo buffer_info;
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = nullptr;
    buffer_info.flags = 0;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 0;
    buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo buffer_allocation_info{};
    buffer_allocation_info.usage = memory_usage;
    // buffer_allocation_info.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    RB_VK(vmaCreateBuffer(_allocator, &buffer_info, &buffer_allocation_info, buffer, allocation, nullptr),
        "Failed to create Vulkan buffer.");
}

void graphics_base_vulkan::_create_descriptor_set_layout(const span<const VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayoutCreateFlags flags, VkDescriptorSetLayout* layout) {
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = flags;
    descriptor_set_layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
    descriptor_set_layout_info.pBindings = bindings.data();
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, layout),
        "Failed to create Vulkan descriptor set layout");
}

void graphics_base_vulkan::_create_shader_module_from_code(shader_stage stage, const span<const std::uint8_t>& code, const std::vector<std::string>& definitions, VkShaderModule* module) {
    std::string str;
    str.resize(code.size());
    std::memcpy(&str[0], code.data(), code.size());
    _create_shader_module(glsl::compile(stage, str, definitions), module);
}

void graphics_base_vulkan::_create_shader_module(const span<const std::uint32_t>& spirv, VkShaderModule* module) {
    VkShaderModuleCreateInfo shader_module_info;
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.pNext = nullptr;
    shader_module_info.flags = 0;
    shader_module_info.codeSize = spirv.size_bytes();
    shader_module_info.pCode = spirv.data();
    RB_VK(vkCreateShaderModule(_device, &shader_module_info, nullptr, module), "Failed to create shader module");
}

void graphics_base_vulkan::_create_compute_pipeline(VkShaderModule shader_module, VkPipelineLayout layout, VkPipeline* pipeline) {
    VkComputePipelineCreateInfo compute_pipeline_create_info;
    compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.pNext = nullptr;
    compute_pipeline_create_info.flags = 0;
    compute_pipeline_create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compute_pipeline_create_info.stage.pNext = nullptr;
    compute_pipeline_create_info.stage.flags = 0;
    compute_pipeline_create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compute_pipeline_create_info.stage.module = shader_module;
    compute_pipeline_create_info.stage.pName = "main";
    compute_pipeline_create_info.stage.pSpecializationInfo = nullptr;
    compute_pipeline_create_info.layout = layout;
    compute_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    compute_pipeline_create_info.basePipelineIndex = 0;
    RB_VK(vkCreateComputePipelines(_device, nullptr, 1, &compute_pipeline_create_info, nullptr, pipeline),
        "Failed to create Vulkan compute pipeline");
}

void graphics_base_vulkan::_buffer_barrier(VkCommandBuffer command_buffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkAccessFlags src_access, VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage) {
    VkBufferMemoryBarrier barrier;
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = src_access;
    barrier.dstAccessMask = dst_access;
    barrier.srcQueueFamilyIndex = _graphics_family;
    barrier.dstQueueFamilyIndex = _graphics_family;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;
    vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void graphics_base_vulkan::_create_image(VkImageType type, VkFormat format, VkExtent3D extent, std::uint32_t mipmaps, std::uint32_t layers, VkImageUsageFlags usage, VkImageLayout layout, VmaMemoryUsage memory_usage, VkImage* image, VmaAllocation* allocation) {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = type;
    image_info.format = format;
    image_info.extent = extent;
    image_info.mipLevels = mipmaps;
    image_info.arrayLayers = layers;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = layout;

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = memory_usage;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, image, allocation, nullptr),
        "Failed to create Vulkan image.");
}

void graphics_base_vulkan::_create_image_view(VkImage image, VkImageViewType type, VkFormat format, VkImageSubresourceRange range, VkImageView* image_view) {
    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = image;
    image_view_info.viewType = type;
    image_view_info.format = format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
    image_view_info.subresourceRange = range;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, image_view),
        "Failed to create Vulkan image view");
}

void graphics_base_vulkan::_create_sampler(VkFilter filter, VkSamplerMipmapMode mipmap_mode, VkSamplerAddressMode address_mode, float anisotropy, float max_lod, bool min_reduction, VkSampler* sampler) {
    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = filter;
    sampler_info.minFilter = filter;
    sampler_info.mipmapMode = mipmap_mode;
    sampler_info.addressModeU = address_mode;
    sampler_info.addressModeV = address_mode;
    sampler_info.addressModeW = address_mode;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = anisotropy;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = max_lod;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    VkSamplerReductionModeCreateInfoEXT reduction_info{ VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT };

    if (min_reduction) {
        reduction_info.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
        reduction_info.reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
        sampler_info.pNext = &reduction_info;
    }

    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, sampler), "Failed to create Vulkan sampler");
}

VkCommandBuffer graphics_base_vulkan::_command_begin() {
    _command_index = (_command_index + 1) % max_command_buffers;

    RB_VK(vkWaitForFences(_device, 1, &_fences[_command_index], VK_TRUE, 1000000000),
        "Failed to wait for render fence");

    RB_VK(vkResetFences(_device, 1, &_fences[_command_index]), "Failed to reset render fence");

    // Now that we are sure that the commands finished executing,
    // we can safely reset the command buffer to begin recording again.
    RB_VK(vkResetCommandBuffer(_command_buffers[_command_index], 0), "Failed to reset command buffer");

    // Begin the command buffer recording.
    // We will use this command buffer exactly once, so we want to let Vulkan know that.
    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;

    RB_VK(vkBeginCommandBuffer(_command_buffers[_command_index], &begin_info), "Failed to begin command buffer");
    return _command_buffers[_command_index];
}

void graphics_base_vulkan::_command_end() {
    // Finalize the command buffer (we can no longer add commands, but it can now be executed)
    RB_VK(vkEndCommandBuffer(_command_buffers[_command_index]), "Failed to end command buffer");

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &_command_buffers[_command_index];
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    RB_VK(vkQueueSubmit(_graphics_queue, 1, &submit_info, _fences[_command_index]), "Failed to queue submit");
}