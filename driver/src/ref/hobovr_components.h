#pragma once

#ifndef HOBOVR_COMPONENTS_H
#define HOBOVR_COMPONENTS_H

namespace hobovr {
  class HobovrExtendedDisplayComponent: public vr::IVRDisplayComponent{
  public:
    HobovrExtendedDisplayComponent(){

    }

    virtual void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth,
                                 uint32_t *pnHeight) {
      *pnX = m_nWindowX;
      *pnY = m_nWindowY;
      *pnWidth = m_nWindowWidth;
      *pnHeight = m_nWindowHeight;
    }

    virtual bool IsDisplayOnDesktop() { return true; }

    virtual bool IsDisplayRealDisplay() { return false; }

    virtual void GetRecommendedRenderTargetSize(uint32_t *pnWidth,
                                                uint32_t *pnHeight) {
      *pnWidth = m_nRenderWidth;
      *pnHeight = m_nRenderHeight;
    }

    virtual void GetEyeOutputViewport(EVREye eEye, uint32_t *pnX, uint32_t *pnY,
                                      uint32_t *pnWidth, uint32_t *pnHeight) {
      *pnY = 0;
      *pnWidth = m_nWindowWidth / 2;
      *pnHeight = m_nWindowHeight;

      if (eEye == Eye_Left) {
        *pnX = 0;
      } else {
        *pnX = m_nWindowWidth / 2;
      }
    }

    virtual void GetProjectionRaw(EVREye eEye, float *pfLeft, float *pfRight,
                                  float *pfTop, float *pfBottom) {
      *pfLeft = -1.0;
      *pfRight = 1.0;
      *pfTop = -1.0;
      *pfBottom = 1.0;
    }

    virtual DistortionCoordinates_t ComputeDistortion(EVREye eEye, float fU,
                                                      float fV) {
      DistortionCoordinates_t coordinates;

      // Distortion for lens implementation from
      // https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc
      float hX;
      float hY;
      double rr;
      double r2;
      double theta;

      rr = sqrt((fU - 0.5f) * (fU - 0.5f) + (fV - 0.5f) * (fV - 0.5f));
      r2 = rr * (1 + m_fDistortionK1 * (rr * rr) +
                 m_fDistortionK2 * (rr * rr * rr * rr));
      theta = atan2(fU - 0.5f, fV - 0.5f);
      hX = float(sin(theta) * r2) * m_fZoomWidth;
      hY = float(cos(theta) * r2) * m_fZoomHeight;

      coordinates.rfBlue[0] = hX + 0.5f;
      coordinates.rfBlue[1] = hY + 0.5f;
      coordinates.rfGreen[0] = hX + 0.5f;
      coordinates.rfGreen[1] = hY + 0.5f;
      coordinates.rfRed[0] = hX + 0.5f;
      coordinates.rfRed[1] = hY + 0.5f;
      // coordinates.rfBlue[0] = fU;
      // coordinates.rfBlue[1] = fV;
      // coordinates.rfGreen[0] = fU;
      // coordinates.rfGreen[1] = fV;
      // coordinates.rfRed[0] = fU;
      // coordinates.rfRed[1] = fV;

      return coordinates;
    }

  private:
  };
}

#endif // HOBOVR_COMPONENTS_H