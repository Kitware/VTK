#pragma once

#include <ospray/ospray.h>

#include "../Backend.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <vector>

namespace RTW
{

  OSPFrameBufferFormat convert(RTWFrameBufferFormat format)
  {
    switch (format)
    {
    case RTW_FB_RGBA8:
      return OSP_FB_RGBA8;
    case RTW_FB_RGBA32F:
      return OSP_FB_RGBA32F;
    default:
      return OSP_FB_NONE;
    }
  }

  OSPTextureFormat convert(RTWTextureFormat format)
  {
    switch (format)
    {
    case RTW_TEXTURE_RGBA8:
      return OSP_TEXTURE_RGBA8;
    case RTW_TEXTURE_SRGBA:
      return OSP_TEXTURE_SRGBA;
    case RTW_TEXTURE_RGBA32F:
      return OSP_TEXTURE_RGBA32F;
    case RTW_TEXTURE_RGB8:
      return OSP_TEXTURE_RGB8;
    case RTW_TEXTURE_SRGB:
      return OSP_TEXTURE_SRGB;
    case RTW_TEXTURE_RGB32F:
      return OSP_TEXTURE_RGB32F;
    case RTW_TEXTURE_R8:
      return OSP_TEXTURE_R8;
    case RTW_TEXTURE_R32F:
      return OSP_TEXTURE_R32F;
    case RTW_TEXTURE_L8:
      return OSP_TEXTURE_L8;
    case RTW_TEXTURE_RA8:
      return OSP_TEXTURE_RA8;
    case RTW_TEXTURE_LA8:
      return OSP_TEXTURE_LA8;
    case RTW_TEXTURE_FORMAT_INVALID:
    default:
      return OSP_TEXTURE_FORMAT_INVALID;
    }
  }




  /*
   * Simple pass-through backend for OSPRay.
   */
  class OSPRayBackend : public Backend
  {
  public:
    RTWError Init() override
    {
      RTWError ret = static_cast<RTWError>(ospInit(nullptr, nullptr));
      return ret;
    }

    void Shutdown() override
    {
      ospShutdown();
    }

    bool IsSupported(RTWFeature feature) const override
    {
      switch (feature)
      {
      case RTW_DEPTH_NORMALIZATION:
        return false;
      case RTW_OPENGL_INTEROP:
        return false;
      case RTW_ANIMATED_PARAMETERIZATION:
        return false;
      case RTW_INSTANCING:
        return true;
      case RTW_DENOISER:
        return false; // OpenImageDenoise is an external lib outside of the backend
      case RTW_DEPTH_COMPOSITING:
        return true;
      }
      return false;
    }

    RTWData NewData(size_t numElements, RTWDataType dataType, const void *source, const uint32_t dataCreationFlags) override
    {
      return reinterpret_cast<RTWData>(ospNewData(numElements, static_cast<OSPDataType>(dataType), source, dataCreationFlags));
    }

    RTWGeometry NewGeometry(const char *type) override
    {
      return reinterpret_cast<RTWGeometry>(ospNewGeometry(type));
    }

    RTWTexture NewTexture(const char* type) override
    {
      return reinterpret_cast<RTWTexture>(ospNewTexture(type));
    }

    RTWLight NewLight3(const char *light_type) override
    {
      return reinterpret_cast<RTWLight>(ospNewLight3(light_type));
    }

    RTWMaterial NewMaterial2(const char *renderer_type, const char *material_type) override
    {
      return reinterpret_cast<RTWMaterial>(ospNewMaterial2(renderer_type, material_type));
    }

    RTWVolume NewVolume(const char *type) override
    {
      return reinterpret_cast<RTWVolume>(ospNewVolume(type));
    }

    RTWTransferFunction NewTransferFunction(const char *type) override
    {
      return reinterpret_cast<RTWTransferFunction>(ospNewTransferFunction(type));
    }

    RTWRenderer NewRenderer(const char *type) override
    {
      return reinterpret_cast<RTWRenderer>(ospNewRenderer(type));
    }

    RTWCamera NewCamera(const char *type) override
    {
      return reinterpret_cast<RTWCamera>(ospNewCamera(type));
    }

    RTWModel NewModel() override
    {
      return reinterpret_cast<RTWModel>(ospNewModel());
    }

    RTWGeometry NewInstance(RTWModel modelToInstantiate, const rtw::affine3f &transform) override
    {
      osp::affine3f xfm;
      memcpy(&xfm, &transform, sizeof(osp::affine3f));
      return reinterpret_cast<RTWGeometry>(ospNewInstance(reinterpret_cast<OSPModel>(modelToInstantiate), xfm));
    }

    RTWFrameBuffer NewFrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels) override
    {
      return reinterpret_cast<RTWFrameBuffer>(ospNewFrameBuffer(osp::vec2i{ size.x, size.y }, convert(format), frameBufferChannels));
    }

