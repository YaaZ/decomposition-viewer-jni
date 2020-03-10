#pragma once


#include "physical-device-properties.h"



namespace vma {


    class Allocator {
        VmaAllocator handle {VK_NULL_HANDLE};
    public:
        inline Allocator() = default;
        inline Allocator(const Allocator&) = delete;
        inline Allocator& operator=(const Allocator&) = delete;
        inline Allocator(Allocator&& a) noexcept {
            handle = a.handle;
            a.handle = VK_NULL_HANDLE;
        }
        inline Allocator& operator=(Allocator&& a) noexcept {
            if(handle != VK_NULL_HANDLE) vmaDestroyAllocator(handle);
            handle = a.handle;
            a.handle = VK_NULL_HANDLE;
            return *this;
        }
        inline operator bool() const { return handle != VK_NULL_HANDLE; } // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        inline VmaAllocator& operator*() noexcept { return handle; }
        inline const VmaAllocator& operator*() const noexcept { return handle; }

        explicit Allocator(const VmaAllocatorCreateInfo& createInfo) {
            vk::createResultValue(vk::Result{vmaCreateAllocator(&createInfo, &handle)}, "Failed to create VMA allocator");
        }
        ~Allocator() {
            if(handle != VK_NULL_HANDLE) vmaDestroyAllocator(handle);
        }
    };





    class StreamBuffer {

        VmaAllocator vma {VK_NULL_HANDLE};
        VmaAllocation allocation {VK_NULL_HANDLE};
        vk::Buffer handle;

    public:
        VmaAllocationInfo allocationInfo {};

        inline StreamBuffer() = default;
        inline StreamBuffer(const StreamBuffer&) = delete;
        inline StreamBuffer& operator=(const StreamBuffer&) = delete;
        inline StreamBuffer(StreamBuffer&& a) noexcept {
            vma = a.vma;
            allocation = a.allocation;
            handle = a.handle;
            allocationInfo = a.allocationInfo;
            a.handle = vk::Buffer();
        }
        inline StreamBuffer& operator=(StreamBuffer&& a) noexcept {
            if(handle) vmaDestroyBuffer(vma, handle, allocation);
            vma = a.vma;
            allocation = a.allocation;
            handle = a.handle;
            allocationInfo = a.allocationInfo;
            a.handle = vk::Buffer();
            return *this;
        }
        inline operator bool() const { return handle; } // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        inline vk::Buffer& operator*() noexcept { return handle; }
        inline const vk::Buffer& operator*() const noexcept { return handle; }

        StreamBuffer(const Allocator& allocator, const vk::BufferCreateInfo& bufferCreateInfo) : vma(*allocator){
            VmaAllocationCreateInfo allocationCreateInfo{
                    /*flags*/          VMA_ALLOCATION_CREATE_MAPPED_BIT,
                    /*usage*/          VMA_MEMORY_USAGE_CPU_TO_GPU,
                    /*requiredFlags*/  0,
                    /*preferredFlags*/ 0,
                    /*memoryTypeBits*/ 0,
                    /*pool*/           VK_NULL_HANDLE,
                    /*pUserData*/      nullptr
            };
            vk::createResultValue((vk::Result) vmaCreateBuffer(vma, (VkBufferCreateInfo*) &bufferCreateInfo,
                    &allocationCreateInfo, (VkBuffer*) &handle, &allocation, &allocationInfo), "Buffer allocation error");
        }
        ~StreamBuffer() {
            if(handle) vmaDestroyBuffer(vma, handle, allocation);
        }

        void flush(VkDeviceSize offset, VkDeviceSize size) {
            vmaFlushAllocation(vma, allocation, offset, size);
        }

    };




    class UnmappedImage {

        vk::Device device;
        VmaAllocator vma {VK_NULL_HANDLE};
        VmaAllocation allocation {VK_NULL_HANDLE};
        vk::Image handle;

    public:
        VmaAllocationInfo allocationInfo {};
        vk::ImageView view;

