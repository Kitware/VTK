//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include "vtkmInitializer.h"
#include <vtkm/cont/Initialize.h>

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
