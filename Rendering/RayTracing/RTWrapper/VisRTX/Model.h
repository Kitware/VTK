#pragma once

#include "../Types.h"
#include "Geometry.h"

#include <VisRTX.h>

#include <set>

namespace RTW
{
    class Model : public Object
    {
        friend class Renderer;

    public:
        Model()
        {
            VisRTX::Context* rtx = VisRTX_GetContext();
            this->model = rtx->CreateModel();
        }

        ~Model()
        {
            for (Geometry* geometry : this->geometries)
                if (geometry)
                    geometry->Release();

            this->model->Release();
        }

        void Commit() override
        {
        }

        void AddGeometry(Geometry* geometry)
        {
            if (!geometry)
                return;

            // Check if already added
            auto it = this->geometries.find(geometry);
            if (it != this->geometries.end())
                return;


            // Add and increase ref count
            this->model->AddGeometry(geometry->geometry);
            this->geometries.insert(geometry);
            geometry->AddRef();
        }

    private:
        VisRTX::Model* model = nullptr;
        std::set<Geometry*> geometries;
    };
}
