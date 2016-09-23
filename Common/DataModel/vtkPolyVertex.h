/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyVertex.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyVertex
 * @brief   cell represents a set of 0D vertices
 *
 * vtkPolyVertex is a concrete implementation of vtkCell to represent a
 * set of 3D vertices.
*/

#ifndef vtkPolyVertex_h
#define vtkPolyVertex_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class vtkVertex;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkPolyVertex : public vtkCell
{
public:
  static vtkPolyVertex *New();
  vtkTypeMacro(vtkPolyVertex,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_POLY_VERTEX;};
  int GetCellDimension() VTK_OVERRIDE {return 0;};
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
            vtkIncrementalPointLocator *locator, vtkCellArray *verts,
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
  vtkPolyVertex();
  ~vtkPolyVertex() VTK_OVERRIDE;

  vtkVertex *Vertex;

private:
  vtkPolyVertex(const vtkPolyVertex&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyVertex&) VTK_DELETE_FUNCTION;
};

#endif


