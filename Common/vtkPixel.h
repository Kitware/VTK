/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixel.h
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
// .NAME vtkPixel - a cell that represents an orthogonal quadrilateral
// .SECTION Description
// vtkPixel is a concrete implementation of vtkCell to represent a 2D
// orthogonal quadrilateral. Unlike vtkQuad, the corners are at right angles,
// and aligned along x-y-z coordinate axes leading to large increases in 
// computational efficiency. 

#ifndef __vtkPixel_h
#define __vtkPixel_h

#include "vtkCell.h"
#include "vtkLine.h"

class VTK_COMMON_EXPORT vtkPixel : public vtkCell
{
public:
  static vtkPixel *New();
  vtkTypeRevisionMacro(vtkPixel,vtkCell);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_PIXEL;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 4;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);
  int EvaluatePosition(float x[3], float* closestPoint,
                       int& subId, float pcoords[3],
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Pixel specific methods.
  static void InterpolationFunctions(float pcoords[3], float weights[4]);
  static void InterpolationDerivs(float pcoords[3], float derivs[8]);

  
protected:
  vtkPixel();
  ~vtkPixel();

  vtkLine *Line;

private:
  vtkPixel(const vtkPixel&);  // Not implemented.
  void operator=(const vtkPixel&);  // Not implemented.
};

#endif


