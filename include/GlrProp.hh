/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GlrProp.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkGlrProperty - SGI gl property
// .SECTION Description
// vtkGlrProperty is a concrete implementation of the abstract class vtkProperty.
// vtkGlrProperty interfaces to the Silicon Graphics gl rendering library.

#ifndef __vtkGlrProperty_hh
#define __vtkGlrProperty_hh

#include "PropDev.hh"

class vtkGlrRenderer;

class vtkGlrProperty : public vtkPropertyDevice
{
 public:
  char *GetClassName() {return "vtkGlrProperty";};

  void Render(vtkProperty *prop, vtkRenderer *ren);
  void Render(vtkProperty *prop, vtkGlrRenderer *ren);
};

#endif
