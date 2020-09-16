#pragma once

#include "vtkLogger.h"

#include "../Types.h"
#include "Object.h"

#include <VisRTX.h>
#include <cassert>
#include <string>

namespace RTW
{
    class Light : public Object
    {
        friend class Renderer;

    public:
        Light(const std::string& type)
            : Object(RTW_LIGHT)
        {
            VisRTX::Context* rtx = VisRTX_GetContext();

            if (type == "DirectionalLight" || type == "distant")
                this->light = rtx->CreateDirectionalLight();
            else if (type == "PointLight" || type == "point" || type == "SphereLight" || type == "sphere")
                this->light = rtx->CreateSphericalLight();
            else if (type == "SpotLight" || type == "spot")
                this->light = rtx->CreateSpotLight();
            else if (type == "QuadLight" || type == "quad")
                this->light = rtx->CreateQuadLight();
            else if (type == "AmbientLight" || type == "ambient")
                this->light = rtx->CreateAmbientLight();
            else if (type == "HDRILight" || type == "hdri")
                this->light = rtx->CreateHDRILight();
            else
            {
                vtkLogF(ERROR, "VisRTX Error: Unhandled light type \"%s\"", type.c_str());
                assert(false);
            }
        }

        ~Light()
        {
            this->light->Release();
        }

        std::string GetType()
        {
            switch(this->light->GetType())
            {
            case VisRTX::LightType::AMBIENT:
                return "ambient";
            case VisRTX::LightType::DIRECTIONAL:
                return "distant";
            case VisRTX::LightType::SPHERICAL:
                return "sphere";
            case VisRTX::LightType::SPOT:
                return "spot";
            case VisRTX::LightType::QUAD:
                return "quad";
            case VisRTX::LightType::HDRI:
                return "hdri";
            default:
                return "unknown";
            }
        }

        void Commit() override
        {
            VisRTX::Vec3f color;
            if (this->GetVec3f({ "color" }, &color))
                this->light->SetColor(color);

            float intensity;
            if (this->GetFloat({ "intensity" }, &intensity))
                this->light->SetIntensity(intensity);

            /*
             * Directional
             */
            if (this->light->GetType() == VisRTX::LightType::DIRECTIONAL)
            {
                VisRTX::DirectionalLight* dirLight = dynamic_cast<VisRTX::DirectionalLight*>(this->light);

                VisRTX::Vec3f direction;
                if (this->GetVec3f({ "direction" }, &direction))
                    dirLight->SetDirection(direction);

                float angularDiameter;
                if (this->GetFloat({ "angularDiameter" }, &angularDiameter))
                    dirLight->SetAngularDiameter(angularDiameter);
            }

            /*
             * Spherical
             */
            else if (this->light->GetType() == VisRTX::LightType::SPHERICAL)
            {
                VisRTX::SphericalLight* sphereLight = dynamic_cast<VisRTX::SphericalLight*>(this->light);

                VisRTX::Vec3f position;
                if (this->GetVec3f({ "position" }, &position))
                    sphereLight->SetPosition(position);

                float radius;
                if (this->GetFloat({ "radius" }, &radius))
                    sphereLight->SetRadius(radius);
            }

            /*
             * Spot
             */
            else if (this->light->GetType() == VisRTX::LightType::SPOT)
            {
                VisRTX::SpotLight* spot = dynamic_cast<VisRTX::SpotLight*>(this->light);

                VisRTX::Vec3f position;
                if (this->GetVec3f({ "position" }, &position))
                    spot->SetPosition(position);

                VisRTX::Vec3f direction;
                if (this->GetVec3f({ "direction" }, &direction))
                    spot->SetDirection(direction);

                float openingAngle;
                if (this->GetFloat({ "openingAngle" }, &openingAngle))
                    spot->SetOpeningAngle(openingAngle);

                float penumbraAngle;
                if (this->GetFloat({ "penumbraAngle" }, &penumbraAngle))
                    spot->SetPenumbraAngle(penumbraAngle);

                float radius;
                if (this->GetFloat({ "radius" }, &radius))
                    spot->SetRadius(radius);
            }

            /*
             * Quad
             */
            else if (this->light->GetType() == VisRTX::LightType::QUAD)
            {
                VisRTX::QuadLight* quad = dynamic_cast<VisRTX::QuadLight*>(this->light);

                VisRTX::Vec3f position, edge1, edge2;
                if (this->GetVec3f({ "position" }, &position) && this->GetVec3f({ "edge1" }, &edge1) && this->GetVec3f({ "edge2" }, &edge2))
                    quad->SetRect(position, edge1, edge2);

                quad->SetTwoSided(false);
            }

            /*
             * HDRI
             */
            else if (this->light->GetType() == VisRTX::LightType::HDRI)
            {
                VisRTX::HDRILight* hdri = dynamic_cast<VisRTX::HDRILight*>(this->light);

                Texture* texture = this->GetObject<Texture>({ "map" });
                if (texture)
                    hdri->SetTexture(texture->texture);

                VisRTX::Vec3f direction;
                if (this->GetVec3f({ "dir", "direction" }, &direction))
                    hdri->SetDirection(direction);

                VisRTX::Vec3f up;
                if (this->GetVec3f({ "up" }, &up))
                    hdri->SetUp(up);
            }
        }

    private:
        VisRTX::Light* light = nullptr;
    };
}
