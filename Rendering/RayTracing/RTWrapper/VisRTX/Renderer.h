#pragma once

#include "../Types.h"
#include "Camera.h"
#include "Data.h"
#include "FrameBuffer.h"
#include "Light.h"
#include "Model.h"
#include "Object.h"
#include "Texture.h"

#include <VisRTX.h>
#include <limits>

namespace RTW
{
    class Renderer : public Object
    {
    public:
        Renderer(const char* /*type*/)
        {
            VisRTX::Context* rtx = VisRTX_GetContext();
            this->renderer = rtx->CreateRenderer();

            this->renderer->SetToneMapping(false);
        }

        ~Renderer()
        {
            this->renderer->Release();
        }

        void Commit() override
        {
            // Model
            Model* model = this->GetObject<Model>({ "model" });
            if (model)
                this->renderer->SetModel(model->model);

            // Camera
            Camera* camera = this->GetObject<Camera>({ "camera" });
            if (camera)
                this->renderer->SetCamera(camera->camera);

            // Lights
            for (VisRTX::Light* light : this->lastLights)
                this->renderer->RemoveLight(light);

            this->lastLights.clear();

            Data* lightData = this->GetObject<Data>({ "lights" });
            if (lightData && lightData->GetDataType() == RTW_OBJECT)
            {
                Light** lights = reinterpret_cast<Light**>(lightData->GetData());
                for (size_t i = 0; i < lightData->GetNumElements(); ++i)
                {
                    Light* lightHandle = lights[i];
                    if (lightHandle)
                    {
                        VisRTX::Light* light = lightHandle->light;
                        this->renderer->AddLight(light);
                        this->lastLights.push_back(light);
                    }
                }
            }

            // Samples per pixel
            int32_t spp;
            if (this->Get1i({ "spp" }, &spp))
                this->renderer->SetSamplesPerPixel(spp);

            // Epsilon
            float epsilon;
            if (this->Get1f({ "epsilon" }, &epsilon))
                this->renderer->SetEpsilon(epsilon);

            // Max ray recursion depth
            int32_t minBounces = this->Get1i({ "rouletteDepth" }, 5);
            int32_t maxBounces = this->Get1i({ "maxDepth" }, 10);
            this->renderer->SetNumBounces(minBounces, maxBounces);

            // Denoiser
            int denoise = this->Get1i({ "denoise" });
            this->renderer->SetDenoiser(denoise > 0 ? VisRTX::DenoiserType::AI : VisRTX::DenoiserType::NONE);
        }

        float RenderFrame(FrameBuffer* frameBuffer, const uint32_t /*frameBufferChannels*/)
        {
            if (!frameBuffer)
                return 0.0f;

            try
            {
                this->renderer->Render(frameBuffer->frameBuffer);
            }
            catch (VisRTX::Exception& e)
            {
                std::cerr << "VisRTX internal error: " << e.what() << std::endl;
            }

            // VisRTX does not use a variance buffer
            return std::numeric_limits<float>::infinity();
        }

    private:
        VisRTX::Renderer* renderer = nullptr;

        std::vector<VisRTX::Light*> lastLights;
    };
}
