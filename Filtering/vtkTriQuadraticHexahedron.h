/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriQuadraticHexahedron.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTriQuadraticHexahedron - cell represents a parabolic, 27-node isoparametric hexahedron
// .SECTION Description
// vtkTriQuadraticHexahedron is a concrete implementation of vtkNonLinearCell to
// represent a three-dimensional, 27-node isoparametric triquadratic
// hexahedron. The interpolation is the standard finite element, triquadratic
// isoparametric shape function. The cell includes 8 edge nodes, 12 mid-edge nodes, 
// 6 mid-face nodes and one mid-volume node. The ordering of the 27 points defining the 
// cell is point ids (0-7,8-19, 20-25, 26)
// where point ids 0-7 are the eight corner vertices of the cube; followed by
// twelve midedge nodes (8-19); followed by 6 mid-face nodes (20-25) and the last node (26) 
// is the mid-volume node. Note that these midedge nodes correspond lie
// on the edges defined by (0,1), (1,2), (2,3), (3,0), (4,5), (5,6), (6,7),
// (7,4), (0,4), (1,5), (2,6), (3,7). The mid-surface nodes lies on the faces
// defined by (first edge nodes id's, than mid-edge nodes id's):
// (0,1,5,4;8,17,12,16), (1,2,6,5;9,18,13,17), (2,3,7,6,10,19,14,18),
// (3,0,4,7;11,16,15,19), (0,1,2,3;8,9,10,11), (4,5,6,7;12,13,14,15).
// The last point lies in the center of the cell (0,1,2,3,4,5,6,7).
//
// .SECTION See Also
// vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
// vtkQuadraticQuad vtkQuadraticPyramid vtkQuadraticWedge
// vtkBiQuadraticQuad
//
// .SECTION Thanks
// Thanks to Soeren Gebbert  who developed this class and
// integrated it into VTK 5.0.

#ifndef __vtkTriQuadraticHexahedron_h
#define __vtkTriQuadraticHexahedron_h

#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkBiQuadraticQuad;
class vtkHexahedron;

class VTK_FILTERING_EXPORT vtkTriQuadraticHexahedron : public vtkNonLinearCell
{
public:
  static vtkTriQuadraticHexahedron *New ();
  vtkTypeRevisionMacro (vtkTriQuadraticHexahedron, vtkNonLinearCell);
  void PrintSelf (ostream & os, vtkIndent indent);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions
  // of these methods.
  int GetCellType () { return VTK_TRIQUADRATIC_HEXAHEDRON; }
  int GetCellDimension () { return 3; }
  int GetNumberOfEdges () { return 12; }
  int GetNumberOfFaces () { return 6; }
  vtkCell *GetEdge (int);
  vtkCell *GetFace (int);

  int CellBoundary (int subId, double pcoords[3], vtkIdList * pts);
  void Contour (double value, vtkDataArray * cellScalars,
    vtkPointLocator * locator, vtkCellArray * verts,
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
  // Clip this triquadratic hexahedron using scalar value provided. Like
  // contouring, except that it cuts the hex to produce linear
  // tetrahedron.
  void Clip (double value, vtkDataArray * cellScalars,
       vtkPointLocator * locator, vtkCellArray * tetras,
       vtkPointData * inPd, vtkPointData * outPd,
       vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd, int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine (double p1[3], double p2[3], double tol, double &t,
    double x[3], double pcoords[3], int &subId);

  // Description:
  // Triquadratic hexahedron specific methods.
  static void InterpolationFunctions (double pcoords[3], double weights[27]);
  static void InterpolationDerivs (double pcoords[3], double derivs[71]);

  // Description:
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives.
  void JacobianInverse (double pcoords[3], double **inverse, double derivs[71]);

protected:
  vtkTriQuadraticHexahedron ();
  ~vtkTriQuadraticHexahedron ();

  vtkQuadraticEdge *Edge;
  vtkBiQuadraticQuad *Face;
  vtkHexahedron *Hex;
  vtkDoubleArray *Scalars;

private:
  vtkTriQuadraticHexahedron (const vtkTriQuadraticHexahedron &);  // Not implemented.
  void operator = (const vtkTriQuadraticHexahedron &);  // Not implemented.
};

#endif
