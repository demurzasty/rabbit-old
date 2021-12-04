#include "graphics_vulkan.hpp"
#include "utils_vulkan.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace rb;

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

graphics_vulkan::graphics_vulkan() {
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
    _create_main_buffer();
    _create_instance_buffer();
}

graphics_vulkan::~graphics_vulkan() {
    vmaDestroyBuffer(_allocator, _instance_buffer, _instance_buffer_allocation);

    vmaDestroyBuffer(_allocator, _main_buffer, _main_buffer_allocation);

    vkDestroySemaphore(_device, _present_semaphore, nullptr);
    vkDestroySemaphore(_device, _render_semaphore, nullptr);

    vkDestroyCommandPool(_device, _command_pool, nullptr);

    for (auto framebuffer : _swapchain_framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }

    vkDestroyRenderPass(_device, _swapchain_render_pass, nullptr);

    for (auto image_view : _swapchain_image_views) {
        vkDestroyImageView(_device, image_view, nullptr);
    }

    vkDestroyImageView(_device, _swapchain_depth_image_view, nullptr);
    vmaDestroyImage(_allocator, _swapchain_depth_image, _swapchain_depth_image_allocation);

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    vmaDestroyAllocator(_allocator);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

void graphics_vulkan::set_camera_projection_matrix(const mat4f& projection_matrix) {
    _main_data.projection_matrix = projection_matrix;
}

void graphics_vulkan::set_camera_view_matrix(const mat4f& view_matrix) {
    _main_data.view_matrix = view_matrix;
}

void graphics_vulkan::set_camera_world_matrix(const mat4f& world_matrix) {
    _main_data.camera_position = { world_matrix[12], world_matrix[13], world_matrix[14] };
}

instance graphics_vulkan::make_instance() {
    auto instance = graphics_impl::make_instance();
    instance_registry().emplace<instance_data>(instance);
    return instance;
}

void graphics_vulkan::destroy_instance(instance instance) {
    graphics_impl::destroy_instance(instance);
}

void graphics_vulkan::set_instance_mesh(instance instance, const std::shared_ptr<mesh>& mesh) {
}

void graphics_vulkan::set_instance_material(instance instance, const std::shared_ptr<material>& material) {

}

void graphics_vulkan::set_instance_world_matrix(instance instance, const mat4f& world_matrix) {
    instance_registry().get<instance_data>(instance) = {
        world_matrix,
        invert(world_matrix)
    };
}

void graphics_vulkan::draw() {
    _main_data.last_projection_view_matrix = _main_data.projection_view_matrix;
    _main_data.projection_view_matrix = _main_data.projection_matrix * _main_data.view_matrix;
    _main_data.inverse_projection_view_matrix = invert(_main_data.projection_view_matrix);

    void* ptr;
    RB_VK(vmaMapMemory(_allocator, _main_buffer_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, &_main_data, sizeof(_main_data));
    vmaUnmapMemory(_allocator, _main_buffer_allocation);

    RB_VK(vmaMapMemory(_allocator, _instance_buffer_allocation, &ptr), "Failed to map memory");

    instance_data* instances{ static_cast<instance_data*>(ptr) };
    for (const auto& [entity, instance_data] : instance_registry().view<instance_data>().each()) {
        const auto instance_index = static_cast<id_type>(instance_registry().entity(entity));
        if (instance_index < 1024) {
            auto& instance = instances[instance_index];
            instance.world_matrix = instance_data.world_matrix;
            instance.normal_matrix = instance_data.normal_matrix;
        }
    }

    vmaUnmapMemory(_allocator, _instance_buffer_allocation);
}

void graphics_vulkan::swap_buffers() {
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

void graphics_vulkan::_initialize_volk() {
    RB_VK(volkInitialize(), "Cannot initialize volk library.");
}

void graphics_vulkan::_create_instance() {
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = "RabBit";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "RabBit";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

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

void graphics_vulkan::_choose_physical_device() {
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
}

void graphics_vulkan::_create_surface() {
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

void graphics_vulkan::_create_device() {
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
        VK_KHR_SWAPCHAIN_EXTENSION_NAME // Need swapchain to present render result onto a screen.
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

void graphics_vulkan::_create_allocator() {
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.instance = _instance;
    allocator_info.physicalDevice = _physical_device;
    allocator_info.device = _device;
    RB_VK(vmaCreateAllocator(&allocator_info, &_allocator), "Failed to create Vulkan memory allocator");
}

void graphics_vulkan::_query_surface() {
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
    } else {
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

void graphics_vulkan::_create_swapchain() {
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

    if (!settings::graphics.vsync) {
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
    } else {
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
    _swapchain_images.resize(image_count);
    RB_VK(vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, _swapchain_images.data()), "Failed to enumerate swapchain images");

    _swapchain_image_views.resize(_swapchain_images.size());
    for (std::size_t index{ 0 }; index < _swapchain_images.size(); ++index) {
        VkImageViewCreateInfo image_view_info;
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.pNext = nullptr;
        image_view_info.flags = 0;
        image_view_info.image = _swapchain_images[index];
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
        RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_swapchain_image_views[index]), "Failed to create image view");
    }

    const auto depth_format = VK_FORMAT_D32_SFLOAT;

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
    RB_VK(vmaCreateImage(_allocator, &depth_image_info, &depth_allocation_info, &_swapchain_depth_image, &_swapchain_depth_image_allocation, nullptr),
        "Failed to create Vulkan depth image");

    VkImageViewCreateInfo depth_image_view_info{};
    depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_info.image = _swapchain_depth_image;
    depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_info.format = depth_format;
    depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_info.subresourceRange.baseMipLevel = 0;
    depth_image_view_info.subresourceRange.levelCount = 1;
    depth_image_view_info.subresourceRange.baseArrayLayer = 0;
    depth_image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &depth_image_view_info, nullptr, &_swapchain_depth_image_view),
        "Failed to create Vulkan depth image view");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = _surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = depth_format;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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

    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_swapchain_render_pass),
        "Failed to create render pass.");

    _swapchain_framebuffers.resize(_swapchain_image_views.size());
    for (std::size_t index{ 0 }; index < _swapchain_images.size(); ++index) {
        VkImageView framebuffer_attachments[] = { _swapchain_image_views[index], _swapchain_depth_image_view };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _swapchain_render_pass;
        framebuffer_info.attachmentCount = sizeof(framebuffer_attachments) / sizeof(*framebuffer_attachments);
        framebuffer_info.pAttachments = framebuffer_attachments;
        framebuffer_info.width = window_size.x;
        framebuffer_info.height = window_size.y;
        framebuffer_info.layers = 1;

        RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_swapchain_framebuffers[index]),
            "Failed to create framebuffer.");
    }
}

