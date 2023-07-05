// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "../Types.h"
#include "Geometry.h"

#include <VisRTX.h>

#include <set>

namespace RTW
{
VTK_ABI_NAMESPACE_BEGIN
    class Group : public Object
    {
        friend class World;

    public:
        Group() : Object(RTW_GROUP) {}

        ~Group() {}

        void Commit() override {}

        void SetObject(const std::string& id, Object *object) override
        {
            Object::SetObject(id, object);
            if(object && object->GetDataType() == RTW_DATA)
            {
                Data *data = reinterpret_cast<Data *>(object);
                if(data->GetElementDataType() == RTW_GEOMETRIC_MODEL)
                {
                    int numElements = data->GetNumElements();
                    Object **elemData = reinterpret_cast<Object **>(data->GetData());
                    while(numElements-- > 0)
                    {
                        Object *elem = *elemData;
                        assert(elem->GetDataType() == RTW_GEOMETRIC_MODEL);
                        geometricModels.insert(reinterpret_cast<GeometricModel *>(elem));
                        elemData++;
                    }
                }
            }
            else if(object && object->GetDataType() == RTW_GEOMETRIC_MODEL)
                geometricModels.insert(reinterpret_cast<GeometricModel *>(object));
        }

        void RemoveParam(const std::string& id) override
        {
            Object *obj = GetObject({id});
            if(obj && obj->GetDataType() == RTW_GEOMETRIC_MODEL)
                geometricModels.erase(reinterpret_cast<GeometricModel *>(obj));
            Object::RemoveParam(id);
        }


    private:
        std::set<GeometricModel *> geometricModels;

    };
VTK_ABI_NAMESPACE_END
}
