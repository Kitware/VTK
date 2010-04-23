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
#include "vtkVersion.h"
#include "vtkTestingObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkTestingObjectFactory);

VTK_CREATE_CREATE_FUNCTION(vtkTestingInteractor);

vtkTestingObjectFactory::vtkTestingObjectFactory()
{
  this->RegisterOverride("vtkRenderWindowInteractor",
                         "vtkTestingInteractor",
                         "Overrides for testing",
                         1,
                         vtkObjectFactoryCreatevtkTestingInteractor);
}

const char* vtkTestingObjectFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}

void vtkTestingObjectFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Description: " << this->GetDescription() << endl;
}

