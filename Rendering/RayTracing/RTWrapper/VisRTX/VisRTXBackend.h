#pragma once

#include "../Backend.h"

namespace RTW
{
    /*
     * Translates OSPRay-ish wrapper API calls to VisRTX
     */
    class VisRTXBackend : public RTW::Backend
    {
    public:
        VisRTXBackend() = default;
        ~VisRTXBackend() = default;

        RTWError Init(int *argc, const char **argv);
        void Shutdown();

        bool IsSupported(RTWFeature feature) const;

        RTWData NewData(size_t numElements, RTWDataType dataType, const void *source, const uint32_t dataCreationFlags);
        RTWGeometry NewGeometry(const char *type);
        RTWTexture NewTexture(const char* type);
        RTWLight NewLight(RTWRenderer renderer, const char *type);
        RTWMaterial NewMaterial(RTWRenderer renderer, const char *material_type);
        RTWRenderer NewRenderer(const char *type);
        RTWCamera NewCamera(const char *type);
        RTWModel NewModel();
        RTWFrameBuffer NewFrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels);
        void Release(RTWObject object);

        void AddGeometry(RTWModel model, RTWGeometry geometry);

        void SetString(RTWObject object, const char *id, const char *s);
        void SetObject(RTWObject object, const char *id, RTWObject other);
        void SetData(RTWObject object, const char *id, RTWData data);
        void SetMaterial(RTWGeometry geometry, RTWMaterial material);
        void Set1i(RTWObject object, const char *id, int32_t x);
        void Set1f(RTWObject object, const char *id, float x);
        void Set2f(RTWObject object, const char *id, float x, float y);
        void Set2i(RTWObject object, const char *id, int x, int y);
        void Set3i(RTWObject object, const char *id, int x, int y, int z);
        void Set3f(RTWObject object, const char *id, float x, float y, float z);
        void Set4f(RTWObject object, const char *id, float x, float y, float z, float w);

        void Commit(RTWObject object);

        float RenderFrame(RTWFrameBuffer frameBuffer, RTWRenderer renderer, const uint32_t frameBufferChannels);

        void FrameBufferClear(RTWFrameBuffer frameBuffer, const uint32_t frameBufferChannels);
        const void* MapFrameBuffer(RTWFrameBuffer frameBuffer, const RTWFrameBufferChannel channel);
        void UnmapFrameBuffer(const void *mapped, RTWFrameBuffer frameBuffer);

        void SetDepthNormalizationGL(RTWFrameBuffer frameBuffer, float clipMin, float clipMax);
        int GetColorTextureGL(RTWFrameBuffer frameBuffer);
        int GetDepthTextureGL(RTWFrameBuffer frameBuffer);

    public:
        /*
         * Unsupported or redundant calls
         */
        RTWLight NewLight3(const char *light_type)
        {
          return this->NewLight(nullptr, light_type);
        }

        RTWMaterial NewMaterial2(const char *renderer_type, const char *material_type)
        {
            return this->NewMaterial(nullptr, material_type);
        }

        RTWGeometry NewInstance(RTWModel modelToInstantiate, const rtw::affine3f &transform)
        {
          return nullptr;
        }

        RTWVolume NewVolume(const char *type)
        {
            return nullptr;
        }

        RTWTransferFunction NewTransferFunction(const char *type)
        {
            return nullptr;
        }

        void AddVolume(RTWModel model, RTWVolume volume)
        {
        }

        RTWError SetRegion(RTWVolume volume, void *source, const rtw::vec3i &regionCoords, const rtw::vec3i &regionSize)
        {
            return RTW_NO_ERROR;
        }
    };
}
