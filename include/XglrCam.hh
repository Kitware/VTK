/*=========================================================================

  Program:   Visualization Library
  Module:    XglrCam.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlXglrCamera - Camera for Suns XGL
// .SECTION Description
// vlXglrCamera is a concrete implementation of the abstract class vlCamera.
// vlXglrCamera interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vlXglrCamera_hh
#define __vlXglrCamera_hh

#include "CamDev.hh"

class vlXglrRenderer;

class vlXglrCamera : public vlCameraDevice
{
 public:
  char *GetClassName() {return "vlXglrCamera";};

  void Render(vlCamera *cam, vlRenderer *ren);
  void Render(vlCamera *cam, vlXglrRenderer *ren);

};

#endif
