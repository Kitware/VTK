/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTDistributionImplementationWrapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @brief
 * Wrapper for forward declaration of
 * OT::DistributionFactoryImplementation::Implementation
 *
*/

#ifndef vtkOTDistributionImplementationWrapper_h
#define vtkOTDistributionImplementationWrapper_h

#include "vtkOTIncludes.h"

class vtkOTDistributionImplementationWrapper
{
public:
  vtkOTDistributionImplementationWrapper(OT::DistributionFactoryImplementation::Implementation impl)
    : Implementation(impl){}

  OT::DistributionFactoryImplementation::Implementation Implementation;
};
#endif
// VTK-HeaderTest-Exclude: vtkOTDistributionImplementationWrapper.h
