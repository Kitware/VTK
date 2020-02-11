#include "VisRTXBackend.h"

#define VISRTX_DYNLOAD
#include <VisRTX.h>

#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "Data.h"
#include "FrameBuffer.h"
#include "Geometry.h"
#include "Light.h"
#include "Material.h"
#include "Model.h"
#include "Object.h"
#include "Renderer.h"
#include "Texture.h"


namespace RTW
{
    RTWError VisRTXBackend::Init()
    {
#ifdef VISRTX_DYNLOAD
        // Load library first
        if (!VisRTX_LoadLibrary())
        {
            // std::cerr << "Error: Failed to load VisRTX library" << std::endl;
            return RTW_UNKNOWN_ERROR;
        }
#endif

        VisRTX::Context* rtx = VisRTX_GetContext();

        if (!rtx || rtx->GetDeviceCount() <= 0)
        {
            // std::cerr << "Error: Unsupported device" << std::endl;
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

    RTWData VisRTXBackend::NewData(size_t numElements, RTWDataType dataType, const void *source, const uint32_t dataCreationFlags)
    {
        return reinterpret_cast<RTWData>(new Data(numElements, dataType, source, dataCreationFlags));
    }

    RTWGeometry VisRTXBackend::NewGeometry(const char *type)
    {
        return reinterpret_cast<RTWGeometry>(new Geometry(type));
    }

    RTWTexture VisRTXBackend::NewTexture(const char* type)
    {
        return reinterpret_cast<RTWTexture>(new Texture(type));
    }

    RTWLight VisRTXBackend::NewLight(RTWRenderer, const char *type)
    {
        return reinterpret_cast<RTWLight>(new Light(type));
    }

    RTWMaterial VisRTXBackend::NewMaterial(RTWRenderer, const char *type)
    {
        return reinterpret_cast<RTWMaterial>(new Material(type));
    }

    RTWRenderer VisRTXBackend::NewRenderer(const char *type)
    {
        return reinterpret_cast<RTWRenderer>(new Renderer(type));
    }

    RTWCamera VisRTXBackend::NewCamera(const char *type)
    {
        return reinterpret_cast<RTWCamera>(new Camera(type));
    }

    RTWModel VisRTXBackend::NewModel()
    {
        return reinterpret_cast<RTWModel>(new Model());
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

    void VisRTXBackend::AddGeometry(RTWModel model, RTWGeometry geometry)
    {
        if (!model || !geometry)
            return;

        reinterpret_cast<Model*>(model)->AddGeometry(reinterpret_cast<Geometry*>(geometry));
    }

    void VisRTXBackend::SetString(RTWObject object, const char *id, const char *s)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetString(id, s);
    }

    void VisRTXBackend::SetObject(RTWObject object, const char *id, RTWObject other)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetObject(id, reinterpret_cast<Object*>(other));
    }

    void VisRTXBackend::SetData(RTWObject object, const char *id, RTWData data)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->SetObject<Data>(id, reinterpret_cast<Data*>(data));
    }

    void VisRTXBackend::SetMaterial(RTWGeometry geometry, RTWMaterial material)
    {
        if (!geometry)
            return;

        reinterpret_cast<Geometry*>(geometry)->SetMaterial(reinterpret_cast<Material*>(material));
    }

    void VisRTXBackend::Set1i(RTWObject object, const char *id, int32_t x)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Set1i(id, x);
    }

    void VisRTXBackend::Set1f(RTWObject object, const char *id, float x)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Set1f(id, x);
    }

    void VisRTXBackend::Set2f(RTWObject object, const char *id, float x, float y)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Set2f(id, x, y);
    }

    void VisRTXBackend::Set2i(RTWObject object, const char *id, int x, int y)
    {
      if (!object)
        return;

      reinterpret_cast<Object*>(object)->Set2i(id, x, y);
    }

    void VisRTXBackend::Set3i(RTWObject object, const char *id, int x, int y, int z)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Set3i(id, x, y, z);
    }

    void VisRTXBackend::Set3f(RTWObject object, const char *id, float x, float y, float z)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Set3f(id, x, y, z);
    }

    void VisRTXBackend::Set4f(RTWObject object, const char *id, float x, float y, float z, float w)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Set4f(id, x, y, z, w);
    }

    void VisRTXBackend::RemoveParam(RTWObject object, const char *id)
    {
        if (object)
        {
            reinterpret_cast<Object*>(object)->RemoveParam(id);
        }
    }

    void VisRTXBackend::Commit(RTWObject object)
    {
        if (!object)
            return;

        reinterpret_cast<Object*>(object)->Commit();
    }

    float VisRTXBackend::RenderFrame(RTWFrameBuffer frameBuffer, RTWRenderer renderer, const uint32_t frameBufferChannels)
    {
        if (!renderer)
            return 0.0f;

        return reinterpret_cast<Renderer*>(renderer)->RenderFrame(reinterpret_cast<FrameBuffer*>(frameBuffer), frameBufferChannels);
    }

    void VisRTXBackend::FrameBufferClear(RTWFrameBuffer frameBuffer, const uint32_t frameBufferChannels)
    {
        if (!frameBuffer)
            return;

        reinterpret_cast<FrameBuffer*>(frameBuffer)->Clear(frameBufferChannels);
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
}
