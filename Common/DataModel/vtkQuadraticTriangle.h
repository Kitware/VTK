/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticTriangle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadraticTriangle - cell represents a parabolic, isoparametric triangle
// .SECTION Description
// vtkQuadraticTriangle is a concrete implementation of vtkNonLinearCell to
// represent a two-dimensional, 6-node, isoparametric parabolic triangle. The
// interpolation is the standard finite element, quadratic isoparametric
// shape function. The cell includes three mid-edge nodes besides the three
// triangle vertices. The ordering of the three points defining the cell is
// point ids (0-2,3-5) where id #3 is the midedge node between points
// (0,1); id #4 is the midedge node between points (1,2); and id #5 is the
// midedge node between points (2,0).

// .SECTION See Also
// vtkQuadraticEdge vtkQuadraticTetra vtkQuadraticPyramid
// vtkQuadraticQuad vtkQuadraticHexahedron vtkQuadraticWedge

#ifndef __vtkQuadraticTriangle_h
#define __vtkQuadraticTriangle_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkTriangle;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticTriangle : public vtkNonLinearCell
{
public:
  static vtkQuadraticTriangle *New();
  vtkTypeMacro(vtkQuadraticTriangle,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions
  // of these methods.
  int GetCellType() {return VTK_QUADRATIC_TRIANGLE;};
  int GetCellDimension() {return 2;}
  int GetNumberOfEdges() {return 3;}
  int GetNumberOfFaces() {return 0;}
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;}

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
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  virtual double *GetParametricCoords();

  // Description:
  // Clip this quadratic triangle using scalar value provided. Like
  // contouring, except that it cuts the triangle to produce linear
  // triangles.
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);


  // Description:
  // Return the center of the quadratic triangle in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Return the distance of the parametric coordinate provided to the
  // cell. If inside the cell, a distance of zero is returned.
  double GetParametricDistance(double pcoords[3]);

  // Description:
  // @deprecated Replaced by vtkQuadraticTriangle::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[6]);
  // Description:
  // @deprecated Replaced by vtkQuadraticTriangle::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[12]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[6])
    {
    vtkQuadraticTriangle::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[12])
    {
    vtkQuadraticTriangle::InterpolationDerivs(pcoords,derivs);
    }

protected:
  vtkQuadraticTriangle();
  ~vtkQuadraticTriangle();

  vtkQuadraticEdge *Edge;
  vtkTriangle      *Face;
  vtkDoubleArray    *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticTriangle(const vtkQuadraticTriangle&);  // Not implemented.
  void operator=(const vtkQuadraticTriangle&);  // Not implemented.
};
//----------------------------------------------------------------------------
inline int vtkQuadraticTriangle::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1./3;
  pcoords[2] = 0.0;
  return 0;
}


#endif


