/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkParallelFactory.h"
#include "vtkPPolyDataNormals.h"
#include "vtkPSphereSource.h"
#include "vtkPOutlineCornerFilter.h"
#include "vtkPOutlineFilter.h"
#include "vtkPStreamTracer.h"
#include "vtkPProbeFilter.h"
#include "vtkPLinearExtrusionFilter.h"
#include "vtkVersion.h"
#ifdef VTK_USE_RENDERING
#  include "vtkPImageWriter.h"
#endif // VTK_USE_RENDERING

vtkStandardNewMacro(vtkParallelFactory);

void vtkParallelFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "VTK Parallel object factory" << endl;
}


#ifdef VTK_USE_RENDERING
VTK_CREATE_CREATE_FUNCTION(vtkPImageWriter);
#endif // VTK_USE_RENDERING
VTK_CREATE_CREATE_FUNCTION(vtkPPolyDataNormals);
VTK_CREATE_CREATE_FUNCTION(vtkPSphereSource);
VTK_CREATE_CREATE_FUNCTION(vtkPStreamTracer);
VTK_CREATE_CREATE_FUNCTION(vtkPLinearExtrusionFilter);
VTK_CREATE_CREATE_FUNCTION(vtkPOutlineCornerFilter);
VTK_CREATE_CREATE_FUNCTION(vtkPOutlineFilter);
VTK_CREATE_CREATE_FUNCTION(vtkPProbeFilter);

vtkParallelFactory::vtkParallelFactory()
{
#ifdef VTK_USE_RENDERING
  this->RegisterOverride("vtkImageWriter",
                         "vtkPImageWriter",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPImageWriter);
#endif // VTK_USE_RENDERING
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
  this->RegisterOverride("vtkStreamTracer",
                         "vtkPStreamTracer",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPStreamTracer);
  this->RegisterOverride("vtkLinearExtrusionFilter",
                         "vtkPLinearExtrusionFilter",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPLinearExtrusionFilter);
  this->RegisterOverride("vtkOutlineCornerFilter",
                         "vtkPOutlineCornerFilter",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPOutlineCornerFilter);
  this->RegisterOverride("vtkOutlineFilter",
                         "vtkPOutlineFilter",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPOutlineFilter);
  this->RegisterOverride("vtkProbeFilter",
                         "vtkPProbeFilter",
                         "Parallel",
                         1,
                         vtkObjectFactoryCreatevtkPProbeFilter);
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
