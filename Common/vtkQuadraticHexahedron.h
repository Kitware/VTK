/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticHexahedron.h
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
// .NAME vtkQuadraticHexahedron - cell represents a parabolic, 20-node isoparametric hexahedron
// .SECTION Description
// vtkQuadraticHexahedron is a concrete implementation of vtkNonLinearCell to
// represent a three-dimensional, 20-node isoparametric parabolic
// hexahedron. The interpolation is the standard finite element, quadratic
// isoparametric shape function. The cell includes a mid-edge node. The
// ordering of the twenty points defining the cell is point ids (0-7,8-19)
// where point ids 0-7 are the eight corner vertices of the cube; followed by
// twelve midedge nodes (8-19). Note that these midedge nodes correspond lie
// on the edges defined by (0,1), (1,2), (2,3), (3,0), (4,5), (5,6), (6,7),
// (7,4), (0,4), (1,5), (2,6), (3,7).

// .SECTION See Also
// vtkQuadraticEdge vtkQUadraticTriangle vtkQuadraticTetra
// vtQuadraticQuad

#ifndef __vtkQuadraticHexahedron_h
#define __vtkQuadraticHexahedron_h

#include "vtkNonLinearCell.h"

class vtkPolyData;
class vtkQuadraticEdge;
class vtkQuadraticQuad;
class vtkHexahedron;

class VTK_COMMON_EXPORT vtkQuadraticHexahedron : public vtkNonLinearCell
{
public:
  static vtkQuadraticHexahedron *New();
  vtkTypeRevisionMacro(vtkQuadraticHexahedron,vtkNonLinearCell);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions 
  // of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_QUADRATIC_HEXAHEDRON;}
  int GetCellDimension() {return 3;}
  int GetNumberOfEdges() {return 12;}
  int GetNumberOfFaces() {return 6;}
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
  // Clip this quadratic hexahedron using scalar value provided. Like 
  // contouring, except that it cuts the hex to produce linear 
  // tetrahedron.
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
  // Quadratic hexahedron specific methods. 
  static void InterpolationFunctions(float pcoords[3], float weights[3]);
  static void InterpolationDerivs(float pcoords[3], float derivs[3]);

  // Description:
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives.
  void JacobianInverse(float pcoords[3], double **inverse, float derivs[60]);

protected:
  vtkQuadraticHexahedron();
  ~vtkQuadraticHexahedron();

  vtkQuadraticEdge *Edge;
  vtkQuadraticQuad *Quad;
  vtkHexahedron    *Hex;
  vtkPointData     *PointData;
  vtkCellData      *CellData;
  vtkFloatArray    *Scalars;
  
  void Subdivide(float *weights);
  void InterpolateAttributes(vtkPointData *inPd, vtkCellData *inCd,
                             vtkIdType cellId, float *weights);

private:
  vtkQuadraticHexahedron(const vtkQuadraticHexahedron&);  // Not implemented.
  void operator=(const vtkQuadraticHexahedron&);  // Not implemented.
};

#endif


