/*=========================================================================

  Program:   Visualization Library
  Module:    Cell.hh
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
// Abstract specification of computational cells.
//
#ifndef __vlCell_h
#define __vlCell_h

#include "Object.hh"
#include "FPoints.hh"

class vlCell : public vlObject
{
public:
  vlCell() {};
  char *GetClassName() {return "vlCell";};

  void SetPoints(vlFloatPoints *pts) {this->Points = pts;};

  virtual float DistanceToPoint(float *x) = 0;

protected:
  vlFloatPoints *Points;

};

#endif


