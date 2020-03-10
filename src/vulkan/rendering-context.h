#pragma once


#include <iostream>


#include "graphic-settings.h"
#include "vma.h"


extern const uint32_t APP_VK_VERSION;



struct RenderingContext {

    vk::UniqueDevice device;
    uint32_t queueFamily {};
    vk::Queue queue;
    PhysicalDeviceProperties physicalDeviceProperties;
    GraphicSettings graphicSettings;
    vma::Allocator vma;

};



static int findApplicableQueueFamily(const vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR surface) {
    std::vector<vk::QueueFamilyProperties> familyProperties = physicalDevice.getQueueFamilyProperties();
    for (int i = 0; i < familyProperties.size(); i++) {
        vk::QueueFamilyProperties& family = familyProperties[i];
        if(physicalDevice.getSurfaceSupportKHR(i, surface) &&
           (family.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics)
            return i;
    }
    return -1;
}



static RenderingContext createRenderingContext(vk::Instance vk, vk::SurfaceKHR surface) {
    for (const vk::PhysicalDevice& physicalDevice : vk.enumeratePhysicalDevices()) {

        RenderingContext renderingContext;

        renderingContext.physicalDeviceProperties = {physicalDevice, surface};
        std::cout << "Found device: " << renderingContext.physicalDeviceProperties.physicalDeviceProperties.deviceName << std::endl;


        // Ensure device supports all necessary layers
        std::vector<vk::LayerProperties> supportedLayers = physicalDevice.enumerateDeviceLayerProperties();
        std::string validationLayerName = "VK_LAYER_KHRONOS_validation";
        uint32_t validationLayerFound = 0;
#if !defined(NDEBUG)
        for(vk::LayerProperties& supportedLayer : supportedLayers) {
            if(std::strcmp(validationLayerName.c_str(), supportedLayer.layerName) == 0) {
                validationLayerFound = 1;
                break;
            }
        }
        if(validationLayerFound == 0) std::cerr << "Device validation layer not found" << std::endl;
#endif


        // Ensure device supports all necessary extensions
        std::vector<vk::ExtensionProperties> supportedExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        std::string swapchainExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        bool swapchainExtensionFound = false;
        std::string dedicatedAllocationExtensionName = VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME;
        std::string getMemoryRequirements2ExtensionName = VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
        std::vector<const char*> extensionNamePointers;
        for(vk::ExtensionProperties& extensionProperties : supportedExtensions) {
            if(std::strcmp(swapchainExtensionName.c_str(), extensionProperties.extensionName) == 0) {
                swapchainExtensionFound = true;
            }
            if(std::strcmp(dedicatedAllocationExtensionName.c_str(), extensionProperties.extensionName) == 0) {
                extensionNamePointers.push_back(dedicatedAllocationExtensionName.c_str());
            }
            if(std::strcmp(getMemoryRequirements2ExtensionName.c_str(), extensionProperties.extensionName) == 0) {
                extensionNamePointers.push_back(getMemoryRequirements2ExtensionName.c_str());
            }
        }
        bool dedicatedAllocationExtensionSupported = extensionNamePointers.size() == 2;
        if(!swapchainExtensionFound) {
            std::cerr << "Device swapchain extension not found" << std::endl;
            continue;
        }
        extensionNamePointers.push_back(swapchainExtensionName.c_str());


        // Check Vulkan version
        if(renderingContext.physicalDeviceProperties.physicalDeviceProperties.apiVersion < APP_VK_VERSION) {
            std::cerr << "Too old driver version" << std::endl;
            continue;
        }

        // Setup graphic settings
        try {
            renderingContext.graphicSettings.validate(renderingContext.physicalDeviceProperties);
        } catch(const GraphicRequirementsNotSatisfiedException& e) {
            std::cerr << e.what() << std::endl;
            continue;
        }

        // Find applicable queue
        int queueFamily = findApplicableQueueFamily(physicalDevice, surface);
        if(queueFamily == -1) {
            std::cerr << "No applicable queue family" << std::endl;
            continue;
        }



        // Create device
        float queuePriority = 1;
        vk::DeviceQueueCreateInfo queueCreateInfo {
                /*flags*/            {},
                /*queueFamilyIndex*/ (uint32_t) queueFamily,
                /*queueCount*/       1,
                /*pQueuePriorities*/ &queuePriority
        };

        vk::PhysicalDeviceFeatures physicalDeviceFeatures {};
        physicalDeviceFeatures.fillModeNonSolid = true;
        physicalDeviceFeatures.wideLines = true;
        physicalDeviceFeatures.geometryShader = true;

        const char* validationLayerNamePointer = validationLayerName.c_str();

        renderingContext.device = renderingContext.physicalDeviceProperties.physicalDevice.createDeviceUnique(vk::DeviceCreateInfo{
                /*flags*/                   {},
                /*queueCreateInfoCount*/    1,
                /*pQueueCreateInfos*/       &queueCreateInfo,
                /*enabledLayerCount*/       validationLayerFound,
                /*ppEnabledLayerNames*/     &validationLayerNamePointer,
                /*enabledExtensionCount*/   (uint32_t) extensionNamePointers.size(),
                /*ppEnabledExtensionNames*/ extensionNamePointers.data(),
                /*pEnabledFeatures*/        &physicalDeviceFeatures
        });
        renderingContext.queueFamily = queueFamily;
        renderingContext.queue = renderingContext.device->getQueue(queueFamily, 0);

        renderingContext.vma = createVmaAllocator(physicalDevice, *renderingContext.device, dedicatedAllocationExtensionSupported);

        return renderingContext;

    }
    throw std::runtime_error("No applicable device found");
}