/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrProp.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkSbrProperty - HP starbase property
// .SECTION Description
// vtkSbrProperty is a concrete implementation of the abstract class vtkProperty.
// vtkSbrProperty interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vtkSbrProperty_hh
#define __vtkSbrProperty_hh

#include "PropDev.hh"

class vtkSbrRenderer;

class vtkSbrProperty : public vtkPropertyDevice
{
 public:
  char *GetClassName() {return "vtkSbrProperty";};

  void Render(vtkProperty *prop, vtkRenderer *ren);
  void Render(vtkProperty *prop, vtkSbrRenderer *ren);
};

#endif
