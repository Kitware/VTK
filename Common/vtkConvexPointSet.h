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
//
// vtkConvexPointSet uses the ordered triangulations approach 
// (vtkOrderedTriangulator) to create triangulations guaranteed to be
// compatible across shared faces.

#ifndef __vtkConvexPointSet_h
#define __vtkConvexPointSet_h

#include "vtkCell3D.h"
#include "vtkLine.h"
#include "vtkTriangle.h"

class vtkUnstructuredGrid;
class vtkCellArray;
class vtkTetra;

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
  vtkCell *GetEdge(int edgeId) {return NULL;}
  vtkCell *GetFace(int faceId) {return NULL;}

  // Description:
  // Satisfy the vtkCell API. This method contours by triangulating the
  // cell and then contouring the resulting tetrahedra.
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);

  // Description:
  // Satisfy the vtkCell API. This method contours by triangulating the
  // cell and then adding clip-edge intersection points into the
  // triangulation; extracting the clipped region.
  virtual void Clip(float value, vtkDataArray *cellScalars, 
                    vtkPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd, 
                    int insideOut);

  // Description:
  // Satisfy the vtkCell API. This method determines the subId, pcoords,
  // and weights by triangulating the convex point set, and then 
  // determining which tetrahedron the point lies in.
  int EvaluatePosition(float x[3], float* closestPoint,
                       int& subId, float pcoords[3],
                       float& dist2, float *weights);

  // Description:
  // The inverse of EvaluatePosition.
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  
  // Description:
  // Triangulates the cells and then intersects them to determine the
  // intersection point.
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);

  // Description:
  // Triangulate using methods of vtkOrderedTriangulator.
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  
  // Description:
  // Computes derivatives by triangulating and from subId and pcoords,
  // evaluating derivatives on the resulting tetrahedron.
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Returns the set of points forming a face of the triangulation of these
  // points that are on the boundary of the cell that are closest 
  // parametrically to the point specified.
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  
  // Description:
  // Return the center of the cell in parametric coordinates. 
  int GetParametricCenter(float pcoords[3]);

protected:
  vtkConvexPointSet();
  ~vtkConvexPointSet();

  vtkTetra *Tetra;
  vtkIdList *TriIds;
  vtkPoints *TriPoints;
  vtkFloatArray *TriScalars;

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



