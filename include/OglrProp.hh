/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrProp.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkOglrProperty - SGI OpenGL property
// .SECTION Description
// vtkOglrProperty is a concrete implementation of the abstract class 
// vtkProperty. vtkOglrProperty interfaces to the Silicon Graphics 
// OpenGL rendering library.

#ifndef __vtkOglrProperty_hh
#define __vtkOglrProperty_hh

#include "PropDev.hh"

class vtkOglrRenderer;

class vtkOglrProperty : public vtkPropertyDevice
{
 public:
  char *GetClassName() {return "vtkOglrProperty";};

  void Render(vtkProperty *prop, vtkRenderer *ren);
  void Render(vtkProperty *prop, vtkOglrRenderer *ren);
};

#endif
