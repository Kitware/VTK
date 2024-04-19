// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "VisRTXBackend.h"

#include "vtkLogger.h"

#define VISRTX_DYNLOAD
#include <VisRTX.h>

#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "Data.h"
#include "FrameBuffer.h"
#include "Geometry.h"
#include "GeometricModel.h"
#include "Light.h"
#include "Material.h"
#include "Instance.h"
#include "Group.h"
#include "World.h"
#include "Object.h"
#include "Renderer.h"
#include "Texture.h"

namespace RTW
{
VTK_ABI_NAMESPACE_BEGIN
    RTWError VisRTXBackend::Init()
    {
#ifdef VISRTX_DYNLOAD
        // Load library first
        if (!VisRTX_LoadLibrary())
        {
            vtkLogF(TRACE, "Failed to load VisRTX library");
            return RTW_UNKNOWN_ERROR;
        }
#endif

        VisRTX::Context* rtx = VisRTX_GetContext();

        if (!rtx || rtx->GetDeviceCount() <= 0)
        {
            vtkLogF(TRACE, "VisRTX Error: Unsupported device");
            return RTW_UNSUPPORTED_DEVICE;
        }

        //uint32_t n = rtx->GetDeviceCount();
        //for (uint32_t i = 0; i < n; ++i)
        //{
        //    std::string name = rtx->GetDeviceName(i);
        //    uint64_t totalMem = rtx->GetDeviceMemoryTotal(i);
        //    uint64_t availMem = rtx->GetDeviceMemoryAvailable(i);

        //    float totalGB = totalMem * 1e-9f;
        //    float availGB = availMem * 1e-9f;

        //    std::cout << std::fixed << std::setprecision(1) << "Device " << i << ": " << name << " (Total: " << totalGB << " GB, Available: " << availGB << " GB)" << std::endl;
        //}

        // Let VisRTX choose the default device(s)

        return RTW_NO_ERROR;
    }

    void VisRTXBackend::Shutdown()
    {
        // .. nothing to do here
    }

    bool VisRTXBackend::IsSupported(RTWFeature feature) const
    {
        switch (feature)
        {
        case RTW_DEPTH_NORMALIZATION:
            return true;
        case RTW_OPENGL_INTEROP:
            return true;
        case RTW_ANIMATED_PARAMETERIZATION:
            return true;
        case RTW_DENOISER:
            return true;
        default:
            break;
        }
        return false;
    }

    RTWData VisRTXBackend::NewData(RTWDataType dataType, size_t numElements)
    {
        return reinterpret_cast<RTWData>(new Data(nullptr, dataType, numElements));
    }

    RTWGeometry VisRTXBackend::NewGeometry(const char *type)
    {
        return reinterpret_cast<RTWGeometry>(new Geometry(type));
    }

    RTWTexture VisRTXBackend::NewTexture(const char* type)
    {
        return reinterpret_cast<RTWTexture>(new Texture(type));
    }

    RTWLight VisRTXBackend::NewLight(const char *light_type)
    {
        return reinterpret_cast<RTWLight>(new Light(light_type));
    }

    RTWMaterial VisRTXBackend::NewMaterial(const char *light_type)
    {
        return reinterpret_cast<RTWMaterial>(new Material(light_type));
    }

    RTWRenderer VisRTXBackend::NewRenderer(const char *type)
    {
        return reinterpret_cast<RTWRenderer>(new Renderer(type));
    }

    RTWCamera VisRTXBackend::NewCamera(const char *type)
    {
        return reinterpret_cast<RTWCamera>(new Camera(type));
    }

    RTWWorld VisRTXBackend::NewWorld()
    {
        return reinterpret_cast<RTWWorld>(new World());
    }

    RTWInstance VisRTXBackend::NewInstance(RTWGroup group)
    {
        return reinterpret_cast<RTWInstance>(new Instance(reinterpret_cast<Group *>(group)));
    }

    RTWGroup VisRTXBackend::NewGroup()
    {
        return reinterpret_cast<RTWGroup>(new Group());
    }

    RTWGeometricModel VisRTXBackend::NewGeometricModel(RTWGeometry geometry)
    {
        return reinterpret_cast<RTWGeometricModel>(new GeometricModel(reinterpret_cast<Geometry *>(geometry)));
    }


