#pragma once

#ifndef HOBOVR_COMPONENTS_H
#define HOBOVR_COMPONENTS_H

// The includes here allow this file to be imported anywhere:
#include "../openvr_driver.h"
#include "../driverlog.h"
#include <cmath>
#include <sstream>
#include <vulkan/vulkan.h>

#if defined (USING_VULKAN)
#include <vulkan/vulkan.h>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <iostream>
#include <optional>
#include <array>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif



using namespace vr;

namespace hobovr {
enum ELensMathType {
    Mt_Invalid = 0,
    Mt_Default = 1
};

// ext display component keys
static const char *const k_pch_ExtDisplay_Section = "hobovr_comp_extendedDisplay";
static const char *const k_pch_ExtDisplay_WindowX_Int32 = "windowX";
static const char *const k_pch_ExtDisplay_WindowY_Int32 = "windowY";
static const char *const k_pch_ExtDisplay_WindowWidth_Int32 = "windowWidth";
static const char *const k_pch_ExtDisplay_WindowHeight_Int32 = "windowHeight";
static const char *const k_pch_ExtDisplay_RenderWidth_Int32 = "renderWidth";
static const char *const k_pch_ExtDisplay_RenderHeight_Int32 = "renderHeight";
static const char *const k_pch_ExtDisplay_EyeGapOffset_Int = "EyeGapOffsetPx";
static const char *const k_pch_ExtDisplay_IsDisplayReal_Bool = "IsDisplayRealDisplay";
static const char *const k_pch_ExtDisplay_IsDisplayOnDesktop_bool = "IsDisplayOnDesktop";

// ext display component keys related to the ELensMathType::Mt_Default distortion type
static const char *const k_pch_ExtDisplay_DistortionK1_Float = "DistortionK1";
static const char *const k_pch_ExtDisplay_DistortionK2_Float = "DistortionK2";
static const char *const k_pch_ExtDisplay_ZoomWidth_Float = "ZoomWidth";
static const char *const k_pch_ExtDisplay_ZoomHeight_Float = "ZoomHeight";

// compile time component settings
static const bool HobovrExtDisplayComp_doLensStuff = true;
static const short HobovrExtDisplayComp_lensDistortionType = ELensMathType::Mt_Default; // has to be one of hobovr::ELensMathType

class HobovrExtendedDisplayComponent: public vr::IVRDisplayComponent {
public:
    HobovrExtendedDisplayComponent(){

        m_nWindowX = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                k_pch_ExtDisplay_WindowX_Int32);

        m_nWindowY = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                k_pch_ExtDisplay_WindowY_Int32);