        inline UnmappedImage() = default;
        inline UnmappedImage(const UnmappedImage&) = delete;
        inline UnmappedImage& operator=(const UnmappedImage&) = delete;
        inline UnmappedImage(UnmappedImage&& a) noexcept {
            device = a.device;
            vma = a.vma;
            allocation = a.allocation;
            handle = a.handle;
            allocationInfo = a.allocationInfo;
            view = a.view;
            a.handle = vk::Image();
        }
        inline UnmappedImage& operator=(UnmappedImage&& a) noexcept {
            if(handle) {
                device.destroyImageView(view);
                vmaDestroyImage(vma, handle, allocation);
            }
            device = a.device;
            vma = a.vma;
            allocation = a.allocation;
            handle = a.handle;
            allocationInfo = a.allocationInfo;
            view = a.view;
            a.handle = vk::Image();
            return *this;
        }
        inline operator bool() const { return handle; } // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        inline vk::Image& operator*() noexcept { return handle; }
        inline const vk::Image& operator*() const noexcept { return handle; }

        UnmappedImage(vk::Device device, const Allocator& allocator, const vk::ImageCreateInfo& imageCreateInfo,
                const vk::ImageViewCreateInfo& imageViewCreateInfo) : vma(*allocator), device(device) {
            VmaAllocationCreateInfo allocationCreateInfo{
                    /*flags*/          0,
                    /*usage*/          VMA_MEMORY_USAGE_GPU_ONLY,
                    /*requiredFlags*/  0,
                    /*preferredFlags*/ 0,
                    /*memoryTypeBits*/ 0,
                    /*pool*/           VK_NULL_HANDLE,
                    /*pUserData*/      nullptr
            };
            vk::createResultValue((vk::Result) vmaCreateImage(vma, (VkImageCreateInfo*) &imageCreateInfo,
                    &allocationCreateInfo, (VkImage*) &handle, &allocation, &allocationInfo), "Image allocation error");

            vk::ImageViewCreateInfo newImageCreateInfo {imageViewCreateInfo};
            newImageCreateInfo.image = handle;
            view = device.createImageView(newImageCreateInfo);
        }
        ~UnmappedImage() {
            if(handle) {
                device.destroyImageView(view);
                vmaDestroyImage(vma, handle, allocation);
            }
        }

    };


}





static vma::Allocator createVmaAllocator(vk::PhysicalDevice physicalDevice, vk::Device device, bool dedicatedAllocation) {
    VmaAllocatorCreateFlags dedicatedAllocationBit =
            dedicatedAllocation ? VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT : 0;
    VmaVulkanFunctions functions {
            /*vkGetPhysicalDeviceProperties*/       VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
            /*vkGetPhysicalDeviceMemoryProperties*/ VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
            /*vkAllocateMemory*/                    VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
            /*vkFreeMemory*/                        VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
            /*vkMapMemory*/                         VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
            /*vkUnmapMemory*/                       VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
            /*vkFlushMappedMemoryRanges*/           VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
            /*vkInvalidateMappedMemoryRanges*/      VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
            /*vkBindBufferMemory*/                  VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
            /*vkBindImageMemory*/                   VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
            /*vkGetBufferMemoryRequirements*/       VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
            /*vkGetImageMemoryRequirements*/        VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
            /*vkCreateBuffer*/                      VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
            /*vkDestroyBuffer*/                     VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
            /*vkCreateImage*/                       VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
            /*vkDestroyImage*/                      VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
            /*vkCmdCopyBuffer*/                     VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
            /*vkGetBufferMemoryRequirements2KHR*/   VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2KHR,
            /*vkGetImageMemoryRequirements2KHR*/    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2KHR,
            /*vkBindBufferMemory2KHR*/              VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2KHR,
            /*vkBindImageMemory2KHR*/               VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2KHR
    };
    return vma::Allocator({
        /*flags*/                       dedicatedAllocationBit,
        /*physicalDevice*/              physicalDevice,
        /*device*/                      device,
        /*preferredLargeHeapBlockSize*/ 1024L * 1024L * 16L,
        /*pAllocationCallbacks*/        nullptr,
        /*pDeviceMemoryCallbacks*/      nullptr,
        /*frameInUseCount*/             0,
        /*pHeapSizeLimit*/              nullptr,
        /*pVulkanFunctions*/            &functions,
        /*pRecordSettings*/             nullptr
    });
}