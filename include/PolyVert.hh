/*=========================================================================

  Program:   Visualization Library
  Module:    PolyVert.hh
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
// Computational class for polypoints.
//
#ifndef __vlPolyPoints_h
#define __vlPolyPoints_h

#include "Cell.hh"

class vlPolyPoints : public vlCell
{
public:
  vlPolyPoints() {};
  char *GetClassName() {return "vlPolyPoints";};

  int CellDimension() {return 0;};
  float EvaluatePosition(float x[3], int& subId, float pcoords[3]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3]);

};

#endif