        m_nWindowWidth = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                    k_pch_ExtDisplay_WindowWidth_Int32);

        m_nWindowHeight = vr::VRSettings()->GetInt32(
                    k_pch_ExtDisplay_Section, k_pch_ExtDisplay_WindowHeight_Int32);

        m_nRenderWidth = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                    k_pch_ExtDisplay_RenderWidth_Int32);

        m_nRenderHeight = vr::VRSettings()->GetInt32(
                    k_pch_ExtDisplay_Section, k_pch_ExtDisplay_RenderHeight_Int32);

        if constexpr(HobovrExtDisplayComp_doLensStuff){
            if constexpr(HobovrExtDisplayComp_lensDistortionType == ELensMathType::Mt_Default){
                m_fDistortionK1 = vr::VRSettings()->GetFloat(
                            k_pch_ExtDisplay_Section, k_pch_ExtDisplay_DistortionK1_Float);

                m_fDistortionK2 = vr::VRSettings()->GetFloat(
                            k_pch_ExtDisplay_Section, k_pch_ExtDisplay_DistortionK2_Float);

                m_fZoomWidth = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section,
                                                          k_pch_ExtDisplay_ZoomWidth_Float);

                m_fZoomHeight = vr::VRSettings()->GetFloat(k_pch_ExtDisplay_Section,
                                                           k_pch_ExtDisplay_ZoomHeight_Float);
            }
        }

        m_iEyeGapOff = vr::VRSettings()->GetInt32(k_pch_ExtDisplay_Section,
                                                  k_pch_ExtDisplay_EyeGapOffset_Int);

        m_bIsDisplayReal = vr::VRSettings()->GetBool(k_pch_ExtDisplay_Section,
                                                     k_pch_ExtDisplay_IsDisplayReal_Bool);

        m_bIsDisplayOnDesktop = vr::VRSettings()->GetBool(k_pch_ExtDisplay_Section,
                                                          k_pch_ExtDisplay_IsDisplayOnDesktop_bool);

        DriverLog("Ext_display: component created\n");
        DriverLog("Ext_display: lens distortion enable: %d", HobovrExtDisplayComp_doLensStuff);

        if constexpr(HobovrExtDisplayComp_doLensStuff){
            DriverLog("Ext_display: distortion math type: %d", HobovrExtDisplayComp_lensDistortionType);
            if constexpr(HobovrExtDisplayComp_lensDistortionType == ELensMathType::Mt_Default)
                    DriverLog("Ext_display: distortion coefficient: k1=%f, k2=%f, zw=%f, zh=%f", m_fDistortionK1, m_fDistortionK2, m_fZoomWidth, m_fZoomHeight);
        }

        DriverLog("Ext_display: eye gap offset: %d", m_iEyeGapOff);
        DriverLog("Ext_display: is display real: %d", (int)m_bIsDisplayReal);
        DriverLog("Ext_display: is display on desktop: %d", (int)m_bIsDisplayOnDesktop);
        DriverLog("Ext_display: window bounds: %d %d %d %d", m_nWindowX, m_nWindowY, m_nWindowWidth, m_nWindowHeight);
        DriverLog("Ext_display: render target: %d %d", m_nRenderWidth, m_nRenderHeight);
        DriverLog("Ext_display: left eye viewport: %d %d %d %d", 0, 0, m_nWindowWidth/2, m_nWindowHeight);
        DriverLog("Ext_display: right eye viewport: %d %d %d %d", m_nWindowWidth/2 + m_iEyeGapOff, 0, m_nWindowWidth/2, m_nWindowHeight);

    }

    virtual void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth,
                                 uint32_t *pnHeight) {
        *pnX = m_nWindowX;
        *pnY = m_nWindowY;
        *pnWidth = m_nWindowWidth;
        *pnHeight = m_nWindowHeight;
    }

    virtual bool IsDisplayOnDesktop() { return m_bIsDisplayOnDesktop; }

    virtual bool IsDisplayRealDisplay() { return m_bIsDisplayReal; }

    virtual void GetRecommendedRenderTargetSize(uint32_t *pnWidth,
                                                uint32_t *pnHeight) {
        *pnWidth = m_nRenderWidth;
        *pnHeight = m_nRenderHeight;
    }

    virtual void GetEyeOutputViewport(vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY,
                                      uint32_t *pnWidth, uint32_t *pnHeight) {
        *pnY = 0;
        *pnWidth = m_nWindowWidth / 2;
        *pnHeight = m_nWindowHeight;

        if (eEye == vr::Eye_Left) {
            *pnX = 0;
        } else {
            *pnX = m_nWindowWidth / 2 + m_iEyeGapOff;
        }
    }

    virtual void GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight,
                                  float *pfTop, float *pfBottom) {
        *pfLeft = -1.0;
        *pfRight = 1.0;
        *pfTop = -1.0;
        *pfBottom = 1.0;
    }

    virtual DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU,
                                                      float fV) {
        DistortionCoordinates_t coordinates;

        if constexpr(HobovrExtDisplayComp_doLensStuff) {
            if constexpr(HobovrExtDisplayComp_lensDistortionType == ELensMathType::Mt_Default) {
                // Distortion implementation from
                // https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc#L232

                // in here 0.5f is the distortion center
                double rr = sqrt((fU - 0.5f) * (fU - 0.5f) + (fV - 0.5f) * (fV - 0.5f));
                double r2 = rr * (1 + m_fDistortionK1 * (rr * rr) +
                                  m_fDistortionK2 * (rr * rr * rr * rr));
                double theta = atan2(fU - 0.5f, fV - 0.5f);
                auto hX = float(sin(theta) * r2) * m_fZoomWidth;
                auto hY = float(cos(theta) * r2) * m_fZoomHeight;

                coordinates.rfBlue[0] = hX + 0.5f;
                coordinates.rfBlue[1] = hY + 0.5f;
                coordinates.rfGreen[0] = hX + 0.5f;
                coordinates.rfGreen[1] = hY + 0.5f;
                coordinates.rfRed[0] = hX + 0.5f;
                coordinates.rfRed[1] = hY + 0.5f;
            }
        } else {
            coordinates.rfBlue[0] = fU;
            coordinates.rfBlue[1] = fV;
            coordinates.rfGreen[0] = fU;
            coordinates.rfGreen[1] = fV;
            coordinates.rfRed[0] = fU;
            coordinates.rfRed[1] = fV;
        }

        return coordinates;
    }

    const char* GetComponentNameAndVersion() {return vr::IVRDisplayComponent_Version;}

