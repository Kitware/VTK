/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestingObjectFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTestingObjectFactory.h"

vtkCxxRevisionMacro(vtkTestingInteractor, "1.1");
vtkCxxRevisionMacro(vtkTestingObjectFactory, "1.1");

VTK_CREATE_CREATE_FUNCTION(vtkTestingInteractor);

vtkTestingObjectFactory::vtkTestingObjectFactory()
{
  this->RegisterOverride("vtkRenderWindowInteractor",
                         "vtkTestingInteractor",
                         "Overrides for testing",
                         1,
                         vtkObjectFactoryCreatevtkTestingInteractor);
}
