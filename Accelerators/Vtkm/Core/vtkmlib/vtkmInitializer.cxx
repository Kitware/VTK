// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkmInitializer.h"
#include <viskores/cont/Initialize.h>

VTK_ABI_NAMESPACE_BEGIN
void InitializeVTKm()
{
// Kokkos enabled devices needs to be initialized
#ifdef VTK_USE_KOKKOS
  static bool isInitialized{ false };
  if (!isInitialized)
  {
    int argc{ 1 };
    char const* argv[]{ "viskores", nullptr };
    viskores::cont::Initialize(argc, const_cast<char**>(argv));
    isInitialized = true;
  }
#endif
}

vtkmInitializer::vtkmInitializer()
{
  InitializeVTKm();
}
VTK_ABI_NAMESPACE_END
