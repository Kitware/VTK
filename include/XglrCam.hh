/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrCam.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkXglrCamera - Camera for Suns XGL
// .SECTION Description
// vtkXglrCamera is a concrete implementation of the abstract class vtkCamera.
// vtkXglrCamera interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vtkXglrCamera_hh
#define __vtkXglrCamera_hh

#include "CamDev.hh"

class vtkXglrRenderer;

class vtkXglrCamera : public vtkCameraDevice
{
 public:
  char *GetClassName() {return "vtkXglrCamera";};

  void Render(vtkCamera *cam, vtkRenderer *ren);
  void Render(vtkCamera *cam, vtkXglrRenderer *ren);

};

#endif
