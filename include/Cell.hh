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

#define MAX_CELL_SIZE 128
#define TOL 1.e-05 // Tolerance for geometric calculation

#include "Object.hh"
#include "FPoints.hh"
#include "FScalars.hh"
#include "CellArr.hh"
#include "IdList.hh"

class vlCell : public vlObject
{
public:
  vlCell(): Points(MAX_CELL_SIZE), PointIds(MAX_CELL_SIZE) {};
  void Initialize(int npts, int *pts, vlPoints *p);
  char *GetClassName() {return "vlCell";};

  // Because these objects (cells and derived classes) are computational 
  // objects, and because they are used internally, do not use memory 
  // reference counting.

  // Number of points in cell
  int GetNumberOfPoints() {return this->PointIds.GetNumberOfIds();};

  // Dimensionality of cell (0,1,2, or 3)
  virtual int CellDimension() = 0;

  // given a point x[3] return inside(=1) or outside(=0) cell; evaluate 
  // parametric coordinates, sub-cell id (!=0 only if cell is composite), and 
  // distance squared  of point x[3] to cell (in particular, the sub-cell 
  // indicated).
  virtual int EvaluatePosition(float x[3], int& subId, float pcoords[3], float& dist2) = 0;

  // Determine global coordinate from subId and parametric coordinates
  virtual void EvaluateLocation(int& subId, float pcoords[3], float x[3]) = 0;

  // Generate contouring primitives
  virtual void Contour(float value, vlFloatScalars *cellScalars, 
                       vlFloatPoints *points, vlCellArray *verts, 
                       vlCellArray *lines, vlCellArray *polys, 
                       vlFloatScalars *scalars) = 0;

  // Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax)
  float *GetBounds();

  // Quick intersection of cell bounding box.  Returns != 0 for hit.
  char HitBBox(float bounds[6], float origin[3], float dir[3], float coord[3]);

  // Point coordinates for cell.
  vlFloatPoints *GetPoints() {return &this->Points;};

  // Point ids for cell.
  vlIdList *GetPointIds() {return &this->PointIds;};

  // left public for quick computational access
  vlFloatPoints Points;
  vlIdList PointIds;

};

#endif


