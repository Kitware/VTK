#pragma once

#include "../Types.h"
#include "Geometry.h"

#include <VisRTX.h>

#include <set>

namespace RTW
{
    class GeometricModel : public Object
    {
        friend class World;

    public:
        GeometricModel(Geometry *_geometry)
            : Object(RTW_GEOMETRIC_MODEL), geometry(_geometry)
        {
            if(geometry)
                geometry->AddRef();
        }

        ~GeometricModel()
        {
            if(geometry)
                geometry->Release();
        }

        void Commit() override
        {
            //Forward "material" data to geometry if extant.
            bool found = false;
            Material *material = reinterpret_cast<Material *>(GetObject<Material>({"material"}, nullptr, &found));
            if(found)
            {
                if(material->GetDataType() == RTW_DATA)
                {
                    geometry->SetObject("material", reinterpret_cast<Data *>(material));
                    geometry->Commit();
                }
                else
                {
                    assert(material->GetDataType() == RTW_MATERIAL);
                    geometry->SetMaterial(material);
                    geometry->Commit();
                }
            }

            //Forward "color" data to geometry if extant.
            Data *color = reinterpret_cast<Data *>(GetObject<Data>({"color"}, nullptr, &found));
            if(found)
            {
                assert(color->GetDataType() == RTW_DATA);
                geometry->SetObject("color", color);
                geometry->Commit();
            }
        }

    private:
        Geometry *geometry;
    };
}
