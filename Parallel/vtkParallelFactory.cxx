/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelFactory.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkParallelFactory.h"
#include "vtkPImageWriter.h"
#include "vtkPPolyDataNormals.h"
#include "vtkPSphereSource.h"
#include "vtkVersion.h"

vtkCxxRevisionMacro(vtkParallelFactory, "1.8");
vtkStandardNewMacro(vtkParallelFactory);

void vtkParallelFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "VTK Parallel object factory" << endl;
}


VTK_CREATE_CREATE_FUNCTION(vtkPImageWriter);
VTK_CREATE_CREATE_FUNCTION(vtkPPolyDataNormals);
VTK_CREATE_CREATE_FUNCTION(vtkPSphereSource);


vtkParallelFactory::vtkParallelFactory()
{
  this->RegisterOverride("vtkImageWriter",
                         "vtkPImageWriter",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPImageWriter);
  this->RegisterOverride("vtkPolyDataNormals",
                         "vtkPPolyDataNormals",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPPolyDataNormals);
  this->RegisterOverride("vtkSphereSource",
                         "vtkPSphereSource",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPSphereSource);
}

const char* vtkParallelFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}

const char* vtkParallelFactory::GetDescription()
{
  return "VTK Parallel Support Factory";
}


extern "C" vtkObjectFactory* vtkLoad()
{
  return vtkParallelFactory::New();
}
