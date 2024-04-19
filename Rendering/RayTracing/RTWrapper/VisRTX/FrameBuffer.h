// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "../Types.h"

#include "vtkLogger.h"

#include <VisRTX.h>
#include <cassert>

namespace RTW
{
VTK_ABI_NAMESPACE_BEGIN
    class FrameBuffer : public Object
    {
        friend class Renderer;

    public:
        FrameBuffer(const rtw::vec2i &size, const RTWFrameBufferFormat format, const uint32_t frameBufferChannels)
            : Object(RTW_FRAMEBUFFER)
        {
            VisRTX::Context* rtx = VisRTX_GetContext();

            if (format == RTW_FB_RGBA8)
                this->frameBuffer = rtx->CreateFrameBuffer(VisRTX::FrameBufferFormat::RGBA8, VisRTX::Vec2ui(size.x, size.y));
            else if (format == RTW_FB_RGBA32F)
                this->frameBuffer = rtx->CreateFrameBuffer(VisRTX::FrameBufferFormat::RGBA32F, VisRTX::Vec2ui(size.x, size.y));
            else
                assert(false);

            this->format = format;
            this->channels = frameBufferChannels;
        }

        ~FrameBuffer()
        {
            this->frameBuffer->Release();
        }

        void Commit() override
        {
        }

        void Clear()
        {
            this->frameBuffer->Clear();
        }

        const void* Map(const RTWFrameBufferChannel channel)
        {
            if (channel == RTW_FB_COLOR)
                return this->frameBuffer->MapColorBuffer();
            if (channel == RTW_FB_DEPTH)
                return this->frameBuffer->MapDepthBuffer();

            assert(false);
            return nullptr;
        }

        void Unmap(const void *mapped)
        {
            this->frameBuffer->Unmap(mapped);
        }

        void SetDepthNormalizationGL(float clipMin, float clipMax)
        {
            this->frameBuffer->SetDepthNormalization(clipMin, clipMax);
        }

        int GetColorTextureGL()
        {
            try
            {
                return this->frameBuffer->GetColorTextureGL();
            }
            catch(const VisRTX::Exception& e)
            {
                vtkLogF(ERROR, "VISRTX Error: Could not get color texture.");
                return 0;
            }
        }

        int GetDepthTextureGL()
        {
            try
            {
                return this->frameBuffer->GetDepthTextureGL();
            }
            catch(const VisRTX::Exception& e)
            {
                vtkLogF(ERROR, "VISRTX Error: Could not get depth texture.");
                return 0;
            }
        }

    private:
        VisRTX::FrameBuffer* frameBuffer = nullptr;
        RTWFrameBufferFormat format;
        uint32_t channels;
    };
VTK_ABI_NAMESPACE_END
}
