/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrLgt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkOglrLight - SGI OpenGL light
// .SECTION Description
// vtkOglrLight is a concrete implementation of the abstract class vtkLight.
// vtkOglrLight interfaces to the Silicon Graphics OpenGL rendering library.

#ifndef __vtkOglrLight_hh
#define __vtkOglrLight_hh

#include "LgtDev.hh"

class vtkOglrRenderer;

class vtkOglrLight : public vtkLightDevice
{
protected:
  
public:
  char *GetClassName() {return "vtkOglrLight";};

  void Render(vtkLight *lgt, vtkRenderer *ren,int light_index);
  void Render(vtkLight *lgt, vtkOglrRenderer *ren,int light_index);
};

#endif

