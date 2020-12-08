#define NOMINMAX
#include <vector>
#include <cstring>
#include <iostream>
#include <csignal>
#include <jawt_md.h>

#define VMA_IMPLEMENTATION
#include "include.h"




extern const uint32_t APP_VK_VERSION = VK_MAKE_VERSION(1, 1, 0);

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static vk::DynamicLoader* dynamicLoader;
vk::Instance vkInstance;





static void initDefaultDynamicDispatcher() {
    dynamicLoader = new vk::DynamicLoader();
    auto vkGetInstanceProcAddr = dynamicLoader->getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    if(!dynamicLoader->success()) throw std::runtime_error("Cannot load vulkan library");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
}



static void ensureVulkanVersionSupported() {
    uint32_t version = vk::enumerateInstanceVersion();
    if(version < APP_VK_VERSION) {
        throw std::runtime_error(
                "Unsupported Vulkan version: " + std::to_string(VK_VERSION_MAJOR(version)) + "." +
                std::to_string(VK_VERSION_MINOR(version)) + "." + std::to_string(VK_VERSION_PATCH(version)) // NOLINT(hicpp-signed-bitwise)
        );
    }
}





static vk::Instance createVkInstance() {
    // Check if necessary layers are supported
    std::vector<vk::LayerProperties> supportedLayers = vk::enumerateInstanceLayerProperties();
    std::string validationLayerName = "VK_LAYER_KHRONOS_validation";
    uint32_t validationLayerFound = 0;
#if !defined(NDEBUG)
    for(vk::LayerProperties& supportedLayer : supportedLayers) {
        if(std::strcmp(validationLayerName.c_str(), supportedLayer.layerName) == 0) {
            validationLayerFound = 1;
            break;
        }
    }
    if(validationLayerFound == 0) std::cerr << "Instance validation layer not found" << std::endl;
#endif

    // Check if necessary extensions are supported
    std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();
    std::string debugUtilsExtensionName = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    bool debugUtilsExtensionFound = false;
    std::string surfaceExtensionName = VK_KHR_SURFACE_EXTENSION_NAME;
    std::string platformSurfaceExtensionName = PLATFORM_SPECIFIC_SURFACE_EXTENSION_NAME;
    std::string physicalDeviceProperties2ExtensionName = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
    std::vector<const char*> extensionNamePointers;
    for(vk::ExtensionProperties& extensionProperties : supportedExtensions) {
#if !defined(NDEBUG)
        if(std::strcmp(debugUtilsExtensionName.c_str(), extensionProperties.extensionName) == 0) {
            debugUtilsExtensionFound = true;
        }
#endif
        if(std::strcmp(surfaceExtensionName.c_str(), extensionProperties.extensionName) == 0) {
            extensionNamePointers.push_back(surfaceExtensionName.c_str());
        }
        if(std::strcmp(platformSurfaceExtensionName.c_str(), extensionProperties.extensionName) == 0) {
            extensionNamePointers.push_back(platformSurfaceExtensionName.c_str());
        }
        if(std::strcmp(physicalDeviceProperties2ExtensionName.c_str(), extensionProperties.extensionName) == 0) {
            extensionNamePointers.push_back(physicalDeviceProperties2ExtensionName.c_str());
        }
    }
    if(extensionNamePointers.size() < 3) throw std::runtime_error("Required extensions were not found");
#if !defined(NDEBUG)
    if(debugUtilsExtensionFound) extensionNamePointers.push_back(debugUtilsExtensionName.c_str());
    else std::cerr << "Instance debug utils extension not found" << std::endl;
#endif

    vk::ApplicationInfo applicationInfo {
            /*pApplicationName*/   "Decomposition viewer",
            /*applicationVersion*/ 0,
            /*pEngineName*/        "",
            /*engineVersion*/      0,
            /*apiVersion*/         APP_VK_VERSION
    };

    const char* validationLayerNamePointer = validationLayerName.c_str();
    vk::InstanceCreateInfo instanceCreateInfo {
            /*flags*/                   {},
            /*pApplicationInfo*/        &applicationInfo,
            /*enabledLayerCount*/       validationLayerFound,
            /*ppEnabledLayerNames*/     &validationLayerNamePointer,
            /*enabledExtensionCount*/   (uint32_t) extensionNamePointers.size(),
            /*ppEnabledExtensionNames*/ extensionNamePointers.data()
    };

#if !defined(NDEBUG)
    vk::ValidationFeatureEnableEXT enabledValidationFeature = vk::ValidationFeatureEnableEXT::eBestPractices;
    vk::ValidationFeaturesEXT validationFeatures {
            /*enabledValidationFeatureCount*/  1,
            /*pEnabledValidationFeatures*/     &enabledValidationFeature,
            /*disabledValidationFeatureCount*/ 0,
            /*pDisabledValidationFeatures*/    nullptr
    };
    if(validationLayerFound == 1 && debugUtilsExtensionFound) instanceCreateInfo.setPNext(&validationFeatures);
#endif

    vk::Instance instance = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    return instance;
}





#if !defined(NDEBUG)
static vk::DebugUtilsMessengerEXT debugMessenger;
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void*
) {
    if(data == nullptr) return 0;
    std::string severity, type;
    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0) severity += ", verbose";
    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0) severity += ", info";
    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) severity += ", warning";
    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) severity += ", error";
    if((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0) type += ", general";
    if((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0) type += ", validation";
    if((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0) type += ", performance";
    std::string messageId = std::string(data->pMessageIdName);
    // Here we can filter messages like: if(messageId == "shit") return 0;
    std::string message = "[" + severity.substr(2) + "] [" + type.substr(2) + "] [" + messageId + "] " + std::string(data->pMessage);
    if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
        std::cerr << message << std::endl;
        raise(SIGABRT);
    }
    else if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) std::cerr << message << std::endl;
    else std::cout << message << std::endl;
    return 0;
}

static vk::DebugUtilsMessengerEXT createDebugMessenger(vk::Instance instance) {
    return instance.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT{
            /*flags*/           {},
            /*messageSeverity*/ vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            /*messageType*/     vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            /*pfnUserCallback*/ debugCallback,
            /*pUserData*/       nullptr
    });
}
#endif





void initVulkan() {
    initDefaultDynamicDispatcher();
    ensureVulkanVersionSupported();
    vkInstance = createVkInstance();
#if !defined(NDEBUG)
    debugMessenger = createDebugMessenger(vkInstance);
#endif
}



void destroyVulkan() {
#if !defined(NDEBUG)
    vkInstance.destroyDebugUtilsMessengerEXT(debugMessenger);
#endif
    vkInstance.destroy();
    delete dynamicLoader;
}