/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrProp.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkXglrProperty - Suns XGL property
// .SECTION Description
// vtkXglrProperty is a concrete implementation of the abstract class 
// vtkProperty. vtkXglrProperty interfaces to Suns XGL rendering library.

#ifndef __vtkXglrProperty_hh
#define __vtkXglrProperty_hh

#include "PropDev.hh"

class vtkXglrRenderer;

class vtkXglrProperty : public vtkPropertyDevice
{
 public:
  char *GetClassName() {return "vtkXglrProperty";};

  void Render(vtkProperty *prop, vtkRenderer *ren);
  void Render(vtkProperty *prop, vtkXglrRenderer *ren);
};

#endif
