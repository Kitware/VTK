/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ObjectFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkObjectFactory.h>
#include <vtkObjectFactoryCollection.h>
#include <vtkOverrideInformation.h>
#include <vtkOverrideInformationCollection.h>
#include <vtkVersion.h>

#include "vtkDaxThreshold.h"
VTK_CREATE_CREATE_FUNCTION(vtkDaxThreshold)

#include "vtkDaxMarchingCubes.h"
VTK_CREATE_CREATE_FUNCTION(vtkDaxMarchingCubes)

class VTK_EXPORT DaxObjectFactory : public vtkObjectFactory
{
public:
  static DaxObjectFactory* New();
  DaxObjectFactory();
  virtual const char* GetVTKSourceVersion() { return VTK_SOURCE_VERSION; }
  const char* GetDescription() { return "Dax Object Factory"; }
  vtkTypeMacro(DaxObjectFactory,vtkObjectFactory)

protected:
  DaxObjectFactory(const DaxObjectFactory&);
  void operator=(const DaxObjectFactory&);
};


DaxObjectFactory::DaxObjectFactory()
{
  this->RegisterOverride("vtkThreshold",
                         "vtkDaxThreshold",
                         "Override threshold with CUDA version",
                         1,
                         vtkObjectFactoryCreatevtkDaxThreshold);

  this->RegisterOverride("vtkMarchingCubes",
                         "vtkDaxMarchingCubes",
                         "Override marching cubes with CUDA version",
                         1,
                         vtkObjectFactoryCreatevtkDaxThreshold);
}

vtkStandardNewMacro(DaxObjectFactory)
