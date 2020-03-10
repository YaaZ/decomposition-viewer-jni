#pragma once


#include "rendering-context.h"


class Swapchain {
    vk::UniqueSwapchainKHR handle;
    Swapchain(vk::UniqueSwapchainKHR&& handle, vk::Extent2D dimension, std::vector<vk::Image>&& images, std::vector<vk::UniqueImageView>&& imageViews) :
    handle(std::move(handle)), extent(dimension), images(std::move(images)), imageViews(std::move(imageViews)) {}
public:
    Swapchain() = default;

    vk::Extent2D extent;
    std::vector<vk::Image> images;
    std::vector<vk::UniqueImageView> imageViews;


    inline const vk::SwapchainKHR& operator*() const { // NOLINT(google-runtime-operator)
        return *handle;
    }

    static Swapchain create(RenderingContext& renderingContext, const Swapchain& oldSwapchain) {
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = renderingContext.physicalDeviceProperties.getSurfaceCapabilities();
        vk::Extent2D extent = surfaceCapabilities.currentExtent;
        if(extent.width == -1 || extent.height == -1) extent = surfaceCapabilities.maxImageExtent;
        vk::UniqueSwapchainKHR newSurface = renderingContext.device->createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR{
                /*flags*/                 {},
                /*surface*/               renderingContext.physicalDeviceProperties.surface,
                /*minImageCount*/         renderingContext.graphicSettings.minImageCount,
                /*imageFormat*/           renderingContext.graphicSettings.imageFormat,
                /*imageColorSpace*/       renderingContext.graphicSettings.imageColorSpace,
                /*imageExtent*/           extent,
                /*imageArrayLayers*/      1,
                /*imageUsage*/            renderingContext.graphicSettings.imageUsage,
                /*imageSharingMode*/      vk::SharingMode::eExclusive,
                /*queueFamilyIndexCount*/ 0,
                /*pQueueFamilyIndices*/   nullptr,
                /*preTransform*/          *renderingContext.graphicSettings.preTransform,
                /*compositeAlpha*/        *renderingContext.graphicSettings.compositeAlpha,
                /*presentMode*/           *renderingContext.graphicSettings.presentMode,
                /*clipped*/               true,
                /*oldSwapchain*/          !oldSwapchain.handle ? vk::SwapchainKHR() : *(oldSwapchain.handle)
        });
        std::vector<vk::Image> images = renderingContext.device->getSwapchainImagesKHR(*newSurface);

        vk::ImageViewCreateInfo imageViewCreateInfo{
                /*flags*/            {},
                /*image*/            {},
                /*viewType*/         vk::ImageViewType::e2D,
                /*format*/           renderingContext.graphicSettings.imageFormat,
                /*components*/       {},
                /*subresourceRange*/ vk::ImageSubresourceRange{
                        /*aspectMask*/     vk::ImageAspectFlagBits::eColor,
                        /*baseMipLevel*/   0,
                        /*levelCount*/     1,
                        /*baseArrayLayer*/ 0,
                        /*layerCount*/     1
                }
        };
        std::vector<vk::UniqueImageView> imageViews(images.size());
        for (size_t i = 0; i < images.size(); i++) {
            imageViewCreateInfo.image = images[i];
            imageViews[i] = renderingContext.device->createImageViewUnique(imageViewCreateInfo);
        }

        return Swapchain(std::move(newSurface), surfaceCapabilities.currentExtent, std::move(images), std::move(imageViews));
    }


};