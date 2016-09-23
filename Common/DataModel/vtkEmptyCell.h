/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyCell.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEmptyCell
 * @brief   an empty cell used as a place-holder during processing
 *
 * vtkEmptyCell is a concrete implementation of vtkCell. It is used
 * during processing to represented a deleted element.
*/

#ifndef vtkEmptyCell_h
#define vtkEmptyCell_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class VTKCOMMONDATAMODEL_EXPORT vtkEmptyCell : public vtkCell
{
public:
  static vtkEmptyCell *New();
  vtkTypeMacro(vtkEmptyCell,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_EMPTY_CELL;};
  int GetCellDimension() VTK_OVERRIDE {return 0;};
  int GetNumberOfEdges() VTK_OVERRIDE {return 0;};
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;};
  vtkCell *GetEdge(int)  VTK_OVERRIDE {return 0;};
  vtkCell *GetFace(int)  VTK_OVERRIDE {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts1,
               vtkCellArray *lines, vtkCellArray *verts2,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *pts,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;
  //@}

  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) VTK_OVERRIDE;
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;

protected:
  vtkEmptyCell() {}
  ~vtkEmptyCell() VTK_OVERRIDE {}

private:
  vtkEmptyCell(const vtkEmptyCell&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEmptyCell&) VTK_DELETE_FUNCTION;
};

#endif


