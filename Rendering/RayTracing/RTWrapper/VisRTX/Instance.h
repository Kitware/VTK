// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "../Types.h"
#include "Group.h"

#include <VisRTX.h>

#include <set>

namespace RTW
{
VTK_ABI_NAMESPACE_BEGIN
    class Instance : public Object
    {
        friend class World;

    public:
        Instance(Group *_group)
            : Object(RTW_INSTANCE),
            group(_group)
        {
            if(group)
                group->AddRef();
        }

        ~Instance()
        {
            if(group)
                group->Release();
        }

        void Commit() override {}

    private:
        Group *group;
    };
VTK_ABI_NAMESPACE_END
}
