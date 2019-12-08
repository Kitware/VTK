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

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "openturns/DistributionFactoryImplementation.hxx"
#include "openturns/DistributionImplementation.hxx"
#include "openturns/Epanechnikov.hxx"
#include "openturns/KernelSmoothing.hxx"
#include "openturns/Point.hxx"
#include "openturns/ResourceMap.hxx"
#include "openturns/Sample.hxx"
#include "openturns/Triangular.hxx"

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif

#endif
// VTK-HeaderTest-Exclude: vtkOTIncludes.h
