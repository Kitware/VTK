/*=========================================================================

  Program:   Visualization Library
  Module:    Voxel.hh
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
// Computational class for bricks.
//
#ifndef __vlBrick_h
#define __vlBrick_h

#include "Cell.hh"

class vlBrick : public vlCell
{
public:
  vlBrick() {};
  char *GetClassName() {return "vlBrick";};

  int CellDimension() {return 3;};
  float EvaluatePosition(float x[3], int& subId, float pcoords[3]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3]);
  void ShapeFunctions(float pcoords[3], float sf[8]);
  void ShapeDerivs(float pcoords[3], float derivs[24]);

};

#endif