    void Release(RTWObject object) override
    {
      ospRelease(reinterpret_cast<OSPObject>(object));
    }

    void AddGeometry(RTWModel model, RTWGeometry geometry) override
    {
      ospAddGeometry(reinterpret_cast<OSPModel>(model), reinterpret_cast<OSPGeometry>(geometry));
    }

    void AddVolume(RTWModel model, RTWVolume volume) override
    {
      ospAddVolume(reinterpret_cast<OSPModel>(model), reinterpret_cast<OSPVolume>(volume));
    }

    void SetString(RTWObject object, const char *id, const char *s) override
    {
      ospSetString(reinterpret_cast<OSPObject>(object), id, s);
    }

    void SetObject(RTWObject object, const char *id, RTWObject other) override
    {
      ospSetObject(reinterpret_cast<OSPObject>(object), id, reinterpret_cast<OSPObject>(other));
    }

    void SetData(RTWObject object, const char *id, RTWData data) override
    {
      ospSetData(reinterpret_cast<OSPObject>(object), id, reinterpret_cast<OSPData>(data));
    }

    void SetMaterial(RTWGeometry geometry, RTWMaterial material) override
    {
      ospSetMaterial(reinterpret_cast<OSPGeometry>(geometry), reinterpret_cast<OSPMaterial>(material));
    }

    void Set1i(RTWObject object, const char *id, int32_t x) override
    {
      ospSet1i(reinterpret_cast<OSPObject>(object), id, x);
    }

    void Set1f(RTWObject object, const char *id, float x) override
    {
      ospSet1f(reinterpret_cast<OSPObject>(object), id, x);
    }

    void Set2f(RTWObject object, const char *id, float x, float y) override
    {
      ospSet2f(reinterpret_cast<OSPObject>(object), id, x, y);
    }

    void Set2i(RTWObject object, const char *id, int x, int y) override
    {
      ospSet2i(reinterpret_cast<OSPObject>(object), id, x, y);
    }

    void Set3i(RTWObject object, const char *id, int x, int y, int z) override
    {
      ospSet3i(reinterpret_cast<OSPObject>(object), id, x, y, z);
    }

    void Set3f(RTWObject object, const char *id, float x, float y, float z) override
    {
      ospSet3f(reinterpret_cast<OSPObject>(object), id, x, y, z);
    }

    void Set4f(RTWObject object, const char *id, float x, float y, float z, float w) override
    {
      ospSet4f(reinterpret_cast<OSPObject>(object), id, x, y, z, w);
    }

    void RemoveParam(RTWObject object, const char *id) override
    {
      ospRemoveParam(reinterpret_cast<OSPObject>(object), id);
    }

    RTWError SetRegion(RTWVolume volume, void *source, const rtw::vec3i &regionCoords, const rtw::vec3i &regionSize) override
    {
      return static_cast<RTWError>(ospSetRegion(reinterpret_cast<OSPVolume>(volume), source,
        osp::vec3i{ regionCoords.x, regionCoords.y, regionCoords.z },
        osp::vec3i{ regionSize.x, regionSize.y, regionSize.z }));
    }

    void Commit(RTWObject object) override
    {
      ospCommit(reinterpret_cast<OSPObject>(object));
    }

    float RenderFrame(RTWFrameBuffer frameBuffer, RTWRenderer renderer, const uint32_t frameBufferChannels) override
    {
      return ospRenderFrame(reinterpret_cast<OSPFrameBuffer>(frameBuffer), reinterpret_cast<OSPRenderer>(renderer), frameBufferChannels);
    }

    void FrameBufferClear(RTWFrameBuffer frameBuffer, const uint32_t frameBufferChannels) override
    {
      ospFrameBufferClear(reinterpret_cast<OSPFrameBuffer>(frameBuffer), frameBufferChannels);
    }

    const void* MapFrameBuffer(RTWFrameBuffer frameBuffer, const RTWFrameBufferChannel channel) override
    {
      return ospMapFrameBuffer(reinterpret_cast<OSPFrameBuffer>(frameBuffer), static_cast<OSPFrameBufferChannel>(channel));
    }

    void UnmapFrameBuffer(const void *mapped, RTWFrameBuffer frameBuffer) override
    {
      ospUnmapFrameBuffer(mapped, reinterpret_cast<OSPFrameBuffer>(frameBuffer));
    }

    void SetDepthNormalizationGL(RTWFrameBuffer /*frameBuffer*/, float /*clipMin*/, float /*clipMax*/) override
    {
      // not supported
    }

    int GetColorTextureGL(RTWFrameBuffer /*frameBuffer*/) override
    {
      // not supported
      return 0;
    }

    int GetDepthTextureGL(RTWFrameBuffer /*frameBuffer*/) override
    {
      // not supported
      return 0;
    }
  };
}
