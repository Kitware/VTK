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
#include "IdList.hh"
#include "CellType.hh"

class vlCellArray;

class vlCell : public vlObject
{
public:
  vlCell(): Points(MAX_CELL_SIZE), PointIds(MAX_CELL_SIZE) {};
  void Initialize(int npts, int *pts, vlPoints *p);
  char *GetClassName() {return "vlCell";};

  // Because these objects (cells and derived classes) are computational 
  // objects, and because they are used internally, do not use memory 
  // reference counting.

  // Type of cell
  virtual int GetCellType() = 0;

  // Dimensionality of cell (0,1,2, or 3)
  virtual int GetCellDimension() = 0;

  // Point coordinates for cell.
  vlFloatPoints *GetPoints() {return &this->Points;};

  // Number of points (and other topological entities) in cell
  int GetNumberOfPoints() {return this->PointIds.GetNumberOfIds();};
  virtual int GetNumberOfEdges() = 0;
  virtual int GetNumberOfFaces() = 0;

  // Get topological entities
  vlIdList *GetPointIds() {return &this->PointIds;};
  int GetPointId(int ptId) {return this->PointIds.GetId(ptId);};
  virtual vlCell *GetEdge(int edgeId) = 0;
  virtual vlCell *GetFace(int faceId) = 0;

  // given a point x[3] return inside(=1) or outside(=0) cell; evaluate 
  // parametric coordinates, sub-cell id (!=0 only if cell is composite),
  // distance squared  of point x[3] to cell (in particular, the sub-cell 
  // indicated), and interpolation weights in cell.
  virtual int EvaluatePosition(float x[3], float closestPoint[3], 
                               int& subId, float pcoords[3], 
                               float& dist2, float weights[MAX_CELL_SIZE]) = 0;

  // Determine global coordinate from subId and parametric coordinates
  virtual void EvaluateLocation(int& subId, float pcoords[3], 
                                float x[3], float weights[MAX_CELL_SIZE]) = 0;

  // Generate contouring primitives
  virtual void Contour(float value, vlFloatScalars *cellScalars, 
                       vlFloatPoints *points, vlCellArray *verts, 
                       vlCellArray *lines, vlCellArray *polys, 
                       vlFloatScalars *scalars) = 0;

  // Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax)
  float *GetBounds();

  // Quick intersection of cell bounding box.  Returns != 0 for hit.
  char HitBBox(float bounds[6], float origin[3], float dir[3], float coord[3]);

  // left public for quick computational access
  vlFloatPoints Points;
  vlIdList PointIds;

};

#endif


