#pragma once

#include "../Types.h"

#include <VisRTX.h>
#include <algorithm>
#include <cassert>

namespace RTW
{
    class Texture : public Object
    {
        friend class Renderer;
        friend class Material;
        friend class Light;

    private:
        VisRTX::TextureFormat convert(RTWTextureFormat format) const
        {
            switch (format)
            {
                case RTW_TEXTURE_SRGBA: // no support for sRGB textures in VisRTX
                case RTW_TEXTURE_RGBA8:
                    return VisRTX::TextureFormat::RGBA8;
                case RTW_TEXTURE_SRGB: // no support for sRGB textures in VisRTX
                case RTW_TEXTURE_RGB8:
                    return VisRTX::TextureFormat::RGB8;
                case RTW_TEXTURE_RGBA32F:
                    return VisRTX::TextureFormat::RGBA32F;
                case RTW_TEXTURE_RGB32F:
                    return VisRTX::TextureFormat::RGB32F;
                case RTW_TEXTURE_R8:
                    return VisRTX::TextureFormat::R8;
                case RTW_TEXTURE_R32F:
                    return VisRTX::TextureFormat::R32F;
                default:
                    break;
            }

            assert(false);
            return VisRTX::TextureFormat::RGBA8;
        }

    public:
        Texture(const char* /*type*/)
            : Object(RTW_TEXTURE)
        {
        }

        ~Texture()
        {
            if (this->texture)
                this->texture->Release();
        }

        void Commit() override
        {
            int type = this->GetInt({ "format" });
            int flags = this->GetInt({ "filter" });

            void* source = nullptr;
            Data* data = this->GetObject<Data>({ "data" });
            VisRTX::Vec2i size = VisRTX::Vec2i(data->GetWidth(), data->GetHeight());
            if (data)
                source = data->GetData();

            VisRTX::Context* rtx = VisRTX_GetContext();

            if (!this->texture)
                this->texture = rtx->CreateTexture(VisRTX::Vec2ui(size.x, size.y), convert((RTWTextureFormat)type), source);
            else
                this->texture->SetPixels(VisRTX::Vec2ui(size.x, size.y), convert((RTWTextureFormat)type), source);

            if (flags & RTW_TEXTURE_FILTER_NEAREST)
                this->texture->SetFiltering(VisRTX::TextureFiltering::NEAREST, VisRTX::TextureFiltering::NEAREST);
        }


    private:
        VisRTX::Texture* texture = nullptr;
    };
}
