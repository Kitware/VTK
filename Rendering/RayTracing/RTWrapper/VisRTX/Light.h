#pragma once

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
                std::cerr << "Error: Unhandled light type \"" << type << "\"" << std::endl;
                assert(false);
            }
        }

        ~Light()
        {
            this->light->Release();
        }

        void Commit() override
        {
            VisRTX::Vec3f color;
            if (this->Get3f({ "color" }, &color))
                this->light->SetColor(color);

            float intensity;
            if (this->Get1f({ "intensity" }, &intensity))
                this->light->SetIntensity(intensity);

            /*
             * Directional
             */
            if (this->light->GetType() == VisRTX::LightType::DIRECTIONAL)
            {
                VisRTX::DirectionalLight* dirLight = dynamic_cast<VisRTX::DirectionalLight*>(this->light);

                VisRTX::Vec3f direction;
                if (this->Get3f({ "direction" }, &direction))
                    dirLight->SetDirection(direction);

                float angularDiameter;
                if (this->Get1f({ "angularDiameter" }, &angularDiameter))
                    dirLight->SetAngularDiameter(angularDiameter);
            }

            /*
             * Spherical
             */
            else if (this->light->GetType() == VisRTX::LightType::SPHERICAL)
            {
                VisRTX::SphericalLight* sphereLight = dynamic_cast<VisRTX::SphericalLight*>(this->light);

                VisRTX::Vec3f position;
                if (this->Get3f({ "position" }, &position))
                    sphereLight->SetPosition(position);

                float radius;
                if (this->Get1f({ "radius" }, &radius))
                    sphereLight->SetRadius(radius);
            }

            /*
             * Spot
             */
            else if (this->light->GetType() == VisRTX::LightType::SPOT)
            {
                VisRTX::SpotLight* spot = dynamic_cast<VisRTX::SpotLight*>(this->light);

                VisRTX::Vec3f position;
                if (this->Get3f({ "position" }, &position))
                    spot->SetPosition(position);

                VisRTX::Vec3f direction;
                if (this->Get3f({ "direction" }, &direction))
                    spot->SetDirection(direction);

                float openingAngle;
                if (this->Get1f({ "openingAngle" }, &openingAngle))
                    spot->SetOpeningAngle(openingAngle);

                float penumbraAngle;
                if (this->Get1f({ "penumbraAngle" }, &penumbraAngle))
                    spot->SetPenumbraAngle(penumbraAngle);

                float radius;
                if (this->Get1f({ "radius" }, &radius))
                    spot->SetRadius(radius);
            }

            /*
             * Quad
             */
            else if (this->light->GetType() == VisRTX::LightType::QUAD)
            {
                VisRTX::QuadLight* quad = dynamic_cast<VisRTX::QuadLight*>(this->light);

                VisRTX::Vec3f position, edge1, edge2;
                if (this->Get3f({ "position" }, &position) && this->Get3f({ "edge1" }, &edge1) && this->Get3f({ "edge2" }, &edge2))
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
                if (this->Get3f({ "dir", "direction" }, &direction))
                    hdri->SetDirection(direction);

                VisRTX::Vec3f up;
                if (this->Get3f({ "up" }, &up))
                    hdri->SetUp(up);
            }
        }

    private:
        VisRTX::Light* light = nullptr;
    };
}
