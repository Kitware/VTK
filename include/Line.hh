/*=========================================================================

  Program:   Visualization Library
  Module:    Line.hh
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
// Computational class for lines.
//
#ifndef __vlLine_h
#define __vlLine_h

#include "Cell.hh"

class vlLine : public vlCell
{
public:
  vlLine() {};
  char *GetClassName() {return "vlLine";};

  int CellDimension() {return 1;};
  float EvaluatePosition(float x[3], int& subId, float pcoords[3]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3]);

  Intersection(float x[3], float xray[3], float x1[3], float x2[3],
               float& u, float& v);
};

#endif


