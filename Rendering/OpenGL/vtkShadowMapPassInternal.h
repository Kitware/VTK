/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShadowMapPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// These internal classes are shared by vtkShadowMapBakerPass and
// vtkShadowMapPass.

#ifndef __vtkShadowMapPassInternal_h
#define __vtkShadowMapPassInternal_h

#include "vtkCamera.h"
#include "vtkTextureObject.h"

class vtkShadowMapBakerPassTextures
{
public:
  vtksys_stl::vector<vtkSmartPointer<vtkTextureObject> > Vector;
};

class vtkShadowMapBakerPassLightCameras
{
public:
  vtksys_stl::vector<vtkSmartPointer<vtkCamera> > Vector;
};

#endif
