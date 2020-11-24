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
#include"VulkanWindow.hpp"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
  class HobovrVirtualDisplayComponent : public IVRVirtualDisplay {
  public:
    HobovrVirtualDisplayComponent() {
        try {
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
    void createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
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
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }
    0

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

    void createImage(VKDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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

        endSingleTimeCommands(commandBuffer);
    }


    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
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

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkCommandPool commandPool;

    // IVRVirtualDisplay interface
  public:
    void Present(const PresentInfo_t *pPresentInfo, uint32_t unPresentInfoSize){
        try {
            VRVulkanTextureData_t* vdata = (VRVulkanTextureData_t*)pPresentInfo->backbufferTextureHandle;

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            VkDeviceSize imageSize = vdata->m_nWidth * vdata->m_nHeight * 4;

            createBuffer(vdata->m_pDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            stbi_uc* pixels;
            vkMapMemory(vdata->m_pDevice, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(vdata->m_pDevice, stagingBufferMemory);

            stbi_image_free(pixels);

            createImage(vdata->m_nWidth, vdata->m_nHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

            transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(vdata->m_nWidth), static_cast<uint32_t>(vdata->m_nHeight));
            transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vkDestroyBuffer(vdata->m_pDevice, stagingBuffer, nullptr);
            vkFreeMemory(vdata->m_pDevice, stagingBufferMemory, nullptr);
        }catch (const std::exception& e) {
            std::stringstream s;
            s << "VulkanWindow:" << e.what() << std::endl;
            DriverLog(s.str().c_str());
        }
    }
    void WaitForPresent(){
        return; // present immedietely for now.
    }
    bool GetTimeSinceLastVsync(float *pfSecondsSinceLastVsync, uint64_t *pulFrameCounter){
        return true; //give thumbs up and don't modify anything for now.
    }
  private:
    VulkanWindow app;
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
