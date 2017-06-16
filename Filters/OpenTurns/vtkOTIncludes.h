/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTIncludes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkOTIncludes_h
#define vtkOTIncludes_h

#include "vtkOTConfig.h"

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
 #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#if (OPENTURNS_VERSION_MAJOR == 1 && OPENTURNS_VERSION_MINOR == 8)
 #include "openturns/NumericalPoint.hxx"
 #include "openturns/NumericalSample.hxx"
#else
 #include "openturns/Point.hxx"
 #include "openturns/Sample.hxx"
#endif

#include "openturns/DistributionFactoryImplementation.hxx"
#include "openturns/DistributionImplementation.hxx"
#include "openturns/Epanechnikov.hxx"
#include "openturns/KernelSmoothing.hxx"
#include "openturns/ResourceMap.hxx"
#include "openturns/Triangular.hxx"

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
 #pragma GCC diagnostic pop
#endif

#endif
// VTK-HeaderTest-Exclude: vtkOTIncludes.h
