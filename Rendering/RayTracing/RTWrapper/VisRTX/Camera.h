// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "../Types.h"

#include "Object.h"

#include <VisRTX.h>
#include <cassert>
#include <string>

namespace RTW
{
VTK_ABI_NAMESPACE_BEGIN
    class Camera : public Object
    {
        friend class Renderer;

    public:
        Camera(const std::string& type) : Object(RTW_CAMERA)
        {
            VisRTX::Context* rtx = VisRTX_GetContext();

            if (type == "perspective")
                this->camera = rtx->CreatePerspectiveCamera();
            else if (type == "orthographic")
                this->camera = rtx->CreateOrthographicCamera();
            else
                assert(false);
        }

        ~Camera()
        {
            this->camera->Release();
        }

        void Commit() override
        {
            VisRTX::Vec3f pos;
            if (this->GetVec3f({ "position" }, &pos))
            {
                this->camera->SetPosition(pos);
            }

            VisRTX::Vec3f dir;
            if (this->GetVec3f({ "direction" }, &dir))
            {
                this->camera->SetDirection(dir);
            }

            VisRTX::Vec3f up;
            if (this->GetVec3f({ "up" }, &up))
            {
                this->camera->SetUp(up);
            }

            VisRTX::Vec2f imageBegin, imageEnd;
            if (this->GetVec2f({ "imageStart" }, &imageBegin) && this->GetVec2f({ "imageEnd" }, &imageEnd))
            {
                this->camera->SetImageRegion(imageBegin, imageEnd);
            }

            if (this->camera->GetType() == VisRTX::CameraType::PERSPECTIVE)
            {
                VisRTX::PerspectiveCamera* pc = dynamic_cast<VisRTX::PerspectiveCamera*>(this->camera);

                float fovy;
                if (this->GetFloat({ "fovy" }, &fovy))
                {
                    pc->SetFovY(fovy);
                }

                float aspect;
                if (this->GetFloat({ "aspect" }, &aspect))
                {
                    pc->SetAspect(aspect);
                }

                float focalDistance;
                if (this->GetFloat({ "focusDistance" }, &focalDistance))
                {
                    pc->SetFocalDistance(focalDistance);
                }

                float apertureRadius;
                if (this->GetFloat({ "apertureRadius" }, &apertureRadius))
                {
                    pc->SetApertureRadius(apertureRadius);
                }
            }

            else if (this->camera->GetType() == VisRTX::CameraType::ORTHOGRAPHIC)
            {
                VisRTX::OrthographicCamera* oc = dynamic_cast<VisRTX::OrthographicCamera*>(this->camera);

                float height;
                if (this->GetFloat({ "height" }, &height))
                {
                    oc->SetHeight(height);
                }

                float aspect;
                if (this->GetFloat({ "aspect" }, &aspect))
                {
                    oc->SetAspect(aspect);
                }
            }

            else
            {
                assert(false);
            }
        }

    private:
        VisRTX::Camera* camera = nullptr;
    };
VTK_ABI_NAMESPACE_END
}
