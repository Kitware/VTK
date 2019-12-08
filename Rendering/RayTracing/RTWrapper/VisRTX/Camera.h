#pragma once

#include "../Types.h"

#include "Object.h"

#include <VisRTX.h>
#include <cassert>
#include <string>

namespace RTW
{
    class Camera : public Object
    {
        friend class Renderer;

    public:
        Camera(const std::string& type)
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
            if (this->Get3f({ "pos" }, &pos))
                this->camera->SetPosition(pos);

            VisRTX::Vec3f dir;
            if (this->Get3f({ "dir" }, &dir))
                this->camera->SetDirection(dir);

            VisRTX::Vec3f up;
            if (this->Get3f({ "up" }, &up))
                this->camera->SetUp(up);

            VisRTX::Vec2f imageBegin, imageEnd;
            if (this->Get2f({ "imageStart" }, &imageBegin) && this->Get2f({ "imageEnd" }, &imageEnd))
                this->camera->SetImageRegion(imageBegin, imageEnd);

            if (this->camera->GetType() == VisRTX::CameraType::PERSPECTIVE)
            {
                VisRTX::PerspectiveCamera* pc = dynamic_cast<VisRTX::PerspectiveCamera*>(this->camera);

                float fovy;
                if (this->Get1f({ "fovy" }, &fovy))
                    pc->SetFovY(fovy);

                float aspect;
                if (this->Get1f({ "aspect" }, &aspect))
                    pc->SetAspect(aspect);

                float focalDistance;
                if (this->Get1f({ "focusDistance" }, &focalDistance))
                    pc->SetFocalDistance(focalDistance);

                float apertureRadius;
                if (this->Get1f({ "apertureRadius" }, &apertureRadius))
                    pc->SetApertureRadius(apertureRadius);
            }

            else if (this->camera->GetType() == VisRTX::CameraType::ORTHOGRAPHIC)
            {
                VisRTX::OrthographicCamera* oc = dynamic_cast<VisRTX::OrthographicCamera*>(this->camera);

                float height;
                if (this->Get1f({ "height" }, &height))
                    oc->SetHeight(height);

                float aspect;
                if (this->Get1f({ "aspect" }, &aspect))
                    oc->SetAspect(aspect);
            }

            else
            {
                assert(false);
            }
        }

    private:
        VisRTX::Camera* camera = nullptr;
    };
}
