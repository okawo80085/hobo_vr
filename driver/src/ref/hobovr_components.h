#pragma once

#ifndef HOBOVR_COMPONENTS_H
#define HOBOVR_COMPONENTS_H

#define VULKAN_DRIVER
#ifdef VULKAN_DRIVER
#include "d3drender.h"
#endif

//#include "openvr.h"
#include "../openvr_driver.h"
//#include "openvr_capi.h"
#include "../driverlog.h"

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

#ifdef VULKAN_DRIVER
  // this is a port of the ALVR DirectModeComponent to Vulkan: https://github.com/polygraphene/ALVR
  class HobovrDriverDirectModeComponent : public vr::IVRDriverDirectModeComponent {
  public:
      DirectModeComponent(std::shared_ptr<CD3DRender> pD3DRender,
          std::shared_ptr<CEncoder> pEncoder,
          std::shared_ptr<Listener> Listener,
          std::shared_ptr<RecenterManager> recenterManager)
          : m_pD3DRender(pD3DRender)
          , m_pEncoder(pEncoder)
          , m_Listener(Listener)
          , m_recenterManager(recenterManager)
          , m_poseMutex(NULL)
          , m_submitLayer(0)
          , m_LastReferencedFrameIndex(0)
          , m_LastReferencedClientTime(0) {
      }

      void OnPoseUpdated(TrackingInfo &info) {
          // Put pose history buffer
          TrackingHistoryFrame history;
          history.info = info;

          vr::HmdQuaternion_t recentered = m_recenterManager->GetRecenteredHMD();
          HmdMatrix_QuatToMat(recentered.w,
              recentered.x,
              recentered.y,
              recentered.z,
              &history.rotationMatrix);

          Log(L"Rotation Matrix=(%f, %f, %f, %f) (%f, %f, %f, %f) (%f, %f, %f, %f)"
              , history.rotationMatrix.m[0][0], history.rotationMatrix.m[0][1], history.rotationMatrix.m[0][2], history.rotationMatrix.m[0][3]
              , history.rotationMatrix.m[1][0], history.rotationMatrix.m[1][1], history.rotationMatrix.m[1][2], history.rotationMatrix.m[1][3]
              , history.rotationMatrix.m[2][0], history.rotationMatrix.m[2][1], history.rotationMatrix.m[2][2], history.rotationMatrix.m[2][3]);

          m_poseMutex.Wait(INFINITE);
          if (m_poseBuffer.size() == 0) {
              m_poseBuffer.push_back(history);
          }
          else {
              if (m_poseBuffer.back().info.FrameIndex != info.FrameIndex) {
                  // New track info
                  m_poseBuffer.push_back(history);
              }
          }
          if (m_poseBuffer.size() > 10) {
              m_poseBuffer.pop_front();
          }
          m_poseMutex.Release();

          m_LastReferencedFrameIndex = info.FrameIndex;
          m_LastReferencedClientTime = info.clientTime;
      }

      /** Specific to Oculus compositor support, textures supplied must be created using this method. */
      virtual void CreateSwapTextureSet(uint32_t unPid, const SwapTextureSetDesc_t *pSwapTextureSetDesc, vr::SharedTextureHandle_t(*pSharedTextureHandles)[3]) override
      {
          Log(L"CreateSwapTextureSet pid=%d Format=%d %dx%d SampleCount=%d", unPid, pSwapTextureSetDesc->nFormat
              , pSwapTextureSetDesc->nWidth, pSwapTextureSetDesc->nHeight, pSwapTextureSetDesc->nSampleCount);

          //HRESULT hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, NULL, 0, D3D11_SDK_VERSION, &pDevice, &eFeatureLevel, &pContext);

          D3D11_TEXTURE2D_DESC SharedTextureDesc = {};
          SharedTextureDesc.ArraySize = 1;
          SharedTextureDesc.MipLevels = 1;
          SharedTextureDesc.SampleDesc.Count = pSwapTextureSetDesc->nSampleCount;
          SharedTextureDesc.SampleDesc.Quality = 0;
          SharedTextureDesc.Usage = D3D11_USAGE_DEFAULT;
          SharedTextureDesc.Format = (DXGI_FORMAT)pSwapTextureSetDesc->nFormat;

          // Some(or all?) applications request larger texture than we specified in GetRecommendedRenderTargetSize.
          // But, we must create textures in requested size to prevent cropped output. And then we must shrink texture to H.264 movie size.
          SharedTextureDesc.Width = pSwapTextureSetDesc->nWidth;
          SharedTextureDesc.Height = pSwapTextureSetDesc->nHeight;

          SharedTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
          //SharedTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
          SharedTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

          ProcessResource *processResource = new ProcessResource();
          processResource->pid = unPid;

          for (int i = 0; i < 3; i++) {
              HRESULT hr = m_pD3DRender->GetDevice()->CreateTexture2D(&SharedTextureDesc, NULL, &processResource->textures[i]);
              //Log(L"texture%d %p res:%d %s", i, texture[i], hr, GetDxErrorStr(hr).c_str());

              IDXGIResource* pResource;
              hr = processResource->textures[i]->QueryInterface(__uuidof(IDXGIResource), (void**)&pResource);
              //Log(L"QueryInterface %p res:%d %s", pResource, hr, GetDxErrorStr(hr).c_str());

              hr = pResource->GetSharedHandle(&processResource->sharedHandles[i]);
              //Log(L"GetSharedHandle %p res:%d %s", processResource->sharedHandles[i], hr, GetDxErrorStr(hr).c_str());

              m_handleMap.insert(std::make_pair(processResource->sharedHandles[i], std::make_pair(processResource, i)));

              (*pSharedTextureHandles)[i] = (vr::SharedTextureHandle_t)processResource->sharedHandles[i];

              pResource->Release();

              Log(L"Created Texture %d %p", i, processResource->sharedHandles[i]);
          }
          //m_processMap.insert(std::pair<uint32_t, ProcessResource *>(unPid, processResource));
      }

      /** Used to textures created using CreateSwapTextureSet.  Only one of the set's handles needs to be used to destroy the entire set. */
      virtual void DestroySwapTextureSet(vr::SharedTextureHandle_t sharedTextureHandle) override
      {
          Log(L"DestroySwapTextureSet %p", sharedTextureHandle);

          auto it = m_handleMap.find((HANDLE)sharedTextureHandle);
          if (it != m_handleMap.end()) {
              // Release all reference (a bit forcible)
              ProcessResource *p = it->second.first;
              m_handleMap.erase(p->sharedHandles[0]);
              m_handleMap.erase(p->sharedHandles[1]);
              m_handleMap.erase(p->sharedHandles[2]);
              delete p;
          }
          else {
              Log(L"Requested to destroy not managing texture. handle:%p", sharedTextureHandle);
          }
      }

      /** Used to purge all texture sets for a given process. */
      virtual void DestroyAllSwapTextureSets(uint32_t unPid) override
      {
          Log(L"DestroyAllSwapTextureSets pid=%d", unPid);

          for (auto it = m_handleMap.begin(); it != m_handleMap.end();) {
              if (it->second.first->pid == unPid) {
                  if (it->second.second == 0) {
                      delete it->second.first;
                  }
                  m_handleMap.erase(it++);
              }
              else {
                  ++it;
              }
          }
      }

      /** After Present returns, calls this to get the next index to use for rendering. */
      virtual void GetNextSwapTextureSetIndex(vr::SharedTextureHandle_t sharedTextureHandles[2], uint32_t(*pIndices)[2]) override
      {
          Log(L"GetNextSwapTextureSetIndex %p %p %d %d", sharedTextureHandles[0], sharedTextureHandles[1], (*pIndices)[0], (*pIndices)[1]);
          (*pIndices)[0]++;
          (*pIndices)[0] %= 3;
          (*pIndices)[1]++;
          (*pIndices)[1] %= 3;
      }

      /** Call once per layer to draw for this frame.  One shared texture handle per eye.  Textures must be created
      * using CreateSwapTextureSet and should be alternated per frame.  Call Present once all layers have been submitted. */
      virtual void SubmitLayer(const SubmitLayerPerEye_t(&perEye)[2], const vr::HmdMatrix34_t *pPose) override
      {
          Log(L"SubmitLayer Handles=%p,%p DepthHandles=%p,%p %f-%f,%f-%f %f-%f,%f-%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f"
              , perEye[0].hTexture, perEye[1].hTexture, perEye[0].hDepthTexture, perEye[1].hDepthTexture
              , perEye[0].bounds.uMin, perEye[0].bounds.uMax, perEye[0].bounds.vMin, perEye[0].bounds.vMax
              , perEye[1].bounds.uMin, perEye[1].bounds.uMax, perEye[1].bounds.vMin, perEye[1].bounds.vMax
              , pPose->m[0][0], pPose->m[0][1], pPose->m[0][2], pPose->m[0][3]
              , pPose->m[1][0], pPose->m[1][1], pPose->m[1][2], pPose->m[1][3]
              , pPose->m[2][0], pPose->m[2][1], pPose->m[2][2], pPose->m[2][3]
          );
          // pPose is qRotation which is calculated by SteamVR using vr::DriverPose_t::qRotation.
          // pPose->m[0][0], pPose->m[0][1], pPose->m[0][2],
          // pPose->m[1][0], pPose->m[1][1], pPose->m[1][2],
          // pPose->m[2][0], pPose->m[2][1], pPose->m[2][2],
          // position
          // x = pPose->m[0][3], y = pPose->m[1][3], z = pPose->m[2][3]

          if (m_submitLayer == 0) {
              // Detect FrameIndex of submitted frame by pPose.
              // This is important part to achieve smooth headtracking.
              // We search for history of TrackingInfo and find the TrackingInfo which have nearest matrix value.

              m_poseMutex.Wait(INFINITE);
              float minDiff = 100000;
              int index = 0;
              int minIndex = 0;
              auto minIt = m_poseBuffer.begin();
              for (auto it = m_poseBuffer.begin(); it != m_poseBuffer.end(); it++, index++) {
                  float distance = 0;
                  // Rotation matrix composes a part of ViewMatrix of TrackingInfo.
                  // Be carefull of transpose.
                  // And bottom side and right side of matrix should not be compared, because pPose does not contain that part of matrix.
                  for (int i = 0; i < 3; i++) {
                      for (int j = 0; j < 3; j++) {
                          distance += pow(it->rotationMatrix.m[j][i] - pPose->m[j][i], 2);
                      }
                  }
                  //Log(L"diff %f %llu", distance, it->info.FrameIndex);
                  if (minDiff > distance) {
                      minIndex = index;
                      minIt = it;
                      minDiff = distance;
                  }
              }
              if (minIt != m_poseBuffer.end()) {
                  // found the frameIndex
                  m_prevSubmitFrameIndex = m_submitFrameIndex;
                  m_prevSubmitClientTime = m_submitClientTime;
                  m_submitFrameIndex = minIt->info.FrameIndex;
                  m_submitClientTime = minIt->info.clientTime;

                  m_prevFramePoseRotation = m_framePoseRotation;
                  m_framePoseRotation.x = minIt->info.HeadPose_Pose_Orientation.x;
                  m_framePoseRotation.y = minIt->info.HeadPose_Pose_Orientation.y;
                  m_framePoseRotation.z = minIt->info.HeadPose_Pose_Orientation.z;
                  m_framePoseRotation.w = minIt->info.HeadPose_Pose_Orientation.w;

                  Log(L"Frame pose found. m_prevSubmitFrameIndex=%llu m_submitFrameIndex=%llu minDiff=%f", m_prevSubmitFrameIndex, m_submitFrameIndex, minDiff);
              }
              else {
                  m_submitFrameIndex = 0;
                  m_submitClientTime = 0;
                  m_framePoseRotation = HmdQuaternion_Init(0.0, 0.0, 0.0, 0.0);
              }
              m_poseMutex.Release();
          }
          if (m_submitLayer < MAX_LAYERS) {
              m_submitLayers[m_submitLayer][0] = perEye[0];
              m_submitLayers[m_submitLayer][1] = perEye[1];
              m_submitLayer++;
          }
          else {
              Log(L"Too many layers submitted!");
          }

          if (g_DriverTestMode & 8) {
              // Crash test
              *(char*)0 = 1;
          }

          //CopyTexture();
      }

      /** Submits queued layers for display. */
      virtual void Present(vr::SharedTextureHandle_t syncTexture) override
      {
          bool useMutex = Settings::Instance().m_UseKeyedMutex;
          Log(L"Present syncTexture=%p (use:%d) m_prevSubmitFrameIndex=%llu m_submitFrameIndex=%llu", syncTexture, useMutex, m_prevSubmitFrameIndex, m_submitFrameIndex);

          IDXGIKeyedMutex *pKeyedMutex = NULL;

          uint32_t layerCount = m_submitLayer;
          m_submitLayer = 0;

          if (m_prevSubmitFrameIndex == m_submitFrameIndex) {
              Log(L"Discard duplicated frame. FrameIndex=%llu", m_submitFrameIndex);
              return;
          }

          ID3D11Texture2D *pSyncTexture = m_pD3DRender->GetSharedTexture((HANDLE)syncTexture);
          if (!pSyncTexture)
          {
              Log(L"[VDispDvr] SyncTexture is NULL!");
              return;
          }

          if (useMutex) {
              // Access to shared texture must be wrapped in AcquireSync/ReleaseSync
              // to ensure the compositor has finished rendering to it before it gets used.
              // This enforces scheduling of work on the gpu between processes.
              if (SUCCEEDED(pSyncTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void **)&pKeyedMutex)))
              {
                  Log(L"[VDispDvr] Wait for SyncTexture Mutex.");
                  // TODO: Reasonable timeout and timeout handling
                  HRESULT hr = pKeyedMutex->AcquireSync(0, 10);
                  if (hr != S_OK)
                  {
                      Log(L"[VDispDvr] ACQUIRESYNC FAILED!!! hr=%d %p %s", hr, hr, GetDxErrorStr(hr).c_str());
                      pKeyedMutex->Release();
                      return;
                  }
              }

              Log(L"[VDispDvr] Mutex Acquired.");
          }

          CopyTexture(layerCount);

          if (useMutex) {
              if (pKeyedMutex)
              {
                  pKeyedMutex->ReleaseSync(0);
                  pKeyedMutex->Release();
              }
              Log(L"[VDispDvr] Mutex Released.");
          }

          m_pEncoder->NewFrameReady();
      }

      void CopyTexture(uint32_t layerCount) {

          uint64_t presentationTime = GetTimestampUs();

          ID3D11Texture2D *pTexture[MAX_LAYERS][2];
          ComPtr<ID3D11Texture2D> Texture[MAX_LAYERS][2];
          vr::VRTextureBounds_t bounds[MAX_LAYERS][2];

          for (uint32_t i = 0; i < layerCount; i++) {
              // Find left eye texture.
              HANDLE leftEyeTexture = (HANDLE)m_submitLayers[i][0].hTexture;
              auto it = m_handleMap.find(leftEyeTexture);
              if (it == m_handleMap.end()) {
                  // Ignore this layer.
                  Log(L"Submitted texture is not found on HandleMap. eye=right layer=%d/%d Texture Handle=%p", i, layerCount, leftEyeTexture);
              }
              else {
                  Texture[i][0] = it->second.first->textures[it->second.second];
                  D3D11_TEXTURE2D_DESC desc;
                  Texture[i][0]->GetDesc(&desc);

                  Log(L"CopyTexture: layer=%d/%d pid=%d Texture Size=%dx%d Format=%d", i, layerCount, it->second.first->pid, desc.Width, desc.Height, desc.Format);

                  // Find right eye texture.
                  HANDLE rightEyeTexture = (HANDLE)m_submitLayers[i][1].hTexture;
                  it = m_handleMap.find(rightEyeTexture);
                  if (it == m_handleMap.end()) {
                      // Ignore this layer
                      Log(L"Submitted texture is not found on HandleMap. eye=left layer=%d/%d Texture Handle=%p", i, layerCount, rightEyeTexture);
                      Texture[i][0].Reset();
                  }
                  else {
                      Texture[i][1] = it->second.first->textures[it->second.second];
                  }
              }

              pTexture[i][0] = Texture[i][0].Get();
              pTexture[i][1] = Texture[i][1].Get();
              bounds[i][0] = m_submitLayers[i][0].bounds;
              bounds[i][1] = m_submitLayers[i][1].bounds;
          }

          // This can go away, but is useful to see it as a separate packet on the gpu in traces.
          m_pD3DRender->GetContext()->Flush();

          Log(L"Waiting for finish of previous encode.");

          if (Settings::Instance().m_captureLayerDDSTrigger) {
              wchar_t buf[1000];

              for (uint32_t i = 0; i < layerCount; i++) {
                  Log(L"Writing Debug DDS. m_LastReferencedFrameIndex=%llu layer=%d/%d", 0, i, layerCount);
                  _snwprintf_s(buf, sizeof(buf), L"%hs\\debug-%llu-%d-%d.dds", Settings::Instance().m_DebugOutputDir.c_str(), m_submitFrameIndex, i, layerCount);
                  HRESULT hr = DirectX::SaveDDSTextureToFile(m_pD3DRender->GetContext(), pTexture[i][0], buf);
                  Log(L"Writing Debug DDS: End hr=%p %s", hr, GetDxErrorStr(hr).c_str());
              }
              Settings::Instance().m_captureLayerDDSTrigger = false;
          }

          // Wait for the encoder to be ready.  This is important because the encoder thread
          // blocks on transmit which uses our shared d3d context (which is not thread safe).
          m_pEncoder->WaitForEncode();

          std::string debugText;

          if (Settings::Instance().m_DebugFrameIndex) {
              TrackingInfo info;
              m_Listener->GetTrackingInfo(info);

              char buf[2000];
              snprintf(buf, sizeof(buf), "%llu\n%f\n%f", m_prevSubmitFrameIndex, m_prevFramePoseRotation.x, info.HeadPose_Pose_Orientation.x);
              debugText = buf;
          }

          uint64_t submitFrameIndex = m_submitFrameIndex + Settings::Instance().m_trackingFrameOffset;
          Log(L"Fix frame index. FrameIndex=%llu Offset=%d New FrameIndex=%llu"
              , m_submitFrameIndex, Settings::Instance().m_trackingFrameOffset, submitFrameIndex);

          // Copy entire texture to staging so we can read the pixels to send to remote device.
          m_pEncoder->CopyToStaging(pTexture, bounds, layerCount, m_recenterManager->IsRecentering(), presentationTime, submitFrameIndex, m_submitClientTime, m_recenterManager->GetFreePIEMessage(), debugText);

          m_pD3DRender->GetContext()->Flush();
      }

  private:
      std::shared_ptr<CD3DRender> m_pD3DRender;
      std::shared_ptr<CEncoder> m_pEncoder;
      std::shared_ptr<Listener> m_Listener;
      std::shared_ptr<RecenterManager> m_recenterManager;

      // Resource for each process
      struct ProcessResource {
          ComPtr<ID3D11Texture2D> textures[3];
          HANDLE sharedHandles[3];
          uint32_t pid;
      };
      std::map<HANDLE, std::pair<ProcessResource *, int> > m_handleMap;

      static const int MAX_LAYERS = 10;
      int m_submitLayer;
      SubmitLayerPerEye_t m_submitLayers[MAX_LAYERS][2];
      vr::HmdQuaternion_t m_prevFramePoseRotation;
      vr::HmdQuaternion_t m_framePoseRotation;
      uint64_t m_submitFrameIndex;
      uint64_t m_submitClientTime;
      uint64_t m_prevSubmitFrameIndex;
      uint64_t m_prevSubmitClientTime;

      uint64_t m_LastReferencedFrameIndex;
      uint64_t m_LastReferencedClientTime;

      IPCMutex m_poseMutex;
      struct TrackingHistoryFrame {
          TrackingInfo info;
          vr::HmdMatrix34_t rotationMatrix;
      };
      std::list<TrackingHistoryFrame> m_poseBuffer;
  };
#endif
  // this is a dummy class meant to expand the component handling system, DO NOT USE THIS!
  class HobovrCameraComponent {
  public:
    HobovrCameraComponent() {}
  };

  // this is a dummy class meant to expand the component handling system, DO NOT USE THIS!
  class HobovrVirtualDisplayComponent {
  public:
    HobovrVirtualDisplayComponent() {}
  };

}

#endif // HOBOVR_COMPONENTS_H
