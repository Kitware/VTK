/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GlrCam.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkGlrCamera - SGI gl camera
// .SECTION Description
// vtkGlrCamera is a concrete implementation of the abstract class vtkCamera.
// vtkGlrCamera interfaces to the Silicon Graphics gl rendering library.

#ifndef __vtkGlrCamera_hh
#define __vtkGlrCamera_hh

#include "CamDev.hh"

class vtkGlrRenderer;

class vtkGlrCamera : public vtkCameraDevice
{
 public:
  virtual char *GetClassName() {return "vtkGlrCamera";};

  void Render(vtkCamera *cam, vtkRenderer *ren);
  void Render(vtkCamera *cam, vtkGlrRenderer *ren);

};

#endif