    RTWFrameBuffer VisRTXBackend::NewFrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels)
    {
        return reinterpret_cast<RTWFrameBuffer>(new FrameBuffer(size, format, frameBufferChannels));
    }

    void VisRTXBackend::Release(RTWObject object)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Release();
    }

    void VisRTXBackend::SetString(RTWObject object, const char *id, const char *s)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetString(id, s);
    }

    void VisRTXBackend::SetBool(RTWObject object, const char *id, bool b)
    {
        if(!object)
            return;

        reinterpret_cast<Object*>(object)->SetBool(id, b);
    }

    void VisRTXBackend::SetObject(RTWObject object, const char *id, RTWObject other)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetObject(id, reinterpret_cast<Object*>(other));
    }

    void VisRTXBackend::SetObjectAsData(RTWObject target, const char *id, RTWDataType type, RTWObject obj)
    {
        if(!target)
            return;

        reinterpret_cast<Object*>(target)->SetObject(id, reinterpret_cast<Object*>(obj));
    }

    void VisRTXBackend::SetInt(RTWObject object, const char *id, int32_t x)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetInt(id, x);
    }

    void VisRTXBackend::SetUInt(RTWObject object, const char *id, uint32_t x)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetInt(id, static_cast<int>(x));
    }

    void VisRTXBackend::SetFloat(RTWObject object, const char *id, float x)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetFloat(id, x);
    }

    void VisRTXBackend::SetVec2f(RTWObject object, const char *id, float x, float y)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetVec2f(id, x, y);
    }

    void VisRTXBackend::SetVec2i(RTWObject object, const char *id, int x, int y)
    {
      if (!object)
        return;

      reinterpret_cast<Object*>(object)->SetVec2i(id, x, y);
    }

    void VisRTXBackend::SetVec3i(RTWObject object, const char *id, int x, int y, int z)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetVec3i(id, x, y, z);
    }

    void VisRTXBackend::SetVec3f(RTWObject object, const char *id, float x, float y, float z)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetVec3f(id, x, y, z);
    }

    void VisRTXBackend::SetVec4f(RTWObject object, const char *id, float x, float y, float z, float w)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetVec4f(id, x, y, z, w);
    }

    void VisRTXBackend::SetBox1f(RTWObject object, const char *id, float x, float y)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetVec2f(id, x, y);
    }

    void VisRTXBackend::SetLinear2f(RTWObject object, const char *id, float x, float y, float z, float w)
    {
        if (!object)
            return;
        reinterpret_cast<Object*>(object)->SetVec4f(id, x, y, z, w);
    }

    void VisRTXBackend::RemoveParam(RTWObject object, const char *id)
    {
        if (object)
        {
            reinterpret_cast<Object*>(object)->RemoveParam(id);
        }
    }

    RTWData VisRTXBackend::NewSharedData1D(const void *source, RTWDataType type, uint32_t numElements)
    {
        return reinterpret_cast<RTWData>(new Data(source, type, numElements, true));
    }

    RTWData VisRTXBackend::NewSharedData2D(const void *source, RTWDataType type, uint32_t numElements1, uint32_t numElements2)
    {
        return reinterpret_cast<RTWData>(new Data(source, type, numElements1, numElements2, true));
    }

    RTWData VisRTXBackend::NewSharedData3D(const void *source, RTWDataType type, uint32_t numElements1, uint32_t numElements2, uint32_t numElements3)
    {
        return reinterpret_cast<RTWData>(new Data(source, type, numElements1, numElements2, numElements3, true));
    }

    RTWData VisRTXBackend::NewCopyData1D(const void *source, RTWDataType type, size_t numElements)
    {
        return reinterpret_cast<RTWData>(new Data(source, type, numElements, false));
    }

    RTWData VisRTXBackend::NewCopyData2D(const void *source, RTWDataType type, size_t numElements1, size_t numElements2)
    {
        return reinterpret_cast<RTWData>(new Data(source, type, numElements1, numElements2, false));
    }

    RTWData VisRTXBackend::NewCopyData3D(const void *source, RTWDataType type, size_t numElements1, size_t numElements2, size_t numElements3)
    {
        return reinterpret_cast<RTWData>(new Data(source, type, numElements1, numElements2, numElements3, false));
    }


    void VisRTXBackend::Commit(RTWObject object)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Commit();
    }

    float VisRTXBackend::RenderFrame(RTWFrameBuffer frameBuffer, RTWRenderer renderer, RTWCamera camera, RTWWorld world)
    {
        if (!renderer)
            return 0.0f;

        return reinterpret_cast<Renderer*>(renderer)->RenderFrame(
                reinterpret_cast<FrameBuffer*>(frameBuffer),
                reinterpret_cast<Camera*>(camera),
                reinterpret_cast<World*>(world));
    }

    void VisRTXBackend::FrameBufferClear(RTWFrameBuffer frameBuffer)
    {
        if (!frameBuffer)
            return;

        reinterpret_cast<FrameBuffer*>(frameBuffer)->Clear();
    }

    const void* VisRTXBackend::MapFrameBuffer(RTWFrameBuffer frameBuffer, const RTWFrameBufferChannel channel)
    {
        if (!frameBuffer)
            return nullptr;

        return reinterpret_cast<FrameBuffer*>(frameBuffer)->Map(channel);
    }

    void VisRTXBackend::UnmapFrameBuffer(const void *mapped, RTWFrameBuffer frameBuffer)
    {
        if (!frameBuffer)
            return;

        reinterpret_cast<FrameBuffer*>(frameBuffer)->Unmap(mapped);
    }

    void VisRTXBackend::SetDepthNormalizationGL(RTWFrameBuffer frameBuffer, float clipMin, float clipMax)
    {
        if (!frameBuffer)
            return;

        reinterpret_cast<FrameBuffer*>(frameBuffer)->SetDepthNormalizationGL(clipMin, clipMax);
    }

    int VisRTXBackend::GetColorTextureGL(RTWFrameBuffer frameBuffer)
    {
        if (!frameBuffer)
            return 0;

        return reinterpret_cast<FrameBuffer*>(frameBuffer)->GetColorTextureGL();
    }

    int VisRTXBackend::GetDepthTextureGL(RTWFrameBuffer frameBuffer)
    {
        if (!frameBuffer)
            return 0;

        return reinterpret_cast<FrameBuffer*>(frameBuffer)->GetDepthTextureGL();
    }
VTK_ABI_NAMESPACE_END
}
