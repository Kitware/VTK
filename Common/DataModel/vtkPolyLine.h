/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyLine
 * @brief   cell represents a set of 1D lines
 *
 * vtkPolyLine is a concrete implementation of vtkCell to represent a set
 * of 1D lines.
*/

#ifndef vtkPolyLine_h
#define vtkPolyLine_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class vtkPoints;
class vtkCellArray;
class vtkLine;
class vtkDataArray;
class vtkIncrementalPointLocator;
class vtkCellData;

class VTKCOMMONDATAMODEL_EXPORT vtkPolyLine : public vtkCell
{
public:
  static vtkPolyLine *New();
  vtkTypeMacro(vtkPolyLine,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Given points and lines, compute normals to lines. These are not true
   * normals, they are "orientation" normals used by classes like vtkTubeFilter
   * that control the rotation around the line. The normals try to stay pointing
   * in the same direction as much as possible (i.e., minimal rotation) w.r.t the
   * firstNormal (computed if NULL). Always returns 1 (success).
   */
  static int GenerateSlidingNormals(vtkPoints *, vtkCellArray *, vtkDataArray *);
  static int GenerateSlidingNormals(vtkPoints *, vtkCellArray *, vtkDataArray *,
                                    double* firstNormal);
  //@}

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_POLY_LINE;};
  int GetCellDimension() VTK_OVERRIDE {return 1;};
  int GetNumberOfEdges() VTK_OVERRIDE {return 0;};
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;};
  vtkCell *GetEdge(int vtkNotUsed(edgeId)) VTK_OVERRIDE {return 0;};
  vtkCell *GetFace(int vtkNotUsed(faceId)) VTK_OVERRIDE {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;
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
  int IsPrimaryCell() VTK_OVERRIDE {return 0;}
  //@}

  /**
   * Return the center of the point cloud in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

protected:
  vtkPolyLine();
  ~vtkPolyLine() VTK_OVERRIDE;

  vtkLine *Line;

private:
  vtkPolyLine(const vtkPolyLine&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyLine&) VTK_DELETE_FUNCTION;
};

#endif


