/*=========================================================================

  Program:   Visualization Library
  Module:    SbrCam.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlSbrCamera - HP starbase camera
// .SECTION Description
// vlSbrCamera is a concrete implementation of the abstract class vlCamera.
// vlSbrCamera interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vlSbrCamera_hh
#define __vlSbrCamera_hh

#include "Camera.hh"
#include "starbase.c.h"

class vlSbrRenderer;

class vlSbrCamera : public vlCamera
{
 public:
  char *GetClassName() {return "vlSbrCamera";};

  void Render(vlRenderer *ren);
  void Render(vlSbrRenderer *ren);

};

#endif
