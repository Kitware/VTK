/*=========================================================================

  Program:   Visualization Library
  Module:    Plane.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

//
// Object for computation on plane
//
#ifndef __vlPlane_h
#define __vlPlane_h

#define TOL 1.e-05 // Tolerance for geometric calculation

#include "Object.hh"

class vlPlane : public vlObject
{
public:
  vlPlane() {};
  char *GetClassName() {return "vlPlane";};

  int ProjectPoint(float x[3], float origin[3], float normal[3], float xproj[3]);

};

#endif


