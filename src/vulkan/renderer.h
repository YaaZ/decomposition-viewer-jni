#pragma once


#include "rendering-context.h"
#include "swapchain.h"
#include "shader-module.h"


template <typename Type>
static inline Type operator!(const vk::ResultValue<Type>& r) {
    vk::createResultValue(r.result, "Vulkan error");
    return r.value;
}



class VulkanRenderer : public RenderingContext {


    vk::UniqueSemaphore acquireImageSemaphore, renderingCompleteSemaphore;
    vk::UniqueFence renderingCompleteFence;
    vk::UniqueRenderPass renderPass;
    vk::UniqueDescriptorSetLayout descriptorSetLayout;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniqueShaderModule vertexShader, triangleFragmentShader, flatFragmentShader, triangleVertexGeometryShader;
    vk::UniquePipeline trianglePipeline, triangleEdgePipeline, polygonEdgePipeline, polygonVertexPipeline;
    vk::UniqueDescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;

    vma::StreamBuffer uniformBuffer;
    vma::StreamBuffer vertexBuffer;
    vma::StreamBuffer triangleIndexBuffer, triangleDrawIndirectBuffer;
    vma::StreamBuffer polygonVerticesBuffer, polygonDrawIndirectBuffer;

    Swapchain swapchain;

    struct SwapchainContext {

        vma::UnmappedImage image;

        std::vector<vk::UniqueFramebuffer> framebuffers;

        std::vector<vk::CommandBuffer> commandBuffers;

    } swapchainContext;

