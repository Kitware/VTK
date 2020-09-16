#pragma once

#include "../Types.h"
#include "Geometry.h"

#include <VisRTX.h>

#include <set>

namespace RTW
{
    class World : public Object
    {
        friend class Renderer;

    public:
        World()
            : Object(RTW_WORLD)
        {
            VisRTX::Context* rtx = VisRTX_GetContext();
            this->model = rtx->CreateModel();
        }

        ~World()
        {
            for (Geometry* geometry : this->geometries)
                if (geometry)
                    geometry->Release();
            this->model->Release();
        }

        void Commit() override
        {
        }

        void SetObject(const std::string& id, Object *object) override
        {
            Object::SetObject(id, object);
            if(object && object->GetDataType() == RTW_DATA)
            {
                Data *data = reinterpret_cast<Data *>(object);
                if(data->GetElementDataType() == RTW_INSTANCE)
                {
                    int numElements = data->GetNumElements();
                    Instance **elemData = reinterpret_cast<Instance **>(data->GetData());
                    while(numElements-- > 0)
                    {
                        Object *elem = *elemData;
                        assert(elem->GetDataType() == RTW_INSTANCE);
                        Group *group = reinterpret_cast<Instance *>(elem)->group;
                        std::set<GeometricModel *> &geoModels = group->geometricModels;
                        for(auto &m : geoModels)
                            AddGeometry(m->geometry);
                        elemData++;
                    }
                }
            }
            else if(object && object->GetDataType() == RTW_INSTANCE)
            {
                Group *group = reinterpret_cast<Instance *>(object)->group;
                std::set<GeometricModel *> &geoModels = group->geometricModels;
                for(auto &m : geoModels)
                    AddGeometry(m->geometry);
            }
        }

        void RemoveParam(const std::string& id) override
        {
            Object *obj = GetObject({id});
            if(obj && obj->GetDataType() == RTW_INSTANCE)
            {
                Group *group = reinterpret_cast<Instance *>(obj)->group;
                std::set<GeometricModel *> geoModels = group->geometricModels;
                for(auto &m : geoModels)
                    RemoveGeometry(m->geometry);
            }
            Object::RemoveParam(id);
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

        void RemoveGeometry(Geometry* geometry)
        {
            if (!geometry)
                return;

            // Check if already added
            auto it = this->geometries.find(geometry);
            if (it != this->geometries.end())
            {
                this->model->RemoveGeometry(geometry->geometry);
                this->geometries.erase(geometry);
                geometry->Release();
            }
        }



    private:
        //The OSPRay 'world' object roughly corresponds to a VisRTX 'model' object.
        VisRTX::Model* model = nullptr;
        std::set<Geometry*> geometries;
    };
}
