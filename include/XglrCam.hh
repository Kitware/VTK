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

#include "Camera.hh"

class vlXglrRenderer;

class vlXglrCamera : public vlCamera
{
 public:
  char *GetClassName() {return "vlXglrCamera";};

  void Render(vlRenderer *ren);
  void Render(vlXglrRenderer *ren);

};

#endif