private:
    int32_t m_nWindowX;
    int32_t m_nWindowY;
    int32_t m_nWindowWidth;
    int32_t m_nWindowHeight;
    int32_t m_nRenderWidth;
    int32_t m_nRenderHeight;
    int32_t m_iEyeGapOff;

    bool m_bIsDisplayReal;
    bool m_bIsDisplayOnDesktop;

    float m_fDistortionK1;
    float m_fDistortionK2;
    float m_fZoomWidth;
    float m_fZoomHeight;
};

// this is a dummy class meant to expand the component handling system, DO NOT USE THIS!
class HobovrDriverDirectModeComponent {
public:
    HobovrDriverDirectModeComponent() {}
};

// this is a dummy class meant to expand the component handling system, DO NOT USE THIS!
class HobovrCameraComponent {
public:
    HobovrCameraComponent() {}
};

#if defined (USING_VULKAN)
class HobovrVirtualDisplayComponent : public IVRVirtualDisplay, HobovrExtendedDisplayComponent {
public:
    HobovrVirtualDisplayComponent() {
        try {
            DebugDriverLog("starting HobovrVirtualDisplayComponent");
            tp = std::chrono::system_clock::now();
            //app.initWindow();
            //app.initVulkan();
        } catch (const std::exception& e) {
            std::stringstream s;
            s << "VulkanWindow:" << e.what() << std::endl;
            DriverLog(s.str().c_str());
        }
    }
    ~HobovrVirtualDisplayComponent(){
        try {
            //app.cleanup();
        } catch (const std::exception& e) {
            std::stringstream s;
            s << "VulkanWindow:" << e.what() << std::endl;
            DriverLog(s.str().c_str());
        }
    }
private:
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }


    void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
                    commandBuffer,
                    sourceStage, destinationStage,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                    );

        endSingleTimeCommands(device, graphicsQueue, commandBuffer);
    }


    void copyImageToBuffer(VkDevice device, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, buffer, 1, &region);

        endSingleTimeCommands(device, graphicsQueue, commandBuffer);
    }

    VkCommandBuffer beginSingleTimeCommands(VkDevice device) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    VkCommandPool commandPool;

    // IVRVirtualDisplay interface
