/*=========================================================================

  Program:   Visualization Library
  Module:    Cell.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCell - abstract class to specify cell behavior
// .SECTION Description
// vlCell is an abstract class that specifies the interfaces for data cells.
// Data cells are simple topological elements like points, lines, polygons, 
// and tetrahedra that visualization datasets are composed of. In some 
// cases visualization datasets may explicitly represent cells (e.g., 
// vlPolyData, vlUnstructuredGrid), and in some cases, the datasets are 
// implicitly composed of cells (e.g., vlStructuredPoints).

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

  // Description:
  // Create concrete copy of this cell.
  virtual vlCell *MakeObject() = 0;

  // Description:
  // Return the type of cell.
  virtual int GetCellType() = 0;

  // Description:
  // Return the topological dimensional of the cell (0,1,2, or 3).
  virtual int GetCellDimension() = 0;

  // Description:
  // Get the point coordinates for the cell.
  vlFloatPoints *GetPoints() {return &this->Points;};

  // Description:
  // Return the number of points in the cell.
  int GetNumberOfPoints() {return this->PointIds.GetNumberOfIds();};

  // Description:
  // Return the number of edges in the cell.
  virtual int GetNumberOfEdges() = 0;

  // Description:
  // Return the number of faces in the cell.
  virtual int GetNumberOfFaces() = 0;

  // Description:
  // Return the list of point ids defining cell.
  vlIdList *GetPointIds() {return &this->PointIds;};

  // Description:
  // For cell point i, return the actual point id.
  int GetPointId(int ptId) {return this->PointIds.GetId(ptId);};

  // Description:
  // Return the edge cell from the edgeId of the cell.
  virtual vlCell *GetEdge(int edgeId) = 0;

  // Description:
  // Return the face cell from the faceId of the cell.
  virtual vlCell *GetFace(int faceId) = 0;

  // Description:
  // Given a point x[3] return inside(=1) or outside(=0) cell; evaluate 
  // parametric coordinates, sub-cell id (!=0 only if cell is composite),
  // distance squared  of point x[3] to cell (in particular, the sub-cell 
  // indicated), and interpolation weights in cell.
  virtual int EvaluatePosition(float x[3], float closestPoint[3], 
                               int& subId, float pcoords[3], 
                               float& dist2, float weights[MAX_CELL_SIZE]) = 0;

  // Description:
  // Determine global coordinate from subId and parametric coordinates
  virtual void EvaluateLocation(int& subId, float pcoords[3], 
                                float x[3], float weights[MAX_CELL_SIZE]) = 0;

  // Description:
  // Generate contouring primitives.
  virtual void Contour(float value, vlFloatScalars *cellScalars, 
                       vlFloatPoints *points, vlCellArray *verts, 
                       vlCellArray *lines, vlCellArray *polys, 
                       vlFloatScalars *scalars) = 0;

  // Description:
  // Intersect with a ray. Return parametric coordinates (both line and cell)
  // and global intersection coordinates given ray definition and tolerance. 
  // The method returns non-zero value if intersection occurs.
  virtual int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                                float x[3], float pcoords[3], int& subId) = 0;

  float *GetBounds();
  float GetLength2();

  // Quick intersection of cell bounding box.  Returns != 0 for hit.
  char HitBBox(float bounds[6], float origin[3], float dir[3], 
               float coord[3], float& t);

  // left public for quick computational access
  vlFloatPoints Points;
  vlIdList PointIds;

};

#endif


