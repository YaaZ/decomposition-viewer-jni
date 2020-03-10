#pragma once


class PhysicalDeviceProperties {


public:
    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceProperties physicalDeviceProperties;
    vk::PhysicalDeviceSubgroupProperties subgroupProperties;
    vk::PhysicalDeviceFeatures physicalDeviceFeatures;

    std::vector<vk::SurfaceFormatKHR> surfaceFormats;
    std::vector<vk::PresentModeKHR> surfacePresentModes;

    PhysicalDeviceProperties() = default;
    PhysicalDeviceProperties(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) :
            physicalDevice(physicalDevice), surface(surface),
            surfaceFormats(physicalDevice.getSurfaceFormatsKHR(surface)),
            surfacePresentModes(physicalDevice.getSurfacePresentModesKHR(surface)) {
        vk::PhysicalDeviceProperties2 physicalDeviceProperties2;
        physicalDeviceProperties2.pNext = &subgroupProperties;
        physicalDevice.getProperties2(&physicalDeviceProperties2);
        physicalDeviceProperties = physicalDeviceProperties2.properties;

        physicalDeviceFeatures = physicalDevice.getFeatures();
    }

    [[nodiscard]] vk::SurfaceCapabilitiesKHR getSurfaceCapabilities() const {
        return physicalDevice.getSurfaceCapabilitiesKHR(surface);
    }


};