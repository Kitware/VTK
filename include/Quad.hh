/*=========================================================================

  Program:   Visualization Library
  Module:    Quad.hh
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
// Computational class for quads.
//
#ifndef __vlQuad_h
#define __vlQuad_h

#include "Cell.hh"

class vlQuad : public vlCell
{
public:
  vlQuad() {};
  char *GetClassName() {return "vlQuad";};

  float DistanceToPoint(float *x);

};

#endif


