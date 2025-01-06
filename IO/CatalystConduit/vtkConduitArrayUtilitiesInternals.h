// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkConduitArrayUtilitiesInternals_h
#define vtkConduitArrayUtilitiesInternals_h

#include "vtkABINamespace.h"    // for VTK_ABI_NAMESPACE_BEGIN
#include <catalyst_conduit.hpp> // for conduit_cpp::DataType::Id

namespace internals
{
VTK_ABI_NAMESPACE_BEGIN
conduit_cpp::DataType::Id GetTypeId(conduit_cpp::DataType::Id type, bool force_signed);
VTK_ABI_NAMESPACE_END
};

#endif
