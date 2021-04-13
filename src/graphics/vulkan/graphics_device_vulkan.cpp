#include "graphics_device_vulkan.hpp"
#include "buffer_vulkan.hpp"
#include "shader_vulkan.hpp"
#include "texture_cube_vulkan.hpp"
#include "texture_vulkan.hpp"

#include <rabbit/core/exception.hpp>
#include <rabbit/graphics/builtin_shaders.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>
#include <memory>
#include <iostream>

using namespace rb;

constexpr std::size_t max_frames_in_flight = 2;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static VkShaderModule create_shader_module(VkDevice device, const span<const std::uint8_t>& bytecode) {
	VkShaderModuleCreateInfo shader_module_info;
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.pNext = nullptr;
	shader_module_info.flags = 0;
	shader_module_info.codeSize = bytecode.size();
	shader_module_info.pCode = reinterpret_cast<const std::uint32_t*>(bytecode.data());

	VkShaderModule shader_module;
	if (vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module) != VK_SUCCESS) {
		throw make_exception("Failed to create shader module");
	}

	return shader_module;
}

static std::uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw make_exception("Failed to find suitable memory type!");
}

graphics_device_vulkan::graphics_device_vulkan(const config& config, window& window) {
    if (volkInitialize() != VK_SUCCESS) {
        throw make_exception("Cannot initialize volk libary");
    }

    // Fill application informations.
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = config.window.title.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "RabBit";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // Fill needed extensions.
    const char* enabled_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if RB_WINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
    };

    // Fill needed validation layers.
    const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    // Fill debug utils messenger create informations.
    VkDebugUtilsMessengerCreateInfoEXT debug_info;
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.pNext = nullptr;
    debug_info.flags = 0;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = &debug_callback;
    debug_info.pUserData = nullptr;
    
    // Fill instance create informations.
    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = &debug_info;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = sizeof(validation_layers) / sizeof(*validation_layers);
    instance_info.ppEnabledLayerNames = validation_layers;
    instance_info.enabledExtensionCount = sizeof(enabled_extensions) / sizeof(*enabled_extensions);
    instance_info.ppEnabledExtensionNames = enabled_extensions;

    // Create new Vulkan instance.
    if (vkCreateInstance(&instance_info, nullptr, &_instance) != VK_SUCCESS) {
        throw make_exception("Cannot create Vulkan instance");
    }

    // Load Vulkan functions.
    volkLoadInstance(_instance);
    
    // Query physical device count. We should pick one.
    std::uint32_t physical_device_count = 0;
    if (vkEnumeratePhysicalDevices(_instance, &physical_device_count, nullptr) != VK_SUCCESS) {
        throw make_exception("Failed to query the number of physical devices");
    }

    // No supported physical devices?
    if (physical_device_count == 0) {
        throw make_exception("Couldn't detect any physical device with Vulkan support");
    }

    // Enumerate through physical devices to pick one. 
    auto physical_devices = std::make_unique<VkPhysicalDevice[]>(physical_device_count);
    if (vkEnumeratePhysicalDevices(_instance, &physical_device_count, physical_devices.get()) != VK_SUCCESS) {
        throw make_exception("Failed to enumarate physical devices");
    }

    // Pick first. TODO: We should pick best one.
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
    if (vkCreateWin32SurfaceKHR(_instance, &surface_info, nullptr, &_surface) != VK_SUCCESS) {
        throw make_exception("Failed to create Vulkan surface");
    }
#else
    throw make_exception("No suitable platform");
