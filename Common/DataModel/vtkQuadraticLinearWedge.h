/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticLinearWedge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadraticLinearWedge - cell represents a, 12-node isoparametric wedge
// .SECTION Description
// vtkQuadraticLinearWedge is a concrete implementation of vtkNonLinearCell to
// represent a three-dimensional, 12-node isoparametric linear quadratic
// wedge. The interpolation is the standard finite element, quadratic
// isoparametric shape function in xy - layer and the linear functions in z - direction.
// The cell includes mid-edge node in the triangle edges. The
// ordering of the 12 points defining the cell is point ids (0-5,6-12)
// where point ids 0-5 are the six corner vertices of the wedge; followed by
// six midedge nodes (6-12). Note that these midedge nodes correspond lie
// on the edges defined by (0,1), (1,2), (2,0), (3,4), (4,5), (5,3).
// The Edges (0,3), (1,4), (2,5) dont have midedge nodes.
//
// .SECTION See Also
// vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
// vtkQuadraticHexahedron vtkQuadraticQuad vtkQuadraticPyramid
//
// .SECTION Thanks
// Thanks to Soeren Gebbert  who developed this class and
// integrated it into VTK 5.0.

#ifndef __vtkQuadraticLinearWedge_h
#define __vtkQuadraticLinearWedge_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkLine;
class vtkQuadraticLinearQuad;
class vtkQuadraticTriangle;
class vtkWedge;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticLinearWedge : public vtkNonLinearCell
{
public:
  static vtkQuadraticLinearWedge *New ();
  vtkTypeMacro(vtkQuadraticLinearWedge,vtkNonLinearCell);
  void PrintSelf (ostream & os, vtkIndent indent);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions
  // of these methods.
  int GetCellType () { return VTK_QUADRATIC_LINEAR_WEDGE; }
  int GetCellDimension () { return 3; }
  int GetNumberOfEdges () { return 9; }
  int GetNumberOfFaces () { return 5; }
  vtkCell *GetEdge (int edgeId);
  vtkCell *GetFace (int faceId);

  int CellBoundary (int subId, double pcoords[3], vtkIdList * pts);

  // Description:
  // The quadratic linear wege is splitted into 4 linear wedges,
  // each of them is contoured by a provided scalar value
  void Contour (double value, vtkDataArray * cellScalars,
    vtkIncrementalPointLocator * locator, vtkCellArray * verts,
    vtkCellArray * lines, vtkCellArray * polys,
    vtkPointData * inPd, vtkPointData * outPd, vtkCellData * inCd,
    vtkIdType cellId, vtkCellData * outCd);
  int EvaluatePosition (double x[3], double *closestPoint,
    int &subId, double pcoords[3], double &dist2, double *weights);
  void EvaluateLocation (int &subId, double pcoords[3], double x[3], double *weights);
  int Triangulate (int index, vtkIdList * ptIds, vtkPoints * pts);
  void Derivatives (int subId, double pcoords[3], double *values, int dim, double *derivs);
  virtual double *GetParametricCoords ();

  // Description:
  // Clip this quadratic linear wedge using scalar value provided. Like
  // contouring, except that it cuts the hex to produce linear
  // tetrahedron.
  void Clip (double value, vtkDataArray * cellScalars,
       vtkIncrementalPointLocator * locator, vtkCellArray * tetras,
       vtkPointData * inPd, vtkPointData * outPd,
       vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd, int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine (double p1[3], double p2[3], double tol, double &t,
    double x[3], double pcoords[3], int &subId);

  // Description:
  // Return the center of the quadratic linear wedge in parametric coordinates.
  int GetParametricCenter (double pcoords[3]);

  // Description:
  // @deprecated Replaced by vtkQuadraticLinearWedge::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions (double pcoords[3], double weights[15]);
  // Description:
  // @deprecated Replaced by vtkQuadraticLinearWedge::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs (double pcoords[3], double derivs[45]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions (double pcoords[3], double weights[15])
    {
    vtkQuadraticLinearWedge::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs (double pcoords[3], double derivs[45])
    {
    vtkQuadraticLinearWedge::InterpolationDerivs(pcoords,derivs);
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
  void JacobianInverse (double pcoords[3], double **inverse, double derivs[45]);

protected:
  vtkQuadraticLinearWedge ();
  ~vtkQuadraticLinearWedge ();

  vtkQuadraticEdge *QuadEdge;
  vtkLine *Edge;
  vtkQuadraticTriangle *TriangleFace;
  vtkQuadraticLinearQuad *Face;
  vtkWedge *Wedge;
  vtkDoubleArray *Scalars;  //used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticLinearWedge (const vtkQuadraticLinearWedge &);  // Not implemented.
  void operator = (const vtkQuadraticLinearWedge &);  // Not implemented.
};
//----------------------------------------------------------------------------
// Return the center of the quadratic wedge in parametric coordinates.
inline int vtkQuadraticLinearWedge::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1./3;
  pcoords[2] = 0.5;
  return 0;
}


#endif