public:
    void Present(const PresentInfo_t *pPresentInfo, uint32_t unPresentInfoSize){
        try {
            DebugDriverLog("Converting texture pointer to vulkan");
            VRVulkanTextureData_t* vdata = (VRVulkanTextureData_t*)pPresentInfo->backbufferTextureHandle;

            std::stringstream ss;
            ss << "vdata is: "<< vdata;
            DebugDriverLog(ss.str().c_str());
            if(vdata!=nullptr){

                DebugDriverLog("Converting vulkan variables, stagingBuffer");
                VkBuffer stagingBuffer;
                DebugDriverLog("Converting vulkan variables, stagingBufferMemory");
                VkDeviceMemory stagingBufferMemory;
                DebugDriverLog("Converting vulkan variables, imageSize");
                DebugDriverLog("check");
                uint32_t width = vdata->m_nWidth;
                DebugDriverLog("math");
                uint32_t imageSize = vdata->m_nWidth * vdata->m_nHeight * 4;

                DebugDriverLog("Creating vulkan buffer");
                createBuffer(vdata->m_pPhysicalDevice, vdata->m_pDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

                DebugDriverLog("transition image layout 1.");
                //todo: m_pQueue is not allowed to be accessed in this function. Fix needs to delete all access to this.
                transitionImageLayout(vdata->m_pDevice,vdata->m_pQueue, (VkImage)vdata->m_nImage, (VkFormat)vdata->m_nFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                DebugDriverLog("copyImageToBuffer 1.");
                copyImageToBuffer(vdata->m_pDevice,vdata->m_pQueue, stagingBuffer, (VkImage)vdata->m_nImage, static_cast<uint32_t>(vdata->m_nWidth), static_cast<uint32_t>(vdata->m_nHeight));
                DebugDriverLog("transition image layout 1.");
                transitionImageLayout(vdata->m_pDevice,vdata->m_pQueue, (VkImage)vdata->m_nImage, (VkFormat)vdata->m_nFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                DebugDriverLog("Converting vulkan pointers");
                void* data;
                //stbi_uc* pixels;
                char* pixels;
                DebugDriverLog("vkMapMemory 1");
                vkMapMemory(vdata->m_pDevice, stagingBufferMemory, 0, imageSize, 0, &data);
                DebugDriverLog("memcpy");
                memcpy(pixels, data, static_cast<size_t>(imageSize));
                DebugDriverLog("vkMapMemory 2");
                vkUnmapMemory(vdata->m_pDevice, stagingBufferMemory);

                DebugDriverLog(pixels);

                //stbi_image_free(pixels);
                DebugDriverLog("vkDestroyBuffer");
                vkDestroyBuffer(vdata->m_pDevice, stagingBuffer, nullptr);
                DebugDriverLog("vkFreeMemory");
                vkFreeMemory(vdata->m_pDevice, stagingBufferMemory, nullptr);
            }else{
                DebugDriverLog("Null texture handle. Skipping present.");
            }
        }catch (const std::exception& e) {
            std::stringstream s;
            s << "VulkanWindow:" << e.what() << std::endl;
            DriverLog(s.str().c_str());
        }
    }
    void WaitForPresent(){
        DebugDriverLog("WaitForPresent");
        return; // present immedietely for now.
    }

    virtual bool GetTimeSinceLastVsync( float *pfSecondsSinceLastVsync, uint64_t *pulFrameCounter ) override
    {
        try{
            DebugDriverLog("GetTimeSinceLastVsync 0");
            std::chrono::system_clock::time_point tp2 = std::chrono::system_clock::now();
            DebugDriverLog("GetTimeSinceLastVsync 1");
            dt = float(tp2.time_since_epoch().count()-tp.time_since_epoch().count()) * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
            DebugDriverLog("GetTimeSinceLastVsync 2 %f", dt);
            if (pfSecondsSinceLastVsync!=nullptr){
                *pfSecondsSinceLastVsync = dt ;
            }
            DebugDriverLog("GetTimeSinceLastVsync 3");
            //DebugDriverLog("GetTimeSinceLastVsync vars: %f, %d", pfSecondsSinceLastVsync, pulFrameCounter);
            if (pulFrameCounter!=nullptr){
                *pulFrameCounter = 0;
            }
            //DebugDriverLog("GetTimeSinceLastVsync pfSecondsSinceLastVsync: %f", *pfSecondsSinceLastVsync);
            //DebugDriverLog("GetTimeSinceLastVsync pfSecondsSinceLastVsync: %d", *pulFrameCounter);
            DebugDriverLog("GetTimeSinceLastVsync 4");
            tp = std::chrono::system_clock::now();
            DebugDriverLog("GetTimeSinceLastVsync 5");
            return true;
        }catch (const std::exception& e) {
            std::stringstream s;
            s << "VulkanWindow:" << e.what() << std::endl;
            DriverLog(s.str().c_str());
            return false;
        }
    }

    /*bool GetTimeSinceLastVsync(float *pfSecondsSinceLastVsync, uint64_t *pulFrameCounter){
        DebugDriverLog("GetTimeSinceLastVsync pulFrameCounter");
        if(pulFrameCounter!=nullptr){
            DebugDriverLog("GetTimeSinceLastVsync pulFrameCounter not null. ignoring it.");
            //*pulFrameCounter = 0;
        }
        if (pfSecondsSinceLastVsync!=nullptr){
            DebugDriverLog("GetTimeSinceLastVsync pfSecondsSinceLastVsync not null");
            *pfSecondsSinceLastVsync=0;
        }
        DebugDriverLog("GetTimeSinceLastVsync return");
        return false; //give no vsync
    }*/
    std::chrono::system_clock::time_point tp;
    float dt;
};
#else
// this is a dummy class meant to expand the component handling system, DO NOT USE THIS!
class HobovrVirtualDisplayComponent {
public:
    HobovrVirtualDisplayComponent() {}
};
#endif

}

#endif // HOBOVR_COMPONENTS_H
