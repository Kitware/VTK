/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrLgt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkXglrLight - Light for Suns XGL
// .SECTION Description
// vtkXglrLight is a concrete implementation of the abstract class vtkLight.
// vtkXglrLight interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vtkXglrLight_hh
#define __vtkXglrLight_hh

#include "LgtDev.hh"

class vtkXglrRenderer;

class vtkXglrLight : public vtkLightDevice
{
protected:
  
public:
  char *GetClassName() {return "vtkXglrLight";};

  void Render(vtkLight *lgt, vtkRenderer *ren,int light_index);
  void Render(vtkLight *lgt, vtkXglrRenderer *ren,int light_index);
  
};

#endif

