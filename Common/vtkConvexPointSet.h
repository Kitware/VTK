/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvexPointSet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConvexPointSet - a 3D cell defined by a set of convex points
// .SECTION Description
// vtkConvexPointSet is a concrete implementation that represents
// a 3D cell defined by a convex set of points. Example of such cells are
// octants (from an octree).

#ifndef __vtkConvexPointSet_h
#define __vtkConvexPointSet_h

#include "vtkCell3D.h"
#include "vtkLine.h"
#include "vtkTriangle.h"

class vtkUnstructuredGrid;

class VTK_COMMON_EXPORT vtkConvexPointSet : public vtkCell3D
{
public:
  static vtkConvexPointSet *New();
  vtkTypeRevisionMacro(vtkConvexPointSet,vtkCell3D);

  // Description:
  // See vtkCell3D API for description of these methods.
  virtual void GetEdgePoints(int edgeId, int* &pts) {}
  virtual void GetFacePoints(int faceId, int* &pts) {}

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_CONVEX_POINT_SET;}
  int GetNumberOfEdges() {return 0;}
  int GetNumberOfFaces() {return 0;}
  vtkCell *GetEdge(int edgeId) 
    {return NULL;}
  vtkCell *GetFace(int faceId) 
    {return NULL;}
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(float x[3], float* closestPoint,
                       int& subId, float pcoords[3],
                       float& dist2, float *weights) 
    {return 0;}
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights) {};
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId)
    {return 0;};
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) 
    {return 0;}
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs) {};

  // Description:
  // Returns the set of points that are on the boundary of the cell that
  // are closest parametrically to the point specified.
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts)
    {return 0;};
  
  // Description:
  // Return the center of the cell in parametric coordinates.
  int GetParametricCenter(float pcoords[3]);

protected:
  vtkConvexPointSet();
  ~vtkConvexPointSet();

private:
  vtkConvexPointSet(const vtkConvexPointSet&);  // Not implemented.
  void operator=(const vtkConvexPointSet&);  // Not implemented.
};

inline int vtkConvexPointSet::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  return 0;
}

#endif