void graphics_vulkan::_create_command_pool() {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = _graphics_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    RB_VK(vkCreateCommandPool(_device, &pool_info, nullptr, &_command_pool), "Failed to create command pool.");
}

void graphics_vulkan::_create_synchronization_objects() {
    VkSemaphoreCreateInfo semaphore_info;
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;

    RB_VK(vkCreateSemaphore(_device, &semaphore_info, VK_NULL_HANDLE, &_render_semaphore), "Failed to create render semaphore");
    RB_VK(vkCreateSemaphore(_device, &semaphore_info, VK_NULL_HANDLE, &_present_semaphore), "Failed to create present semaphore");
    RB_VK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, VK_NULL_HANDLE, &_image_index),
        "Failed to reset acquire next swapchain image");
}

void graphics_vulkan::_create_main_buffer() {
    VkBufferCreateInfo main_buffer_info;
    main_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    main_buffer_info.pNext = nullptr;
    main_buffer_info.flags = 0;
    main_buffer_info.size = sizeof(main_data);
    main_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    main_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    main_buffer_info.queueFamilyIndexCount = 0;
    main_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo main_allocation_info{};
    main_allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    RB_VK(vmaCreateBuffer(_allocator, &main_buffer_info, &main_allocation_info, &_main_buffer, &_main_buffer_allocation, nullptr),
        "Failed to create Vulkan buffer.");
}

void graphics_vulkan::_create_instance_buffer() {
    VkBufferCreateInfo instance_buffer_info;
    instance_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    instance_buffer_info.pNext = nullptr;
    instance_buffer_info.flags = 0;
    instance_buffer_info.size = 1024 * sizeof(instance_data);
    instance_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    instance_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    instance_buffer_info.queueFamilyIndexCount = 0;
    instance_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo instance_allocation_info{};
    instance_allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    RB_VK(vmaCreateBuffer(_allocator, &instance_buffer_info, &instance_allocation_info, &_instance_buffer, &_instance_buffer_allocation, nullptr),
        "Failed to create Vulkan buffer.");
}