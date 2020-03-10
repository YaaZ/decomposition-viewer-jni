#pragma once


#include <optional>
#include <cctype>

#include "physical-device-properties.h"



class GraphicRequirementsNotSatisfiedException : public std::exception {

    const std::string message;

public:
    explicit GraphicRequirementsNotSatisfiedException(std::string&& message) : message(std::move(message)) {}

    [[nodiscard]] const char* what() const noexcept final {
        return message.c_str();
    }

};



class GraphicSettings {
public:
    /**
     * Internal settings
     */
public:
    vk::SampleCountFlagBits sampleCount {};
    vk::Format imageFormat {vk::Format::eUndefined};
    vk::ColorSpaceKHR imageColorSpace {};
    vk::ImageUsageFlags imageUsage {};
    std::optional<vk::SurfaceTransformFlagBitsKHR> preTransform {};
    std::optional<vk::CompositeAlphaFlagBitsKHR> compositeAlpha {};

    /**
     * Settings related to swapchain creation
     */
public:
    uint32_t minImageCount {0};
    std::optional<vk::PresentModeKHR> presentMode {};





    void validate(const PhysicalDeviceProperties& properties) {
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = properties.getSurfaceCapabilities();



        // Properties setup

        { // Multisampling
            vk::SampleCountFlags samples = properties.physicalDeviceProperties.limits.framebufferColorSampleCounts;
            if((samples & vk::SampleCountFlagBits::e64) == vk::SampleCountFlagBits::e64) sampleCount = vk::SampleCountFlagBits::e64;
            else if((samples & vk::SampleCountFlagBits::e32) == vk::SampleCountFlagBits::e32) sampleCount = vk::SampleCountFlagBits::e32;
            else if((samples & vk::SampleCountFlagBits::e16) == vk::SampleCountFlagBits::e16) sampleCount = vk::SampleCountFlagBits::e16;
            else if((samples & vk::SampleCountFlagBits::e8) == vk::SampleCountFlagBits::e8) sampleCount = vk::SampleCountFlagBits::e8;
            else if((samples & vk::SampleCountFlagBits::e4) == vk::SampleCountFlagBits::e4) sampleCount = vk::SampleCountFlagBits::e4;
            else if((samples & vk::SampleCountFlagBits::e2) == vk::SampleCountFlagBits::e2) sampleCount = vk::SampleCountFlagBits::e2;
            else sampleCount = vk::SampleCountFlagBits::e1;
        }

        { // Format
            if(properties.surfaceFormats.empty()) throw GraphicRequirementsNotSatisfiedException("No applicable graphic format");
            vk::SurfaceFormatKHR surfaceFormat = properties.surfaceFormats[0];
            imageFormat = properties.surfaceFormats.size() == 1 && surfaceFormat.format == vk::Format::eUndefined ?
                          vk::Format::eB8G8R8A8Unorm : surfaceFormat.format;
            imageColorSpace = surfaceFormat.colorSpace;
        }

        { // Image usage
            imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            if((surfaceCapabilities.supportedUsageFlags & imageUsage) != imageUsage) throw GraphicRequirementsNotSatisfiedException("No applicable image usage flags");
        }

        { // Pre-transform
            if(!preTransform || (surfaceCapabilities.supportedTransforms & preTransform.value()) != preTransform) {
                preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ?
                               vk::SurfaceTransformFlagBitsKHR::eIdentity : surfaceCapabilities.currentTransform;
            }
        }

        { // Composite alpha
            if(!compositeAlpha || (surfaceCapabilities.supportedCompositeAlpha & compositeAlpha.value()) != compositeAlpha) {
                if(surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque) compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
                else throw GraphicRequirementsNotSatisfiedException("Not applicable composite alpha");
            }
        }

        { // Present mode
            // Turned out that immediate mode and 1-2 images are best against tearing when drawing through jawt
            vk::PresentModeKHR presentModes[] = {
                    vk::PresentModeKHR::eImmediate,
                    vk::PresentModeKHR::eFifo
            };
            for(vk::PresentModeKHR mode : presentModes) {
                for (auto surfacePresentMode : properties.surfacePresentModes) {
                    if(surfacePresentMode == mode) {
                        presentMode = mode;
                        goto presentModeSet;
                    }
                }
            }
            throw GraphicRequirementsNotSatisfiedException("No applicable present mode");
            presentModeSet:;
        }

        { // Min image count
            uint32_t imageCount = 2;
            if(minImageCount == 0 || minImageCount < surfaceCapabilities.minImageCount || minImageCount > surfaceCapabilities.maxImageCount) {
                minImageCount = surfaceCapabilities.maxImageCount < imageCount ? surfaceCapabilities.maxImageCount : imageCount;
            }
        }
    }
};