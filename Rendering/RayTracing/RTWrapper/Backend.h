// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "Types.h"

namespace RTW
{
VTK_ABI_NAMESPACE_BEGIN
    class Backend
    {
    public:
        virtual ~Backend() = default;

    public:
        virtual RTWError Init() = 0;
        virtual void Shutdown() = 0;

        virtual bool IsSupported(RTWFeature feature) const = 0;

        virtual RTWData NewData(RTWDataType, size_t numItems) = 0;
        virtual RTWData NewCopyData1D(const void *source, RTWDataType, size_t numItems) = 0;
        virtual RTWData NewCopyData2D(const void *source, RTWDataType, size_t numItems1, size_t numItems2) = 0;
        virtual RTWData NewCopyData3D(const void *source, RTWDataType, size_t numItems1, size_t numItems2, size_t numItems3) = 0;
        virtual RTWData NewSharedData1D(const void *source, RTWDataType, uint32_t numItems) = 0;
        virtual RTWData NewSharedData2D(const void *source, RTWDataType, uint32_t numItems1, uint32_t numItems2) = 0;
        virtual RTWData NewSharedData3D(const void *source, RTWDataType, uint32_t numItems1, uint32_t numItems2,
          uint32_t numItems3) = 0;
        virtual RTWGeometry NewGeometry(const char *type) = 0;
        virtual RTWGroup NewGroup() = 0;
        virtual RTWTexture NewTexture(const char* type) = 0;
        virtual RTWLight NewLight(const char *light_type) = 0;
        virtual RTWMaterial NewMaterial(const char *material_type) = 0;
        virtual RTWVolume NewVolume(const char *type) = 0;
        virtual RTWVolumetricModel NewVolumetricModel(RTWVolume volume) = 0;
        virtual RTWTransferFunction NewTransferFunction(const char *type) = 0;
        virtual RTWRenderer NewRenderer(const char *type) = 0;
        virtual RTWCamera NewCamera(const char *type) = 0;
        virtual RTWWorld NewWorld() = 0;
        virtual RTWGeometricModel NewGeometricModel(RTWGeometry geometry) = 0;
        virtual RTWInstance NewInstance(RTWGroup geometry) = 0;
        virtual RTWFrameBuffer NewFrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels) = 0;

        virtual void Release(RTWObject) = 0;


        virtual void SetString(RTWObject, const char *id, const char *s) = 0;
        virtual void SetObject(RTWObject, const char *id, RTWObject other) = 0;
        virtual void SetObjectAsData(RTWObject target, const char *id, RTWDataType type, RTWObject obj) = 0;
        virtual void SetParam(RTWObject, const char *id, RTWDataType type, const void* mem) = 0;
        virtual void SetBool(RTWObject, const char *id, bool x) = 0;
        virtual void SetBox1f(RTWObject, const char *id, float x, float y) = 0;
        virtual void SetInt(RTWObject, const char *id, int32_t x) = 0;
        virtual void SetUInt(RTWObject, const char *id, uint32_t x) = 0;
        virtual void SetVec2i(RTWObject, const char *id, int32_t x, int32_t y) = 0;
        virtual void SetFloat(RTWObject, const char *id, float x) = 0;
        virtual void SetVec2f(RTWObject, const char *id, float x, float y) = 0;
        virtual void SetVec3i(RTWObject, const char *id, int x, int y, int z) = 0;
        virtual void SetVec3f(RTWObject, const char *id, float x, float y, float z) = 0;
        virtual void SetVec4f(RTWObject, const char *id, float x, float y, float z, float w) = 0;
        virtual void SetLinear2f(RTWObject, const char *id, float x, float y, float z, float w) = 0;

        virtual void RemoveParam(RTWObject, const char *id) = 0;

        virtual void Commit(RTWObject) = 0;

        virtual float RenderFrame(RTWFrameBuffer, RTWRenderer, RTWCamera, RTWWorld) = 0;

        virtual void FrameBufferClear(RTWFrameBuffer) = 0;
        virtual const void* MapFrameBuffer(RTWFrameBuffer, const RTWFrameBufferChannel) = 0;
        virtual void UnmapFrameBuffer(const void *mapped, RTWFrameBuffer) = 0;

        virtual void SetDepthNormalizationGL(RTWFrameBuffer frameBuffer, float clipMin, float clipMax) = 0;
        virtual int GetColorTextureGL(RTWFrameBuffer frameBuffer) = 0;
        virtual int GetDepthTextureGL(RTWFrameBuffer frameBuffer) = 0;
    };
VTK_ABI_NAMESPACE_END
}
