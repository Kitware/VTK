// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkmInitializer.h"
#include <vtkm/cont/Initialize.h>

VTK_ABI_NAMESPACE_BEGIN
void InitializeVTKm()
{
// Only Kokkos HIP backend needs to be initialized
#ifdef VTKM_HIP
  static bool isInitialized{ false };
  if (!isInitialized)
  {
    int argc{ 1 };
    char const* argv[]{ "vtkm", nullptr };
    vtkm::cont::Initialize(argc, const_cast<char**>(argv));
    isInitialized = true;
  }
#endif
}

vtkmInitializer::vtkmInitializer()
{
  (void)InitializeVTKm();
}
VTK_ABI_NAMESPACE_END
