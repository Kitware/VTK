/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GlrLgt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkGlrLight - SGI gl light
// .SECTION Description
// vtkGlrLight is a concrete implementation of the abstract class vtkLight.
// vtkGlrLight interfaces to the Silicon Graphics gl rendering library.

#ifndef __vtkGlrLight_hh
#define __vtkGlrLight_hh

#include "LgtDev.hh"

class vtkGlrRenderer;

class vtkGlrLight : public vtkLightDevice
{
protected:
  
public:
  char *GetClassName() {return "vtkGlrLight";};

  void Render(vtkLight *lgt, vtkRenderer *ren,int light_index);
  void Render(vtkLight *lgt, vtkGlrRenderer *ren,int light_index);
};

#endif

