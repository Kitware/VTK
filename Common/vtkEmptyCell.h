/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyCell.h
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
// .NAME vtkEmptyCell - an empty cell used as a place-holder during processing
// .SECTION Description
// vtkEmptyCell is a concrete implementation of vtkCell. It is used
// during processing to represented a deleted element.

#ifndef __vtkEmptyCell_h
#define __vtkEmptyCell_h

#include "vtkCell.h"

class VTK_COMMON_EXPORT vtkEmptyCell : public vtkCell
{
public:
  static vtkEmptyCell *New();
  vtkTypeRevisionMacro(vtkEmptyCell,vtkCell);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject() {return vtkEmptyCell::New();};
  int GetCellType() {return VTK_EMPTY_CELL;};
  int GetCellDimension() {return 0;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int) {return 0;};
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts1, 
               vtkCellArray *lines, vtkCellArray *verts2, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *pts,
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

protected:
  vtkEmptyCell() {};
  ~vtkEmptyCell() {};


private:
  vtkEmptyCell(const vtkEmptyCell&);  // Not implemented.
  void operator=(const vtkEmptyCell&);  // Not implemented.
};

#endif


