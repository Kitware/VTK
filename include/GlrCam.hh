/*=========================================================================

  Program:   Visualization Library
  Module:    GlrCam.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlGlrCamera - SGI gl camera
// .SECTION Description
// vlGlrCamera is a concrete implementation of the abstract class vlCamera.
// vlGlrCamera interfaces to the Silicon Graphics gl rendering library.

#ifndef __vlGlrCamera_hh
#define __vlGlrCamera_hh

#include "CamDev.hh"

class vlGlrRenderer;

class vlGlrCamera : public vlCameraDevice
{
 public:
  virtual char *GetClassName() {return "vlGlrCamera";};

  void Render(vlCamera *cam, vlRenderer *ren);
  void Render(vlCamera *cam, vlGlrRenderer *ren);

};

#endif
