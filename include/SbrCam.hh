/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrCam.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkSbrCamera - HP starbase camera
// .SECTION Description
// vtkSbrCamera is a concrete implementation of the abstract class vtkCamera.
// vtkSbrCamera interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vtkSbrCamera_hh
#define __vtkSbrCamera_hh

#include "CamDev.hh"
#include "starbase.c.h"

class vtkSbrRenderer;

class vtkSbrCamera : public vtkCameraDevice
{
 public:
  char *GetClassName() {return "vtkSbrCamera";};

  void Render(vtkCamera *, vtkRenderer *ren);
  void Render(vtkCamera *, vtkSbrRenderer *ren);
};

#endif
