#pragma once

#include "Types.h"

namespace RTW
{
    class Backend
    {
    public:
        virtual ~Backend() = default;

    public:
        virtual RTWError Init() = 0;
        virtual void Shutdown() = 0;

        virtual bool IsSupported(RTWFeature feature) const = 0;

        virtual RTWData NewData(size_t numItems, RTWDataType, const void *source, const uint32_t dataCreationFlags = 0) = 0;
        virtual RTWGeometry NewGeometry(const char *type) = 0;
        virtual RTWTexture NewTexture(const char* type) = 0;
        virtual RTWLight NewLight3(const char *light_type) = 0;
        virtual RTWMaterial NewMaterial2(const char *renderer_type, const char *material_type) = 0;
        virtual RTWVolume NewVolume(const char *type) = 0;
        virtual RTWTransferFunction NewTransferFunction(const char *type) = 0;
        virtual RTWRenderer NewRenderer(const char *type) = 0;
        virtual RTWCamera NewCamera(const char *type) = 0;
        virtual RTWModel NewModel() = 0;
        virtual RTWGeometry NewInstance(RTWModel modelToInstantiate, const rtw::affine3f &transform) = 0;
        virtual RTWFrameBuffer NewFrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels) = 0;

        virtual void Release(RTWObject) = 0;

        virtual void AddGeometry(RTWModel, RTWGeometry) = 0;
        virtual void AddVolume(RTWModel, RTWVolume) = 0;

        virtual void SetString(RTWObject, const char *id, const char *s) = 0;
        virtual void SetObject(RTWObject, const char *id, RTWObject other) = 0;
        virtual void SetData(RTWObject, const char *id, RTWData) = 0;
        virtual void SetMaterial(RTWGeometry, RTWMaterial) = 0;
        virtual void Set1i(RTWObject, const char *id, int32_t x) = 0;
        virtual void Set2i(RTWObject, const char *id, int32_t x, int32_t y) = 0;
        virtual void Set1f(RTWObject, const char *id, float x) = 0;
        virtual void Set2f(RTWObject, const char *id, float x, float y) = 0;
        virtual void Set3i(RTWObject, const char *id, int x, int y, int z) = 0;
        virtual void Set3f(RTWObject, const char *id, float x, float y, float z) = 0;
        virtual void Set4f(RTWObject, const char *id, float x, float y, float z, float w) = 0;

        virtual void RemoveParam(RTWObject, const char *id) = 0;

        virtual RTWError SetRegion(RTWVolume, void *source, const rtw::vec3i &regionCoords, const rtw::vec3i &regionSize) = 0;

        virtual void Commit(RTWObject) = 0;

        virtual float RenderFrame(RTWFrameBuffer, RTWRenderer, const uint32_t frameBufferChannels) = 0;

        virtual void FrameBufferClear(RTWFrameBuffer, const uint32_t frameBufferChannels) = 0;
        virtual const void* MapFrameBuffer(RTWFrameBuffer, const RTWFrameBufferChannel) = 0;
        virtual void UnmapFrameBuffer(const void *mapped, RTWFrameBuffer) = 0;

        virtual void SetDepthNormalizationGL(RTWFrameBuffer frameBuffer, float clipMin, float clipMax) = 0;
        virtual int GetColorTextureGL(RTWFrameBuffer frameBuffer) = 0;
        virtual int GetDepthTextureGL(RTWFrameBuffer frameBuffer) = 0;

        // Convenience functions (TODO remove)
        inline void Setf(RTWObject object, const char *id, float x)
        {
          Set1f(object, id, x);
        }

        inline void Set3fv(RTWObject object, const char *id, const float *xyz)
        {
          Set3f(object, id, xyz[0], xyz[1], xyz[2]);
        }

        inline void SetVec2f(RTWObject object, const char *id, const rtw::vec2f &v)
        {
          Set2f(object, id, v.x, v.y);
        }

    };
}
