/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrCam.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkOglrCamera - SGI OpenGL camera
// .SECTION Description
// vtkOglrCamera is a concrete implementation of the abstract class vtkCamera.
// vtkOglrCamera interfaces to the Silicon Graphics OpenGL rendering library.

#ifndef __vtkOglrCamera_hh
#define __vtkOglrCamera_hh

#include "CamDev.hh"

class vtkOglrRenderer;

class vtkOglrCamera : public vtkCameraDevice
{
 public:
  virtual char *GetClassName() {return "vtkOglrCamera";};

  void Render(vtkCamera *cam, vtkRenderer *ren);
  void Render(vtkCamera *cam, vtkOglrRenderer *ren);

};

#endif
