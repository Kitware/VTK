#pragma once

#include <ospray/ospray_util.h>

#include "../Backend.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <vector>
#include <iostream>

namespace RTW
{

  OSPFrameBufferFormat convert(RTWFrameBufferFormat format)
  {
    switch (format)
    {
    case RTW_FB_RGBA8:
      return OSP_FB_RGBA8;
    case RTW_FB_SRGBA:
      return OSP_FB_SRGBA;
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
      static bool once = false;
      RTWError ret = RTW_NO_ERROR;
      if (!once) {
        ret = static_cast<RTWError>(ospInit(nullptr, nullptr));
        OSPDevice device = ospGetCurrentDevice();
        if (!device)
        {
          std::runtime_error("OSPRay device could not be fetched!");
        }
#if OSPRAY_VERSION_MINOR > 1
        ospDeviceSetErrorCallback(device, [](void *, OSPError, const char *errorDetails) {
          std::cerr << "OSPRay ERROR: " << errorDetails << std::endl;
        }, nullptr);
#else
        ospDeviceSetErrorFunc(device, [](OSPError, const char *errorDetails) {
          std::cerr << "OSPRay ERROR: " << errorDetails << std::endl;
        });
#endif
        once = true;
      }
      return ret;
    }

    void Shutdown() override
    {
      //do nothing here. Since OSPRay 2
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

    RTWData NewCopyData1D(const void *source, RTWDataType dataType, size_t numElements) override
    {
      OSPData data = ospNewData1D(static_cast<OSPDataType>(dataType), numElements);
      ospCommit(data);
      OSPData shared = ospNewSharedData1D(source, static_cast<OSPDataType>(dataType), numElements);
      ospCommit(shared);
      ospCopyData1D(shared, data, 0);
      ospCommit(data);
      ospRelease(shared);
      return reinterpret_cast<RTWData>(data);
    }
    RTWData NewCopyData2D(const void *source, RTWDataType dataType, size_t numElements, size_t numElements2) override
    {
      OSPData data = ospNewData2D(static_cast<OSPDataType>(dataType), numElements, numElements2);
      ospCommit(data);
      OSPData shared = ospNewSharedData2D(source, static_cast<OSPDataType>(dataType), numElements, numElements2);
      ospCommit(shared);
      ospCopyData2D(shared, data, 0, 0);
      ospCommit(data);
      ospRelease(shared);
      return reinterpret_cast<RTWData>(data);
    }
    RTWData NewCopyData3D(const void *source, RTWDataType dataType, size_t numElements, size_t numElements2, size_t numElements3) override
    {
      OSPData data = ospNewData(static_cast<OSPDataType>(dataType), numElements, numElements2, numElements3);
      ospCommit(data);
      OSPData shared = ospNewSharedData3D(source, static_cast<OSPDataType>(dataType), numElements, numElements2, numElements3);
      ospCommit(shared);
      ospCopyData(shared, data, 0, 0, 0);
      ospCommit(data);
      ospRelease(shared);
      return reinterpret_cast<RTWData>(data);
    }

    RTWData NewData(RTWDataType dataType, size_t numElements) override
    {
      return reinterpret_cast<RTWData>(ospNewData(static_cast<OSPDataType>(dataType), numElements));
    }

    RTWGeometry NewGeometry(const char *type) override
    {
      return reinterpret_cast<RTWGeometry>(ospNewGeometry(type));
    }

    RTWGroup NewGroup() override
    {
      return reinterpret_cast<RTWGroup>(ospNewGroup());
    }

    RTWData NewSharedData1D(const void* sharedData, RTWDataType type, uint32_t numItems1) override
    {
      return reinterpret_cast<RTWData>(ospNewSharedData1D(sharedData, (OSPDataType)((int)type), numItems1));
    }

    RTWData NewSharedData2D(const void* sharedData, RTWDataType type, uint32_t numItems1, uint32_t numItems2) override
    {
      return reinterpret_cast<RTWData>(ospNewSharedData2D(sharedData, (OSPDataType)((int)type), numItems1, numItems2));
    }

    RTWData NewSharedData3D(const void* sharedData, RTWDataType type, uint32_t numItems1, uint32_t numItems2,
      uint32_t numItems3) override
    {
      return reinterpret_cast<RTWData>(ospNewSharedData3D(sharedData, (OSPDataType)((int)type), 
        numItems1, numItems2, numItems3));
    }

    RTWTexture NewTexture(const char* type) override
    {
      return reinterpret_cast<RTWTexture>(ospNewTexture(type));
    }

    RTWLight NewLight(const char *light_type) override
    {
      return reinterpret_cast<RTWLight>(ospNewLight(light_type));
    }

    RTWMaterial NewMaterial(const char *renderer_type, const char *material_type) override
    {
      return reinterpret_cast<RTWMaterial>(ospNewMaterial(renderer_type, material_type));
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

    RTWGeometricModel NewGeometricModel(RTWGeometry geometry) override
    {
     return reinterpret_cast<RTWGeometricModel>(ospNewGeometricModel(reinterpret_cast<OSPGeometry>(geometry)));
    }

    RTWVolumetricModel NewVolumetricModel(RTWVolume volume) override
    {
     return reinterpret_cast<RTWVolumetricModel>(ospNewVolumetricModel(reinterpret_cast<OSPVolume>(volume)));
    }

    RTWWorld NewWorld() override
    {
      return reinterpret_cast<RTWWorld>(ospNewWorld());
    }

    RTWInstance NewInstance(RTWGroup geometry) override
    {
      return reinterpret_cast<RTWInstance>(ospNewInstance(reinterpret_cast<OSPGroup>(geometry)));
    }

    RTWFrameBuffer NewFrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels) override
    {
      return reinterpret_cast<RTWFrameBuffer>(ospNewFrameBuffer(size.x, size.y, convert(format), frameBufferChannels));
    }

    void Release(RTWObject object) override
    {
      ospRelease(reinterpret_cast<OSPObject>(object));
    }

    void SetString(RTWObject object, const char *id, const char *s) override
    {
      ospSetString(reinterpret_cast<OSPObject>(object), id, s);
    }

    void SetObject(RTWObject object, const char *id, RTWObject other) override
    {
      ospSetObject(reinterpret_cast<OSPObject>(object), id, reinterpret_cast<OSPObject>(other));
    }

    void SetObjectAsData(RTWObject target, const char *id, RTWDataType type, RTWObject obj) override{
      ospSetObjectAsData(reinterpret_cast<OSPObject>(target), id, (OSPDataType)type,
       reinterpret_cast<OSPObject>(obj));
    }

    void SetParam(RTWObject object, const char *id, RTWDataType dataType, const void* mem) override
    {
      ospSetParam(reinterpret_cast<OSPObject>(object), id, static_cast<OSPDataType>(dataType),
        mem);
    }

    void SetInt(RTWObject object, const char *id, int32_t x) override
    {
      ospSetInt(reinterpret_cast<OSPObject>(object), id, x);
    }

    void SetBool(RTWObject object, const char *id, bool x) override
    {
      ospSetBool(reinterpret_cast<OSPObject>(object), id, x);
    }

    void SetFloat(RTWObject object, const char *id, float x) override
    {
      ospSetFloat(reinterpret_cast<OSPObject>(object), id, x);
    }

    void SetVec2f(RTWObject object, const char *id, float x, float y) override
    {
      ospSetVec2f(reinterpret_cast<OSPObject>(object), id, x, y);
    }

    void SetVec2i(RTWObject object, const char *id, int x, int y) override
    {
      ospSetVec2i(reinterpret_cast<OSPObject>(object), id, x, y);
    }

    void SetVec3i(RTWObject object, const char *id, int x, int y, int z) override
    {
      ospSetVec3i(reinterpret_cast<OSPObject>(object), id, x, y, z);
    }

    void SetVec3f(RTWObject object, const char *id, float x, float y, float z) override
    {
      ospSetVec3f(reinterpret_cast<OSPObject>(object), id, x, y, z);
    }

    void SetVec4f(RTWObject object, const char *id, float x, float y, float z, float w) override
    {
      ospSetVec4f(reinterpret_cast<OSPObject>(object), id, x, y, z, w);
    }

    void RemoveParam(RTWObject object, const char *id) override
    {
      ospRemoveParam(reinterpret_cast<OSPObject>(object), id);
    }

    void Commit(RTWObject object) override
    {
      ospCommit(reinterpret_cast<OSPObject>(object));
    }

    float RenderFrame(RTWFrameBuffer frameBuffer, RTWRenderer renderer, RTWCamera camera, RTWWorld world) override
    {
      return ospRenderFrameBlocking(reinterpret_cast<OSPFrameBuffer>(frameBuffer), reinterpret_cast<OSPRenderer>(renderer), 
      reinterpret_cast<OSPCamera>(camera), reinterpret_cast<OSPWorld>(world));
    }

    void FrameBufferClear(RTWFrameBuffer frameBuffer) override
    {
      ospResetAccumulation(reinterpret_cast<OSPFrameBuffer>(frameBuffer));
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
