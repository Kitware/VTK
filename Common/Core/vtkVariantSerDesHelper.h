// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkVariantSerDesHelper_h
#define vtkVariantSerDesHelper_h

#include "vtkABINamespace.h"
#include "vtkCommonCoreModule.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkDeserializer;
class vtkSerializer;
class vtkVariant;

nlohmann::json VTKCOMMONCORE_EXPORT Serialize_vtkVariant(
  const vtkVariant* variant, vtkSerializer* serializer);

void VTKCOMMONCORE_EXPORT Deserialize_vtkVariant(
  const nlohmann::json& state, vtkVariant* variant, vtkDeserializer* deserializer);

VTK_ABI_NAMESPACE_END

#endif
