/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticTetra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadraticTetra - cell represents a parabolic, 10-node isoparametric tetrahedron
// .SECTION Description
// vtkQuadraticTetra is a concrete implementation of vtkNonLinearCell to
// represent a three-dimensional, 10-node, isoparametric parabolic
// tetrahedron. The interpolation is the standard finite element, quadratic
// isoparametric shape function. The cell includes a mid-edge node on each of
// the size edges of the tetrahedron. The ordering of the ten points defining
// the cell is point ids (0-3,4-9) where ids 0-3 are the four tetra
// vertices; and point ids 4-9 are the midedge nodes between (0,1), (1,2),
// (2,0), (0,3), (1,3), and (2,3).
//
// .SECTION See Also
// vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticWedge
// vtkQuadraticQuad vtkQuadraticHexahedron vtkQuadraticPyramid

#ifndef __vtkQuadraticTetra_h
#define __vtkQuadraticTetra_h

#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkQuadraticTriangle;
class vtkTetra;
class vtkDoubleArray;

class VTK_FILTERING_EXPORT vtkQuadraticTetra : public vtkNonLinearCell
{
public:
  static vtkQuadraticTetra *New();
  vtkTypeMacro(vtkQuadraticTetra,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions
  // of these methods.
  int GetCellType() {return VTK_QUADRATIC_TETRA;}
  int GetCellDimension() {return 3;}
  int GetNumberOfEdges() {return 6;}
  int GetNumberOfFaces() {return 4;}
  vtkCell *GetEdge(int);
  vtkCell *GetFace(int);

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
  // Clip this edge using scalar value provided. Like contouring, except
  // that it cuts the tetra to produce new tetras.
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *tetras,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);


  // Description:
  // Return the center of the quadratic tetra in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Return the distance of the parametric coordinate provided to the
  // cell. If inside the cell, a distance of zero is returned.
  double GetParametricDistance(double pcoords[3]);

  // Description:
  // @deprecated Replaced by vtkQuadraticTetra::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[10]);
  // Description:
  // @deprecated Replaced by vtkQuadraticTetra::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[30]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[10])
    {
    vtkQuadraticTetra::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[30])
    {
    vtkQuadraticTetra::InterpolationDerivs(pcoords,derivs);
    }
  // Description:
  // Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
  // Ids are related to the cell, not to the dataset.
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);

  // Description:
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives.
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[30]);

protected:
  vtkQuadraticTetra();
  ~vtkQuadraticTetra();

  vtkQuadraticEdge *Edge;
  vtkQuadraticTriangle *Face;
  vtkTetra *Tetra;
  vtkDoubleArray *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticTetra(const vtkQuadraticTetra&);  // Not implemented.
  void operator=(const vtkQuadraticTetra&);  // Not implemented.
};

#endif


