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

        RTWError Init() override;
        void Shutdown() override;

        bool IsSupported(RTWFeature feature) const override;

        RTWData NewData(size_t numElements, RTWDataType dataType, const void *source, const uint32_t dataCreationFlags) override;
        RTWGeometry NewGeometry(const char *type) override;
        RTWTexture NewTexture(const char* type) override;
        RTWLight NewLight(RTWRenderer renderer, const char *type);
        RTWMaterial NewMaterial(RTWRenderer renderer, const char *material_type);
        RTWRenderer NewRenderer(const char *type) override;
        RTWCamera NewCamera(const char *type) override;
        RTWModel NewModel() override;
        RTWFrameBuffer NewFrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels) override;
        void Release(RTWObject object) override;

        void AddGeometry(RTWModel model, RTWGeometry geometry) override;

        void SetString(RTWObject object, const char *id, const char *s) override;
        void SetObject(RTWObject object, const char *id, RTWObject other) override;
        void SetData(RTWObject object, const char *id, RTWData data) override;
        void SetMaterial(RTWGeometry geometry, RTWMaterial material) override;
        void Set1i(RTWObject object, const char *id, int32_t x) override;
        void Set1f(RTWObject object, const char *id, float x) override;
        void Set2f(RTWObject object, const char *id, float x, float y) override;
        void Set2i(RTWObject object, const char *id, int x, int y) override;
        void Set3i(RTWObject object, const char *id, int x, int y, int z) override;
        void Set3f(RTWObject object, const char *id, float x, float y, float z) override;
        void Set4f(RTWObject object, const char *id, float x, float y, float z, float w) override;

        void RemoveParam(RTWObject object, const char *id) override;

        void Commit(RTWObject object) override;

        float RenderFrame(RTWFrameBuffer frameBuffer, RTWRenderer renderer, const uint32_t frameBufferChannels) override;

        void FrameBufferClear(RTWFrameBuffer frameBuffer, const uint32_t frameBufferChannels) override;
        const void* MapFrameBuffer(RTWFrameBuffer frameBuffer, const RTWFrameBufferChannel channel) override;
        void UnmapFrameBuffer(const void *mapped, RTWFrameBuffer frameBuffer) override;

        void SetDepthNormalizationGL(RTWFrameBuffer frameBuffer, float clipMin, float clipMax) override;
        int GetColorTextureGL(RTWFrameBuffer frameBuffer) override;
        int GetDepthTextureGL(RTWFrameBuffer frameBuffer) override;

    public:
        /*
         * Unsupported or redundant calls
         */
        RTWLight NewLight3(const char *light_type) override
        {
          return this->NewLight(nullptr, light_type);
        }

        RTWMaterial NewMaterial2(const char *renderer_type, const char *material_type) override
        {
            return this->NewMaterial(nullptr, material_type);
        }

        RTWGeometry NewInstance(RTWModel modelToInstantiate, const rtw::affine3f &transform) override
        {
          return nullptr;
        }

        RTWVolume NewVolume(const char *type) override
        {
            return nullptr;
        }

        RTWTransferFunction NewTransferFunction(const char *type) override
        {
            return nullptr;
        }

        void AddVolume(RTWModel model, RTWVolume volume) override
        {
        }

        RTWError SetRegion(RTWVolume volume, void *source, const rtw::vec3i &regionCoords, const rtw::vec3i &regionSize) override
        {
            return RTW_NO_ERROR;
        }
    };
}
