/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrLgt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkSbrLight - HP starbase light
// .SECTION Description
// vtkSbrLight is a concrete implementation of the abstract class vtkLight.
// vtkSbrLight interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vtkSbrLight_hh
#define __vtkSbrLight_hh

#include "LgtDev.hh"

class vtkSbrRenderer;

class vtkSbrLight : public vtkLightDevice
{
protected:
  
public:
  char *GetClassName() {return "vtkSbrLight";};

  void Render(vtkLight *lgt, vtkRenderer *ren,int light_index);
  void Render(vtkLight *lgt, vtkSbrRenderer *ren,int light_index);
  
};

#endif

