// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "vtkLogger.h"

#include "../Backend.h"
#include <iostream>

namespace RTW
{
VTK_ABI_NAMESPACE_BEGIN
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

        RTWData NewData(RTWDataType dataType, size_t numElements) override;
        RTWData NewCopyData1D(const void *source, RTWDataType, size_t numElements) override;
        RTWData NewCopyData2D(const void *source, RTWDataType, size_t numElements1, size_t numElements2) override;
        RTWData NewCopyData3D(const void *source, RTWDataType, size_t numElements1, size_t numElements2, size_t numElements3) override;
        RTWData NewSharedData1D(const void *source, RTWDataType, uint32_t numElements) override;
        RTWData NewSharedData2D(const void *source, RTWDataType, uint32_t numElements1, uint32_t numElements2) override;
        RTWData NewSharedData3D(const void *source, RTWDataType, uint32_t numElements1, uint32_t numElements2, uint32_t numElements3) override;
        RTWGeometry NewGeometry(const char *type) override;
        RTWGroup NewInstance(RTWGroup group) override;
        RTWGroup NewGroup() override;
        RTWTexture NewTexture(const char* type) override;
        RTWLight NewLight(const char *light_type);
        RTWMaterial NewMaterial(const char *material_type);
        RTWRenderer NewRenderer(const char *type) override;
        RTWCamera NewCamera(const char *type) override;
        RTWWorld NewWorld() override;
        RTWGeometricModel NewGeometricModel(RTWGeometry geometry) override;
        RTWFrameBuffer NewFrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels) override;
        void Release(RTWObject object) override;

        void SetString(RTWObject, const char *id, const char *s) override;
        void SetObject(RTWObject, const char *id, RTWObject other) override;
        void SetObjectAsData(RTWObject target, const char *id, RTWDataType type, RTWObject obj) override;
        void SetBool(RTWObject object, const char *id, bool x) override;
        void SetInt(RTWObject object, const char *id, int32_t x) override;
        void SetUInt(RTWObject object, const char *id, uint32_t x) override;
        void SetVec2i(RTWObject object, const char *id, int32_t x, int32_t y) override;
        void SetFloat(RTWObject object, const char *id, float x) override;
        void SetVec2f(RTWObject object, const char *id, float x, float y) override;
        void SetVec3i(RTWObject object, const char *id, int x, int y, int z) override;
        void SetVec3f(RTWObject object, const char *id, float x, float y, float z) override;
        void SetVec4f(RTWObject object, const char *id, float x, float y, float z, float w) override;
        void SetBox1f(RTWObject object, const char *id, float x, float y) override;
        void SetLinear2f(RTWObject object, const char *id, float x, float y, float z, float w) override;

        void RemoveParam(RTWObject object, const char *id) override;

        void Commit(RTWObject object) override;

        float RenderFrame(RTWFrameBuffer frameBuffer, RTWRenderer renderer, RTWCamera camera, RTWWorld world) override;

        void FrameBufferClear(RTWFrameBuffer frameBuffer) override;
        const void* MapFrameBuffer(RTWFrameBuffer frameBuffer, const RTWFrameBufferChannel channel) override;
        void UnmapFrameBuffer(const void *mapped, RTWFrameBuffer frameBuffer) override;

        void SetDepthNormalizationGL(RTWFrameBuffer frameBuffer, float clipMin, float clipMax) override;
        int GetColorTextureGL(RTWFrameBuffer frameBuffer) override;
        int GetDepthTextureGL(RTWFrameBuffer frameBuffer) override;

    public:
        /*
         * Unsupported or redundant calls
         */

        void SetParam(RTWObject target, const char *id, RTWDataType type, const void *mem) override
        {
            vtkLogF(ERROR, "Error: The VisRTX backend does not currently support the SetParam call");
        }

        RTWVolume NewVolume(const char *type) override
        {
            vtkLogF(ERROR, "Error: The VisRTX backend does not currently support volumetric objects");
            return nullptr;
        }

        RTWVolume NewVolumetricModel(RTWVolume volume) override
        {
            vtkLogF(ERROR, "Error: The VisRTX backend does not currently support volumetric models");
            return nullptr;
        }

        RTWTransferFunction NewTransferFunction(const char *type) override
        {
            vtkLogF(ERROR, "Error: The VisRTX backend does not currently support volumetric transfer functions");
            return nullptr;
        }

    };
VTK_ABI_NAMESPACE_END
}
