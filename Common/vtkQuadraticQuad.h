/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticQuad.h
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
// .NAME vtkQuadraticQuad - cell represents a parabolic, isoparametric quad
// .SECTION Description
// vtkQuadraticQuad is a concrete implementation of vtkNonLinearCell to
// represent a two-dimensional, isoparametric parabolic quadrilateral
// element. The interpolation is the standard finite element, quadratic
// isoparametric shape function. The cell includes a mid-edge node. The
// ordering of the eight points defining the cell are point ids (1-4,5-8) 
// where ids 1-4 define the four corner vertices of the quad; ids 5-8 define
// the midedge nodes (1,2), (2,3), (3,4), (4,1).

#ifndef __vtkQuadraticQuad_h
#define __vtkQuadraticQuad_h

#include "vtkNonLinearCell.h"

class vtkPolyData;
class vtkQuadraticEdge;
class vtkQuad;
class vtkPointData;
class vtkCellData;

class VTK_COMMON_EXPORT vtkQuadraticQuad : public vtkNonLinearCell
{
public:
  static vtkQuadraticQuad *New();
  vtkTypeRevisionMacro(vtkQuadraticQuad,vtkNonLinearCell);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions 
  // of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_QUADRATIC_QUAD;};
  int GetCellDimension() {return 2;}
  int GetNumberOfEdges() {return 4;}
  int GetNumberOfFaces() {return 0;}
  vtkCell *GetEdge(int);
  vtkCell *GetFace(int) {return 0;}

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
  // that it cuts the edge to produce linear line segments.
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);

  
  // Description:
  // Quadratic edge specific methods. 
  static void InterpolationFunctions(float pcoords[3], float weights[8]);
  static void InterpolationDerivs(float pcoords[3], float derivs[16]);

protected:
  vtkQuadraticQuad();
  ~vtkQuadraticQuad();

  vtkQuadraticEdge *Edge;
  vtkQuad          *Quad;
  vtkPointData     *PointData;
  vtkCellData      *CellData;
  vtkFloatArray    *Scalars;

  void ComputeMidQuadNode(float *weights);
  void InterpolateAttributes(vtkPointData *inPd, vtkCellData *inCd,
                             vtkIdType cellId, float *weights);

private:
  vtkQuadraticQuad(const vtkQuadraticQuad&);  // Not implemented.
  void operator=(const vtkQuadraticQuad&);  // Not implemented.
};

#endif


