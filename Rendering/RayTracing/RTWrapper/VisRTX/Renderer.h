// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "vtkLogger.h"

#include "../Types.h"
#include "Camera.h"
#include "Data.h"
#include "FrameBuffer.h"
#include "Light.h"
#include "World.h"
#include "Object.h"
#include "Texture.h"

#include <VisRTX.h>
#include <limits>

namespace RTW
{
VTK_ABI_NAMESPACE_BEGIN
    class Renderer : public Object
    {
    public:
        Renderer(const char* /*type*/)
            : Object(RTW_RENDERER)
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
        }

        float RenderFrame(FrameBuffer* frameBuffer, Camera *camera, World *world)
        {
            if (!frameBuffer)
                return 0.0f;

            // Camera
            if (camera)
            {
                this->renderer->SetCamera(camera->camera);
            }

            Data *map_backplate = GetObject<Data>({"map_backplate"});
            Light bgLight("hdri");
            bgLight.SetVec3f("dir", 1.0, 0.0, 0.0);
            bgLight.SetVec3f("up", 0.0, 1.0, 0.0);
            bgLight.SetObject("map", map_backplate);
            bgLight.Commit();

            bool removeTemp = false;

            // World
            if (world)
            {
                //World
                VisRTX::Model *model = world->model;
                this->renderer->SetModel(model);

                // Lights
                for (VisRTX::Light* light : this->lastLights)
                    this->renderer->RemoveLight(light);

                this->lastLights.clear();


                Data *lightData = world->GetObject<Data>({"light"});

                if (lightData &&
                    lightData->GetDataType() == RTW_DATA &&
                    lightData->GetElementDataType() == RTW_LIGHT)
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

                if(map_backplate)
                {
                    removeTemp = true;
                    this->renderer->AddLight(bgLight.light);
                }

            }

            // Samples per pixel
            int32_t spp;
            if (this->GetInt({ "pixelSamples" }, &spp))
            {
                this->renderer->SetSamplesPerPixel(spp);
            }

            // Epsilon
            float epsilon;
            if (this->GetFloat({ "epsilon" }, &epsilon))
            {
                this->renderer->SetEpsilon(epsilon);
            }

            // Max ray recursion depth
            int32_t minBounces = this->GetInt({ "rouletteDepth" }, 5);
            int32_t maxBounces = this->GetFloat({ "maxPathLength" }, 10.0f);
            this->renderer->SetNumBounces(minBounces, maxBounces);

            // Denoiser
            int denoise = this->GetInt({ "denoise" });
            this->renderer->SetDenoiser(denoise > 0 ? VisRTX::DenoiserType::AI : VisRTX::DenoiserType::NONE);


            try
            {
                this->renderer->Render(frameBuffer->frameBuffer);
            }
            catch (VisRTX::Exception& e)
            {
                vtkLogF(ERROR, "VisRTX internal error: \"%s\"", e.what());
            }

            if(removeTemp)
                this->renderer->RemoveLight(bgLight.light);

            // VisRTX does not use a variance buffer
            return std::numeric_limits<float>::infinity();
        }

    private:
        VisRTX::Renderer* renderer = nullptr;

        std::vector<VisRTX::Light*> lastLights;
    };
VTK_ABI_NAMESPACE_END
}