    vk::UniqueCommandPool commandPool;


public:
    VulkanRenderer() = default;
    VulkanRenderer(VulkanRenderer&&) noexcept = default;
    /** We implement custom move assignment operator, because default operator moves members in forward direction,
     * which can cause errors when destroying Vulkan objects, for example device being destroyed before some buffer.
     * Therefore, we first call destructor on current object and then construct new one in place by moving from src.
     */
    VulkanRenderer& operator=(VulkanRenderer&& src) noexcept {
        this->~VulkanRenderer();
        new (this) VulkanRenderer(std::move(src));
        return *this;
    }
    ~VulkanRenderer() {
        if(device) device->waitIdle();
    }
    explicit VulkanRenderer(vk::Instance vk, vk::SurfaceKHR surface) :
    RenderingContext(createRenderingContext(vk, surface)) {

        acquireImageSemaphore = device->createSemaphoreUnique({});
        renderingCompleteSemaphore = device->createSemaphoreUnique({});
        renderingCompleteFence = device->createFenceUnique({vk::FenceCreateFlagBits::eSignaled});

        commandPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
                /*flags*/            {},
                /*queueFamilyIndex*/ queueFamily
        });


        vk::AttachmentDescription renderPassAttachmentDescriptions[] {
                {
                        /*flags*/          {},
                        /*format*/         graphicSettings.imageFormat,
                        /*samples*/        graphicSettings.sampleCount,
                        /*loadOp*/         vk::AttachmentLoadOp::eClear,
                        /*storeOp*/        vk::AttachmentStoreOp::eDontCare,
                        /*stencilLoadOp*/  vk::AttachmentLoadOp::eDontCare,
                        /*stencilStoreOp*/ vk::AttachmentStoreOp::eDontCare,
                        /*initialLayout*/  vk::ImageLayout::eUndefined,
                        /*finalLayout*/    vk::ImageLayout::eColorAttachmentOptimal
                },
                {
                        /*flags*/          {},
                        /*format*/         graphicSettings.imageFormat,
                        /*samples*/        vk::SampleCountFlagBits::e1,
                        /*loadOp*/         vk::AttachmentLoadOp::eDontCare,
                        /*storeOp*/        vk::AttachmentStoreOp::eStore,
                        /*stencilLoadOp*/  vk::AttachmentLoadOp::eDontCare,
                        /*stencilStoreOp*/ vk::AttachmentStoreOp::eDontCare,
                        /*initialLayout*/  vk::ImageLayout::eUndefined,
                        /*finalLayout*/    vk::ImageLayout::ePresentSrcKHR
                }
        };
        vk::AttachmentReference renderPassAttachmentReferences[] {
                {
                        /*attachment*/ 0,
                        /*layout*/     vk::ImageLayout::eColorAttachmentOptimal
                },
                {
                        /*attachment*/ 1,
                        /*layout*/     vk::ImageLayout::eColorAttachmentOptimal
                }
        };
        vk::SubpassDescription renderPassSubpassDescription{
                /*flags*/                   {},
                /*pipelineBindPoint*/       vk::PipelineBindPoint::eGraphics,
                /*inputAttachmentCount*/    0,
                /*pInputAttachments*/       nullptr,
                /*colorAttachmentCount*/    1,
                /*pColorAttachments*/       renderPassAttachmentReferences,
                /*pResolveAttachments*/     renderPassAttachmentReferences + 1,
                /*pDepthStencilAttachment*/ nullptr,
                /*preserveAttachmentCount*/ 0,
                /*pPreserveAttachments*/    nullptr
        };
        renderPass = device->createRenderPassUnique(vk::RenderPassCreateInfo{
                /*flags*/           {},
                /*attachmentCount*/ 2,
                /*pAttachments*/    renderPassAttachmentDescriptions,
                /*subpassCount*/    1,
                /*pSubpasses*/      &renderPassSubpassDescription,
                /*dependencyCount*/ 0,
                /*pDependencies*/   nullptr
        });

        vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding {
                /*binding*/            0,
                /*descriptorType*/     vk::DescriptorType::eUniformBuffer,
                /*descriptorCount*/    1,
                /*stageFlags*/         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry,
                /*pImmutableSamplers*/ nullptr
        };
        descriptorSetLayout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo{
                /*flags*/        {},
                /*bindingCount*/ 1,
                /*pBindings*/    &descriptorSetLayoutBinding
        });

        pipelineLayout = device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{
                /*flags*/                  {},
                /*setLayoutCount*/         1,
                /*pSetLayouts*/            &*descriptorSetLayout,
                /*pushConstantRangeCount*/ 0,
                /*pPushConstantRanges*/    nullptr
        });

        vertexShader = loadShader(*device, resource::shader::main_vert);
        triangleFragmentShader = loadShader(*device, resource::shader::triangle_frag);
        flatFragmentShader = loadShader(*device, resource::shader::flat_frag);
        triangleVertexGeometryShader = loadShader(*device, resource::shader::triangle_vertex_geom);



        vk::PipelineShaderStageCreateInfo stageCreateInfos[] {
                vk::PipelineShaderStageCreateInfo{
                        /*flags*/               {},
                        /*stage*/               vk::ShaderStageFlagBits::eVertex,
                        /*module*/              *vertexShader,
                        /*pName*/               "main",
                        /*pSpecializationInfo*/ nullptr
                },
                vk::PipelineShaderStageCreateInfo{
                        /*flags*/               {},
                        /*stage*/               vk::ShaderStageFlagBits::eFragment,
                        /*module*/              *triangleFragmentShader,
                        /*pName*/               "main",
                        /*pSpecializationInfo*/ nullptr
                },
                {}
        };


        vk::VertexInputBindingDescription vertexInputBindingDescription{
                /*binding*/   0,
                /*stride*/    8,
                /*inputRate*/ vk::VertexInputRate::eVertex
        };
        vk::VertexInputAttributeDescription vertexInputAttributeDescription{
                /*location*/ 0,
                /*binding*/  0,
                /*format*/   vk::Format::eR32G32Sfloat,
                /*offset*/   0
        };
        vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
                /*flags*/                           {},
                /*vertexBindingDescriptionCount*/   1,
                /*pVertexBindingDescriptions*/      &vertexInputBindingDescription,
                /*vertexAttributeDescriptionCount*/ 1,
                /*pVertexAttributeDescriptions*/    &vertexInputAttributeDescription
        };


        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
                /*flags*/                  {},
                /*topology*/               vk::PrimitiveTopology::eTriangleList,
                /*primitiveRestartEnable*/ false
        };


        vk::Viewport viewportStateCreateInfoViewport {};
        vk::Rect2D viewportStateCreateInfoScissor {};
        vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{
                /*flags*/         {},
                /*viewportCount*/ 1,
                /*pViewports*/    &viewportStateCreateInfoViewport,
                /*scissorCount*/  1,
                /*pScissors*/     &viewportStateCreateInfoScissor
        };


        vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
                /*flags*/                   {},
                /*depthClampEnable*/        false,
                /*rasterizerDiscardEnable*/ false,
                /*polygonMode*/             vk::PolygonMode::eFill,
                /*cullMode*/                vk::CullModeFlagBits::eNone,
                /*frontFace*/               vk::FrontFace::eCounterClockwise,
                /*depthBiasEnable*/         false,
                /*depthBiasConstantFactor*/ 0,
                /*depthBiasClamp*/          0,
                /*depthBiasSlopeFactor*/    0,
                /*lineWidth*/               1
        };


        vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{
                /*flags*/                 {},
                /*rasterizationSamples*/  graphicSettings.sampleCount,
                /*sampleShadingEnable*/   false,
                /*minSampleShading*/      0,
                /*pSampleMask*/           nullptr,
                /*alphaToCoverageEnable*/ false,
                /*alphaToOneEnable*/      false
        };


        vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{
                /*flags*/                 {},
                /*depthTestEnable*/       false,
                /*depthWriteEnable*/      false,
                /*depthCompareOp*/        vk::CompareOp::eNever,
                /*depthBoundsTestEnable*/ false,
                /*stencilTestEnable*/     false,
                /*front*/                 {},
                /*back*/                  {},
                /*minDepthBounds*/        0,
                /*maxDepthBounds*/        1
        };


        vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{
                /*blendEnable*/         false,
                /*srcColorBlendFactor*/ vk::BlendFactor::eZero,
                /*dstColorBlendFactor*/ vk::BlendFactor::eZero,
                /*colorBlendOp*/        vk::BlendOp::eAdd,
                /*srcAlphaBlendFactor*/ vk::BlendFactor::eZero,
                /*dstAlphaBlendFactor*/ vk::BlendFactor::eZero,
                /*alphaBlendOp*/        vk::BlendOp::eAdd,
                /*colorWriteMask*/      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
        };
        vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
                /*flags*/           {},
                /*logicOpEnable*/   false,
                /*logicOp*/         vk::LogicOp::eClear,
                /*attachmentCount*/ 1,
                /*pAttachments*/    &colorBlendAttachmentState,
                /*blendConstants*/  {}
        };


        vk::DynamicState dynamicStates[] {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{
                /*flags*/             {},
                /*dynamicStateCount*/ 2,
                /*pDynamicStates*/    dynamicStates
        };


        vk::GraphicsPipelineCreateInfo pipelineCreateInfo {
                /*flags*/               {},
                /*stageCount*/          2,
                /*pStages*/             stageCreateInfos,
                /*pVertexInputState*/   &vertexInputStateCreateInfo,
                /*pInputAssemblyState*/ &inputAssemblyStateCreateInfo,
                /*pTessellationState*/  nullptr,
                /*pViewportState*/      &viewportStateCreateInfo,
                /*pRasterizationState*/ &rasterizationStateCreateInfo,
                /*pMultisampleState*/   &pipelineMultisampleStateCreateInfo,
                /*pDepthStencilState*/  &depthStencilStateCreateInfo,
                /*pColorBlendState*/    &colorBlendStateCreateInfo,
                /*pDynamicState*/       &dynamicStateCreateInfo,
                /*layout*/              *pipelineLayout,
                /*renderPass*/          *renderPass,
                /*subpass*/             0,
                /*basePipelineHandle*/  {},
                /*basePipelineIndex*/   -1
        };


        trianglePipeline = device->createGraphicsPipelineUnique({}, pipelineCreateInfo);


        vk::SpecializationMapEntry specializationMapEntries[] {{
                /*constantID*/ 0,
                /*offset*/     0,
                /*size*/       4
        }, {
                /*constantID*/ 1,
                /*offset*/     4,
                /*size*/       4
        }, {
                /*constantID*/ 2,
                /*offset*/     8,
                /*size*/       4
        }};
        glm::vec3 flatColor {1.0, 1.0, 1.0};
        vk::SpecializationInfo specializationInfo{
                /*mapEntryCount*/ 3,
                /*pMapEntries*/   specializationMapEntries,
                /*dataSize*/      12,
                /*pData*/         &flatColor
        };
        stageCreateInfos[1] = vk::PipelineShaderStageCreateInfo{
                /*flags*/               {},
                /*stage*/               vk::ShaderStageFlagBits::eFragment,
                /*module*/              *flatFragmentShader,
                /*pName*/               "main",
                /*pSpecializationInfo*/ &specializationInfo
        };
        rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eLine;
        triangleEdgePipeline = device->createGraphicsPipelineUnique({}, pipelineCreateInfo);


        flatColor = {0.1, 0.1, 0.1};
        stageCreateInfos[1] = vk::PipelineShaderStageCreateInfo{
                /*flags*/               {},
                /*stage*/               vk::ShaderStageFlagBits::eFragment,
                /*module*/              *flatFragmentShader,
                /*pName*/               "main",
                /*pSpecializationInfo*/ &specializationInfo
        };
        rasterizationStateCreateInfo.lineWidth = 3;
        inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eLineList;
        polygonEdgePipeline = device->createGraphicsPipelineUnique({}, pipelineCreateInfo);


        rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
        rasterizationStateCreateInfo.lineWidth = 1;
        stageCreateInfos[1] = vk::PipelineShaderStageCreateInfo{
                /*flags*/               {},
                /*stage*/               vk::ShaderStageFlagBits::eGeometry,
                /*module*/              *triangleVertexGeometryShader,
                /*pName*/               "main",
                /*pSpecializationInfo*/ nullptr
        };
        stageCreateInfos[2] = vk::PipelineShaderStageCreateInfo{
                /*flags*/               {},
                /*stage*/               vk::ShaderStageFlagBits::eFragment,
                /*module*/              *flatFragmentShader,
                /*pName*/               "main",
                /*pSpecializationInfo*/ &specializationInfo
        };
        pipelineCreateInfo.stageCount = 3;
        polygonVertexPipeline = device->createGraphicsPipelineUnique({}, pipelineCreateInfo);





        vk::DescriptorPoolSize descriptorPoolSize{
                /*type*/            vk::DescriptorType::eUniformBuffer,
                /*descriptorCount*/ 1
        };
        descriptorPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
                /*flags*/         {},
                /*maxSets*/       1,
                /*poolSizeCount*/ 1,
                /*pPoolSizes*/    &descriptorPoolSize
        });
        descriptorSet = device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
                /*descriptorPool*/     *descriptorPool,
                /*descriptorSetCount*/ 1,
                /*pSetLayouts*/        &*descriptorSetLayout
        }).front();
        uniformBuffer = vma::StreamBuffer(vma, vk::BufferCreateInfo{
                /*flags*/                 {},
                /*size*/                  8,
                /*usage*/                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
                /*sharingMode*/           vk::SharingMode::eExclusive,
                /*queueFamilyIndexCount*/ 0,
                /*pQueueFamilyIndices*/   nullptr
        });
        vk::DescriptorBufferInfo descriptorBufferInfo{
                /*buffer*/ *uniformBuffer,
                /*offset*/ 0,
                /*range*/  VK_WHOLE_SIZE
        };
        device->updateDescriptorSets({
            vk::WriteDescriptorSet{
                /*dstSet*/           descriptorSet,
                /*dstBinding*/       0,
                /*dstArrayElement*/  0,
                /*descriptorCount*/  1,
                /*descriptorType*/   vk::DescriptorType::eUniformBuffer,
                /*pImageInfo*/       nullptr,
                /*pBufferInfo*/      &descriptorBufferInfo,
                /*pTexelBufferView*/ nullptr
            }
        }, {});


        triangleDrawIndirectBuffer = vma::StreamBuffer(vma, vk::BufferCreateInfo{
                /*flags*/                 {},
                /*size*/                  sizeof(vk::DrawIndexedIndirectCommand),
                /*usage*/                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndirectBuffer,
                /*sharingMode*/           vk::SharingMode::eExclusive,
                /*queueFamilyIndexCount*/ 0,
                /*pQueueFamilyIndices*/   nullptr
        });

        polygonDrawIndirectBuffer = vma::StreamBuffer(vma, vk::BufferCreateInfo{
                /*flags*/                 {},
                /*size*/                  sizeof(vk::DrawIndirectCommand),
                /*usage*/                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndirectBuffer,
                /*sharingMode*/           vk::SharingMode::eExclusive,
                /*queueFamilyIndexCount*/ 0,
                /*pQueueFamilyIndices*/   nullptr
        });

    }



    void recordCommandBuffers() {
        device->resetCommandPool(*commandPool, {});
        for (uint32_t i = 0; i < swapchain.images.size(); i++) {
            vk::CommandBuffer commandBuffer = swapchainContext.commandBuffers[i];
            commandBuffer.begin(vk::CommandBufferBeginInfo{
                    /*flags*/            {},
                    /*pInheritanceInfo*/ nullptr
            });
            vk::ClearValue clearColor {vk::ClearColorValue {std::array<float, 4> {1.0F, 1.0F, 1.0F, 1.0F}}};
            commandBuffer.beginRenderPass(vk::RenderPassBeginInfo{
                    /*renderPass*/      *renderPass,
                    /*framebuffer*/     *swapchainContext.framebuffers[i],
                    /*renderArea*/      {{0, 0}, swapchain.extent},
                    /*clearValueCount*/ 1,
                    /*pClearValues*/    &clearColor
            }, vk::SubpassContents::eInline);
            commandBuffer.setViewport(0, vk::Viewport{
                    /*x*/        0,
                    /*y*/        0,
                    /*width*/    (float) swapchain.extent.width,
                    /*height*/   (float) swapchain.extent.height,
                    /*minDepth*/ 0,
                    /*maxDepth*/ 1
            });
            commandBuffer.setScissor(0, vk::Rect2D{{0, 0}, swapchain.extent});
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, descriptorSet, {});
            if(vertexBuffer) {
                commandBuffer.bindVertexBuffers(0, {*vertexBuffer}, {0});
                if(triangleIndexBuffer) {
                    commandBuffer.bindIndexBuffer(*triangleIndexBuffer, 0, vk::IndexType::eUint32);
                    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *trianglePipeline);
                    commandBuffer.drawIndexedIndirect(*triangleDrawIndirectBuffer, 0, 1, 0);
                }
            }
            if(polygonVerticesBuffer) {
                commandBuffer.bindVertexBuffers(0, {*polygonVerticesBuffer}, {0});
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *polygonEdgePipeline);
                commandBuffer.drawIndirect(*polygonDrawIndirectBuffer, 0, 1, 0);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *polygonVertexPipeline);
                commandBuffer.drawIndirect(*polygonDrawIndirectBuffer, 0, 1, 0);
            }
            if(vertexBuffer && triangleIndexBuffer) {
                commandBuffer.bindVertexBuffers(0, {*vertexBuffer}, {0});
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *triangleEdgePipeline);
                commandBuffer.drawIndexedIndirect(*triangleDrawIndirectBuffer, 0, 1, 0);
            }
            commandBuffer.endRenderPass();
            commandBuffer.end();
        }
    }



    void updateSwapchainContext() {
        device->waitForFences({*renderingCompleteFence}, true, -1);

        swapchainContext = {};
        Swapchain newSwapchain = Swapchain::create(*this, swapchain);
        swapchain = std::move(newSwapchain);

        swapchainContext.image = vma::UnmappedImage(*device, vma,
                vk::ImageCreateInfo{
                        /*flags*/                 {},
                        /*imageType*/             vk::ImageType::e2D,
                        /*format*/                graphicSettings.imageFormat,
                        /*extent*/                vk::Extent3D(swapchain.extent, 1),
                        /*mipLevels*/             1,
                        /*arrayLayers*/           1,
                        /*samples*/               graphicSettings.sampleCount,
                        /*tiling*/                vk::ImageTiling::eOptimal,
                        /*usage*/                 vk::ImageUsageFlagBits::eColorAttachment,
                        /*sharingMode*/           vk::SharingMode::eExclusive,
                        /*queueFamilyIndexCount*/ 0,
                        /*pQueueFamilyIndices*/   nullptr,
                        /*initialLayout*/         vk::ImageLayout::eUndefined
                },
                vk::ImageViewCreateInfo{
                        /*flags*/            {},
                        /*image*/            {},
                        /*viewType*/         vk::ImageViewType::e2D,
                        /*format*/           graphicSettings.imageFormat,
                        /*components*/       {},
                        /*subresourceRange*/ vk::ImageSubresourceRange{
                                /*aspectMask*/     vk::ImageAspectFlagBits::eColor,
                                /*baseMipLevel*/   0,
                                /*levelCount*/     1,
                                /*baseArrayLayer*/ 0,
                                /*layerCount*/     1
                        }
                }
        );

        swapchainContext.framebuffers.reserve(swapchain.images.size());
        for (size_t i = 0; i < swapchain.images.size(); i++) {
            vk::ImageView views[] {swapchainContext.image.view, *swapchain.imageViews[i]};
            swapchainContext.framebuffers.push_back(device->createFramebufferUnique(vk::FramebufferCreateInfo{
                    /*flags*/           {},
                    /*renderPass*/      *renderPass,
                    /*attachmentCount*/ 2,
                    /*pAttachments*/    views,
                    /*width*/           swapchain.extent.width,
                    /*height*/          swapchain.extent.height,
                    /*layers*/          1
            }));
        }

        device->freeCommandBuffers(*commandPool, swapchainContext.commandBuffers);
        swapchainContext.commandBuffers = device->allocateCommandBuffers(vk::CommandBufferAllocateInfo{
                /*commandPool*/        *commandPool,
                /*level*/              vk::CommandBufferLevel::ePrimary,
                /*commandBufferCount*/ (uint32_t) swapchain.images.size()
        });

        recordCommandBuffers();

    }



    bool ensureBufferSize(vma::StreamBuffer& buffer, vk::DeviceSize size, vk::BufferUsageFlagBits usage) {
        if(!buffer || buffer.allocationInfo.size < size) {
            buffer = vma::StreamBuffer(vma, vk::BufferCreateInfo{
                    /*flags*/                 {},
                    /*size*/                  size,
                    /*usage*/                 vk::BufferUsageFlagBits::eTransferDst | usage,
                    /*sharingMode*/           vk::SharingMode::eExclusive,
                    /*queueFamilyIndexCount*/ 0,
                    /*pQueueFamilyIndices*/   nullptr
            });
            return false;
        }
        else return true;
    }



    void render(const std::vector<std::vector<glm::dvec2>>& polygonSet, const Triangulation* const triangulation, glm::dvec2 scale) {
        device->waitForFences({*renderingCompleteFence}, true, -1);
        device->resetFences({*renderingCompleteFence});

        {
            auto extent = (glm::vec2*) uniformBuffer.allocationInfo.pMappedData;
            extent->x = (float) swapchain.extent.width / scale.x;
            extent->y = (float) swapchain.extent.height / scale.y;
        }
        bool reRecordBuffer = false;
        if(triangulation != nullptr) {
            if(!triangulation->vertices.empty()) {
                if(!ensureBufferSize(vertexBuffer, triangulation->vertices.size() * sizeof(glm::vec2) * 2, vk::BufferUsageFlagBits::eVertexBuffer)) {
                    reRecordBuffer = true;
                }
                auto vertices = (glm::vec2*) vertexBuffer.allocationInfo.pMappedData;
                for (int i = 0; i < triangulation->vertices.size(); i++) {
                    vertices[i] = glm::vec2(triangulation->vertices[i]);
                }
                vertexBuffer.flush(0, VK_WHOLE_SIZE);
            }
            if(!triangulation->triangles.empty()) {
                if(!ensureBufferSize(triangleIndexBuffer, triangulation->triangles.size() * sizeof(glm::ivec3) * 2, vk::BufferUsageFlagBits::eIndexBuffer)) {
                    reRecordBuffer = true;
                }
                std::memcpy(triangleIndexBuffer.allocationInfo.pMappedData, triangulation->triangles.data(), triangulation->triangles.size() * sizeof(glm::ivec3));
                triangleIndexBuffer.flush(0, VK_WHOLE_SIZE);
            }
        }
        *((vk::DrawIndexedIndirectCommand*) triangleDrawIndirectBuffer.allocationInfo.pMappedData) = vk::DrawIndexedIndirectCommand {
                /*indexCount*/    triangulation == nullptr ? 0 : (uint32_t) triangulation->triangles.size() * 3,
                /*instanceCount*/ 1,
                /*firstIndex*/    0,
                /*vertexOffset*/  0,
                /*firstInstance*/ 0
        };
        triangleDrawIndirectBuffer.flush(0, VK_WHOLE_SIZE);

        int polygonPoints = 0;
        if(!polygonSet.empty()) {
            for(const std::vector<glm::dvec2>& polygon : polygonSet) polygonPoints += polygon.size();
            if(!ensureBufferSize(polygonVerticesBuffer, polygonPoints * sizeof(glm::vec2) * 4, vk::BufferUsageFlagBits::eVertexBuffer)) {
                reRecordBuffer = true;
            }
            auto vertices = (glm::vec2*) polygonVerticesBuffer.allocationInfo.pMappedData;
            int counter = 0;
            for(const std::vector<glm::dvec2>& polygon : polygonSet) {
                for (int i = 0; i < polygon.size(); i++) {
                    glm::dvec2 vertex = polygon[i];
                    glm::dvec2 nextVertex = polygon[(i + 1) % polygon.size()];
                    vertices[counter] = glm::vec2(vertex);
                    vertices[counter + 1] = glm::vec2(nextVertex);
                    counter += 2;
                }
            }
            polygonVerticesBuffer.flush(0, VK_WHOLE_SIZE);
        }
        *((vk::DrawIndirectCommand*) polygonDrawIndirectBuffer.allocationInfo.pMappedData) = vk::DrawIndirectCommand{
                /*vertexCount*/   (uint32_t) polygonPoints * 2,
                /*instanceCount*/ 1,
                /*firstVertex*/   0,
                /*firstInstance*/ 0
        };
        polygonDrawIndirectBuffer.flush(0, VK_WHOLE_SIZE);

        if(reRecordBuffer) recordCommandBuffers();

        uint32_t image = !device->acquireNextImageKHR(*swapchain, -1, *acquireImageSemaphore, {});
        vk::CommandBuffer commandBuffer = swapchainContext.commandBuffers[image];
        vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        queue.submit(vk::SubmitInfo{
                /*waitSemaphoreCount*/   1,
                /*pWaitSemaphores*/      &*acquireImageSemaphore,
                /*pWaitDstStageMask*/    &waitDstStageMask,
                /*commandBufferCount*/   1,
                /*pCommandBuffers*/      &commandBuffer,
                /*signalSemaphoreCount*/ 1,
                /*pSignalSemaphores*/    &*renderingCompleteSemaphore
        }, *renderingCompleteFence);
        queue.presentKHR(vk::PresentInfoKHR{
                /*waitSemaphoreCount*/ 1,
                /*pWaitSemaphores*/    &*renderingCompleteSemaphore,
                /*swapchainCount*/     1,
                /*pSwapchains*/        &*swapchain,
                /*pImageIndices*/      &image,
                /*pResults*/           nullptr
        });
    }


};