#endif

    std::uint32_t graphics_family{ UINT32_MAX };
    std::uint32_t present_family{ UINT32_MAX };

    std::uint32_t queue_family_count{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, nullptr);

    auto queue_families = std::make_unique<VkQueueFamilyProperties[]>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, queue_families.get());
    
    for (std::uint32_t index{ 0 }; index < queue_family_count; ++index) {
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
    if (vkCreateDevice(_physical_device, &device_info, nullptr, &_device) != VK_SUCCESS) {
        throw make_exception("Failed creating logical device");
    }

    // Gets logical device queues.
    vkGetDeviceQueue(_device, graphics_family, 0, &_graphics_queue);
    vkGetDeviceQueue(_device, present_family, 0, &_present_queue);

    // Query surface format count of picked physical device.
    std::uint32_t surface_format_count = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &surface_format_count, nullptr) != VK_SUCCESS) {
        throw make_exception("Failed to query surface format count");
    }

    // Enumarate all surface formats.
    auto surface_formats = std::make_unique<VkSurfaceFormatKHR[]>(surface_format_count);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &surface_format_count, surface_formats.get()) != VK_SUCCESS) {
        throw make_exception("Failed to enumerate surface formats");
    }

    // Choose surface color format.
    if (surface_format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        _color_format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        _color_format = surface_formats[0].format;
    }

    // Choose surface color space.
    _color_space = surface_formats[0].colorSpace;

    // Query surface capabilities.
    VkSurfaceCapabilitiesKHR surface_capabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &surface_capabilities) != VK_SUCCESS) {
        throw make_exception("Failetd to retrieve physical device surface capabilities");
    }

    if (surface_capabilities.currentExtent.width != UINT32_MAX) {
        _swap_extent = surface_capabilities.currentExtent;
    } else {
        _swap_extent = {
            static_cast<std::uint32_t>(config.window.size.x),
            static_cast<std::uint32_t>(config.window.size.y)
        };

        _swap_extent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, _swap_extent.width));
        _swap_extent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, _swap_extent.height));
    }

    std::uint32_t present_mode_count = 0;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, nullptr) != VK_SUCCESS) {
        throw make_exception("Failed to query present mode count");
    }

    auto present_modes = std::make_unique<VkPresentModeKHR[]>(present_mode_count);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, present_modes.get()) != VK_SUCCESS) {
        throw make_exception("Failed to enumerate present mode count");
    }

    _present_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (std::uint32_t i = 0; i < present_mode_count; ++i) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            _present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    std::uint32_t image_count = surface_capabilities.minImageCount + 1;
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
    swapchain_info.imageFormat = _color_format;
    swapchain_info.imageColorSpace = _color_space;
    swapchain_info.imageExtent = _swap_extent;
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
    if (vkCreateSwapchainKHR(_device, &swapchain_info, nullptr, &_swap_chain) != VK_SUCCESS) {
        throw make_exception("Failed to create swapchain");
    }

    if (vkGetSwapchainImagesKHR(_device, _swap_chain, &image_count, nullptr) != VK_SUCCESS) {
        throw make_exception("Failed to query swapchain image count");
    }

    _images.resize(image_count);
    if (vkGetSwapchainImagesKHR(_device, _swap_chain, &image_count, _images.data()) != VK_SUCCESS) {
        throw make_exception("Failed to enumerate swapchain images");
    }

    _image_views.resize(_images.size());

    for (std::size_t i{ 0 }; i < _images.size(); ++i) {
        VkImageViewCreateInfo image_view_info;
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.pNext = nullptr;
        image_view_info.flags = 0;
        image_view_info.image = _images[i];
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = _color_format;
        image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device, &image_view_info, nullptr, &_image_views[i]) != VK_SUCCESS) {
            throw make_exception("Failed to create image view");
        }
    }

    VkAttachmentDescription color_attachment{};
    color_attachment.format = _color_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    if (vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass) != VK_SUCCESS) {
        throw make_exception("Failed to create render pass!");
    }

    struct vertex {
        vec3f position;
        vec2f texcoord;
        vec3f normal;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc{};
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attribute_desc[3];
    vertex_input_attribute_desc[0].binding = 0;
    vertex_input_attribute_desc[0].location = 0;
    vertex_input_attribute_desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_desc[0].offset = offsetof(vertex, position);

    vertex_input_attribute_desc[1].binding = 0;
    vertex_input_attribute_desc[1].location = 1;
    vertex_input_attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_desc[1].offset = offsetof(vertex, texcoord);
    
    vertex_input_attribute_desc[2].binding = 0;
    vertex_input_attribute_desc[2].location = 2;
    vertex_input_attribute_desc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_desc[2].offset = offsetof(vertex, normal);

	VkPipelineVertexInputStateCreateInfo vertex_input_info{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.pNext = nullptr;
	vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = sizeof(vertex_input_attribute_desc) / sizeof(*vertex_input_attribute_desc);
	vertex_input_info.pVertexAttributeDescriptions = vertex_input_attribute_desc;

	VkPipelineInputAssemblyStateCreateInfo input_assembly;
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.pNext = nullptr;
	input_assembly.flags = 0;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swap_extent.width);
    viewport.height = static_cast<float>(_swap_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = _swap_extent;

    VkPipelineViewportStateCreateInfo viewport_state;
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = nullptr;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext = nullptr;
    rasterizer.flags = 0;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.pNext = nullptr;
    multisampling.flags = 0;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending;
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.pNext = nullptr;
    color_blending.flags = 0;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_pipeline_layout) != VK_SUCCESS) {
        throw make_exception("Failed to create pipeline layout!");
    }

    auto vertex_shader_module = create_shader_module(_device, builtin_shaders::get(builtin_shader::simple_vert));
	auto fragment_shader_module = create_shader_module(_device, builtin_shaders::get(builtin_shader::simple_frag));

	VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
	vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_shader_stage_info.pNext = nullptr;
	vertex_shader_stage_info.flags = 0;
	vertex_shader_stage_info.module = vertex_shader_module;
	vertex_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
	fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_shader_stage_info.pNext = nullptr;
	fragment_shader_stage_info.flags = 0;
	fragment_shader_stage_info.module = fragment_shader_module;
	fragment_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_info, fragment_shader_stage_info };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = nullptr;
    pipeline_info.flags = 0;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.layout = _pipeline_layout;
    pipeline_info.renderPass = _render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_pipeline) != VK_SUCCESS) {
        throw make_exception("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(_device, vertex_shader_module, nullptr);
    vkDestroyShaderModule(_device, fragment_shader_module, nullptr);

    _framebuffers.resize(_image_views.size());
    for (std::size_t i{ 0 }; i < _image_views.size(); ++i) {
        VkImageView attachments[] = { _image_views[i] };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = _swap_extent.width;
        framebuffer_info.height = _swap_extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
            throw make_exception("Failed to create framebuffer!");
        }
    }

    vertex vertices[3] = {
        { { 0.0f, -0.5f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { 0.5f, 0.5f, 0.0f }, { -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -0.5f, 0.5f, 0.0f }, { -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
    };

    buffer_desc buffer_desc;
    buffer_desc.type = buffer_type::vertex;
    buffer_desc.is_mutable = false;
    buffer_desc.size = sizeof(vertices);
    buffer_desc.stride = sizeof(vertex);
    buffer_desc.data = vertices;
    _vertex_buffer2 = std::make_shared<buffer_vulkan>(_physical_device, _device, buffer_desc);

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = graphics_family;

    if (vkCreateCommandPool(_device, &pool_info, nullptr, &_command_pool) != VK_SUCCESS) {
        throw make_exception("Failed to create command pool!");
    }

    _command_buffers.resize(_framebuffers.size());

    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.commandPool = _command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = static_cast<std::uint32_t>(_command_buffers.size());

    if (vkAllocateCommandBuffers(_device, &command_buffer_alloc_info, _command_buffers.data()) != VK_SUCCESS) {
        throw make_exception("Failed to allocate command buffers!");
    }

    for (std::size_t i{ 0 }; i < _command_buffers.size(); ++i) {
        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(_command_buffers[i], &command_buffer_begin_info) != VK_SUCCESS) {
            throw make_exception("Failed to begin recording command buffer!");
        }

        VkClearValue clear_color{ 0.0f, 0.0f, 0.0f, 1.0f };

        VkRenderPassBeginInfo render_pass_begin_info{};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = _render_pass;
        render_pass_begin_info.framebuffer = _framebuffers[i];
        render_pass_begin_info.renderArea.offset = { 0, 0 };
        render_pass_begin_info.renderArea.extent = _swap_extent;
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        VkBuffer vertex_buffers[] = { _vertex_buffer2->buffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(_command_buffers[i], 0, 1, vertex_buffers, offsets);

        vkCmdDraw(_command_buffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(_command_buffers[i]);

        if (vkEndCommandBuffer(_command_buffers[i]) != VK_SUCCESS) {
            throw make_exception("Failed to record command buffer!");
        }
    }

    _image_available_semaphores.resize(max_frames_in_flight);
    _render_finished_semaphores.resize(max_frames_in_flight);
    _in_flight_fences.resize(max_frames_in_flight);
    _images_in_flight.resize(_images.size(), VK_NULL_HANDLE);


    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (std::size_t i{ 0 }; i < max_frames_in_flight; ++i) {
        if (vkCreateSemaphore(_device, &semaphore_info, nullptr, &_image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_device, &semaphore_info, nullptr, &_render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(_device, &fence_info, nullptr, &_in_flight_fences[i]) != VK_SUCCESS) {
            throw make_exception("Failed to create synchronization objects for a frame!");
        }
    }
}

graphics_device_vulkan::~graphics_device_vulkan() {
    vkDestroyInstance(_instance, nullptr);
}

std::shared_ptr<texture> graphics_device_vulkan::make_texture(const texture_desc& desc) {
    return std::make_shared<texture_vulkan>(desc);
}

std::shared_ptr<texture_cube> graphics_device_vulkan::make_texture(const texture_cube_desc& texture_desc) {
    return std::make_shared<texture_cube_vulkan>(texture_desc);
}

std::shared_ptr<buffer> graphics_device_vulkan::make_buffer(const buffer_desc& buffer_desc) {
    return std::make_shared<buffer_vulkan>(_physical_device, _device, buffer_desc);
}

std::shared_ptr<shader> graphics_device_vulkan::make_shader(const shader_desc& shader_desc) {
    return std::make_shared<shader_vulkan>(_device, shader_desc);
}

std::shared_ptr<mesh> graphics_device_vulkan::make_mesh(const mesh_desc& mesh_desc) {
    return std::make_shared<mesh>(mesh_desc);
}

void graphics_device_vulkan::clear(const color& color) {

}

void graphics_device_vulkan::present() {
    vkWaitForFences(_device, 1, &_in_flight_fences[_current_frame], VK_TRUE, UINT64_MAX);

    std::uint32_t image_index;
    vkAcquireNextImageKHR(_device, _swap_chain, UINT64_MAX, _image_available_semaphores[_current_frame], VK_NULL_HANDLE, &image_index);

    if (_images_in_flight[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(_device, 1, &_images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }

    _images_in_flight[image_index] = _in_flight_fences[_current_frame];

    VkSemaphore wait_semaphores[] = { _image_available_semaphores[_current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signal_semaphores[] = { _render_finished_semaphores[_current_frame] };

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &_command_buffers[image_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(_device, 1, &_in_flight_fences[_current_frame]);

    if (vkQueueSubmit(_graphics_queue, 1, &submit_info, _in_flight_fences[_current_frame]) != VK_SUCCESS) {
        throw make_exception("Failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swap_chain;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(_present_queue, &present_info);

    _current_frame = (_current_frame + 1) % max_frames_in_flight;
}

void graphics_device_vulkan::set_blend_state(const blend_state& blend_state) {

}

void graphics_device_vulkan::set_depth_test(bool depth_test) {

}

void graphics_device_vulkan::set_backbuffer_size(const vec2i& size) {

}

vec2i graphics_device_vulkan::backbuffer_size() const {
    return { 1270, 720 };
}

void graphics_device_vulkan::set_clip_rect(const vec4i& clip_rect) {

}

void graphics_device_vulkan::set_render_target(const std::shared_ptr<texture>& render_target) {

}

void graphics_device_vulkan::set_render_target(const std::shared_ptr<texture_cube>& render_target, texture_cube_face face, int level) {

}

void graphics_device_vulkan::bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) {

}

void graphics_device_vulkan::bind_texture(const std::shared_ptr<texture>& texture, std::size_t binding_index) {

}

void graphics_device_vulkan::bind_texture(const std::shared_ptr<texture_cube>& texture, std::size_t binding_index) {

}

void graphics_device_vulkan::draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<shader>& shader) {

}