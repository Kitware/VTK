/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticTetra.h
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
// .NAME vtkQuadraticTetra - cell represents a parabolic, 10-node isoparametric tetrahedron
// .SECTION Description
// vtkQuadraticTetra is a concrete implementation of vtkNonLinearCell to
// represent a three-dimensional, 10-node, isoparametric parabolic
// tetrahedron. The interpolation is the standard finite element, quadratic
// isoparametric shape function. The cell includes a mid-edge node on each of
// the size edges of the tetrahedron. The ordering of the ten points defining
// the cell is point ids (0-3,4-9) where ids 0-3 are the four tetra
// vertices; and point ids 4-9 are the midedge nodes between (0,1), (1,2), 
// (2,3), (3,0), (0,3), (1,3), and (2,3).
//
// .SECTION See Also
// vtkQuadraticEdge vtkQuadraticTriangle 
// vtQuadraticQuad vtkQuadraticHexahedron

#ifndef __vtkQuadraticTetra_h
#define __vtkQuadraticTetra_h

#include "vtkNonLinearCell.h"

class vtkPolyData;
class vtkQuadraticEdge;
class vtkQuadraticTriangle;
class vtkTetra;

class VTK_COMMON_EXPORT vtkQuadraticTetra : public vtkNonLinearCell
{
public:
  static vtkQuadraticTetra *New();
  vtkTypeRevisionMacro(vtkQuadraticTetra,vtkNonLinearCell);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions 
  // of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_LINE;};
  int GetCellDimension() {return 3;}
  int GetNumberOfEdges() {return 6;}
  int GetNumberOfFaces() {return 4;}
  vtkCell *GetEdge(int);
  vtkCell *GetFace(int);

  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(float x[3], float* closestPoint,
                       int& subId, float pcoords[3], 
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Clip this edge using scalar value provided. Like contouring, except
  // that it cuts the tetra to produce new tetras.
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *tetras,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);

  
  // Description:
  // Return the center of the quadratic tetra in parametric coordinates.
  int GetParametricCenter(float pcoords[3]);

  // Description:
  // Quadratic tetra specific methods. 
  static void InterpolationFunctions(float pcoords[3], float weights[3]);
  static void InterpolationDerivs(float pcoords[3], float derivs[3]);

  // Description:
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives.
  void JacobianInverse(float pcoords[3], double **inverse, float derivs[30]);

protected:
  vtkQuadraticTetra();
  ~vtkQuadraticTetra();

  vtkQuadraticEdge *Edge;
  vtkQuadraticTriangle *Face;
  vtkTetra *Region;
  vtkFloatArray *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticTetra(const vtkQuadraticTetra&);  // Not implemented.
  void operator=(const vtkQuadraticTetra&);  // Not implemented.
};

#endif


