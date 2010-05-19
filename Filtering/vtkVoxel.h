/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVoxel - a cell that represents a 3D orthogonal parallelepiped
// .SECTION Description
// vtkVoxel is a concrete implementation of vtkCell to represent a 3D
// orthogonal parallelepiped. Unlike vtkHexahedron, vtkVoxel has interior
// angles of 90 degrees, and sides are parallel to coordinate axes. This
// results in large increases in computational performance.

// .SECTION See Also
// vtkConvexPointSet vtkHexahedron vtkPyramid vtkTetra vtkWedge

#ifndef __vtkVoxel_h
#define __vtkVoxel_h

#include "vtkCell3D.h"

class vtkLine;
class vtkPixel;
class vtkIncrementalPointLocator;

class VTK_FILTERING_EXPORT vtkVoxel : public vtkCell3D
{
public:
  static vtkVoxel *New();
  vtkTypeMacro(vtkVoxel,vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See vtkCell3D API for description of these methods.
  virtual void GetEdgePoints(int edgeId, int* &pts);
  virtual void GetFacePoints(int faceId, int* &pts);
  virtual double *GetParametricCoords();

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_VOXEL;}
  int GetCellDimension() {return 3;}
  int GetNumberOfEdges() {return 12;}
  int GetNumberOfFaces() {return 6;}
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);

  // Description:
  // @deprecated Replaced by vtkVoxel::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[8]);
  // Description:
  // @deprecated Replaced by vtkVoxel::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[24]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[8])
    {
    vtkVoxel::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[24])
    {
    vtkVoxel::InterpolationDerivs(pcoords,derivs);
    }

  // Description:
  // Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
  // Ids are related to the cell, not to the dataset.
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);

protected:
  vtkVoxel();
  ~vtkVoxel();

  vtkLine *Line;
  vtkPixel *Pixel;

private:
  vtkVoxel(const vtkVoxel&);  // Not implemented.
  void operator=(const vtkVoxel&);  // Not implemented.
};

#endif


