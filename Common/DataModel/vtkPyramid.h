/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPyramid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPyramid - a 3D cell that represents a linear pyramid
// .SECTION Description
// vtkPyramid is a concrete implementation of vtkCell to represent a 3D
// pyramid. A pyramid consists of a rectangular base with four triangular
// faces. vtkPyramid uses the standard isoparametric shape functions for
// a linear pyramid. The pyramid is defined by the five points (0-4) where
// (0,1,2,3) is the base of the pyramid which, using the right hand rule,
// forms a quadrilaterial whose normal points in the direction of the
// pyramid apex at vertex #4.

// .SECTION See Also
// vtkConvexPointSet vtkHexahedron vtkTetra vtkVoxel vtkWedge

#ifndef vtkPyramid_h
#define vtkPyramid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkLine;
class vtkQuad;
class vtkTriangle;
class vtkUnstructuredGrid;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkPyramid : public vtkCell3D
{
public:
  static vtkPyramid *New();
  vtkTypeMacro(vtkPyramid,vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See vtkCell3D API for description of these methods.
  virtual void GetEdgePoints(int edgeId, int* &pts);
  virtual void GetFacePoints(int faceId, int* &pts);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_PYRAMID;}
  int GetCellDimension() {return 3;}
  int GetNumberOfEdges() {return 8;}
  int GetNumberOfFaces() {return 5;}
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
  virtual double *GetParametricCoords();

  // Description:
  // Return the center of the pyramid in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // @deprecated Replaced by vtkPyramid::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[5]);
  // Description:
  // @deprecated Replaced by vtkPyramid::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[15]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[5])
    {
    vtkPyramid::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[15])
    {
    vtkPyramid::InterpolationDerivs(pcoords,derivs);
    }

  int JacobianInverse(double pcoords[3], double **inverse, double derivs[15]);

  // Description:
  // Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
  // Ids are related to the cell, not to the dataset.
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);

protected:
  vtkPyramid();
  ~vtkPyramid();

  vtkLine *Line;
  vtkTriangle *Triangle;
  vtkQuad *Quad;

private:
  vtkPyramid(const vtkPyramid&);  // Not implemented.
  void operator=(const vtkPyramid&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline int vtkPyramid::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.4;
  pcoords[2] = 0.2;
  return 0;
}

#endif



