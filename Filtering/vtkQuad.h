/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuad - a cell that represents a 2D quadrilateral
// .SECTION Description
// vtkQuad is a concrete implementation of vtkCell to represent a 2D
// quadrilateral. vtkQuad is defined by the four points (0,1,2,3) in
// counterclockwise order. vtkQuad uses the standard isoparametric
// interpolation functions for a linear quadrilateral.

#ifndef __vtkQuad_h
#define __vtkQuad_h

#include "vtkCell.h"

class vtkLine;
class vtkTriangle;
class vtkIncrementalPointLocator;

class VTK_FILTERING_EXPORT vtkQuad : public vtkCell
{
public:
  static vtkQuad *New();
  vtkTypeMacro(vtkQuad,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_QUAD;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 4;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;};
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
  // Return the center of the triangle in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Clip this quad using scalar value provided. Like contouring, except
  // that it cuts the quad to produce other quads and/or triangles.
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // @deprecated Replaced by vtkQuad::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double sf[4]);
  // Description:
  // @deprecated Replaced by vtkQuad::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[8]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double sf[4])
    {
    vtkQuad::InterpolationFunctions(pcoords,sf);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[8])
    {
    vtkQuad::InterpolationDerivs(pcoords,derivs);
    }

  // Description:
  // Return the ids of the vertices defining edge (`edgeId`).
  // Ids are related to the cell, not to the dataset.
  int *GetEdgeArray(int edgeId);

protected:
  vtkQuad();
  ~vtkQuad();

  vtkLine     *Line;
  vtkTriangle *Triangle;

private:
  vtkQuad(const vtkQuad&);  // Not implemented.
  void operator=(const vtkQuad&);  // Not implemented.
};
//----------------------------------------------------------------------------
inline int vtkQuad::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.0;
  return 0;
}


#endif


