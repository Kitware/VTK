/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PropDev.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkDeviceObject - an object that requires hardware independence.
// .SECTION Description
// vtkDeviceObject is the superclass that any device dependent object should
// use.  It allows a device independent object to create a device dependent
// object to execute hardware specific calls.

#ifndef __vtkPropertyDevice_hh
#define __vtkPropertyDevice_hh

#include "Object.hh"
class vtkRenderer;
class vtkProperty;

class vtkPropertyDevice : public vtkObject
{
public:
  char *GetClassName() {return "vtkPropertyDevice";};
  virtual void Render(vtkProperty *prp, vtkRenderer *ren) = 0;
};

#endif
