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

#define TOL 1.e-05 // Tolerance for geometric calculation

#include "Object.hh"
#include "FPoints.hh"

class vlCell : public vlObject
{
public:
  vlCell():Points(0) {};
  char *GetClassName() {return "vlCell";};

  void SetPoints(vlFloatPoints *pts) {this->Points = pts;};

  // return inside cell (!=0) if <= tolerance squared; evaluate parametric 
  // coordinates, sub-cell id (if cell is composite), and distance squared 
  // of point x[3] to cell.
  virtual float EvaluatePosition(float x[3], int& subId, float pcoords[3]) = 0;
//  virtual int EvaluatePosition(float x[3], float& tol2,
//                               int& subId, float pcoords[3], float& dist2) = 0;

  // Determine global coordinate from subId and parametric coordinates
  virtual void EvaluateLocation(int& subId, float pcoords[3], float x[3]) = 0;

  // Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax)
  float *GetBounds();

  // Quick intersection of cell bounding box.  Returns != 0 for hit.
  char HitBBox(float bounds[6], float origin[3], float dir[3], float coord[3]);

protected:
  vlFloatPoints *Points;

};

#endif


