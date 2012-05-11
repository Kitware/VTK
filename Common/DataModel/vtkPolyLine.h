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
// .NAME vtkPolyLine - cell represents a set of 1D lines
// .SECTION Description
// vtkPolyLine is a concrete implementation of vtkCell to represent a set
// of 1D lines.

#ifndef __vtkPolyLine_h
#define __vtkPolyLine_h

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given points and lines, compute normals to lines. These are not true
  // normals, they are "orientation" normals used by classes like vtkTubeFilte
  // that control the rotation around the line. The normals try to stay pointing
  // in the same direction as much as possible (i.e., minimal rotation).
  static int GenerateSlidingNormals(vtkPoints *, vtkCellArray *, vtkDataArray *);
  static int GenerateSlidingNormals(vtkPoints *, vtkCellArray *, vtkDataArray *,
                                    double* firstNormal);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_POLY_LINE;};
  int GetCellDimension() {return 1;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int vtkNotUsed(edgeId)) {return 0;};
  vtkCell *GetFace(int vtkNotUsed(faceId)) {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  int IsPrimaryCell() {return 0;}

  // Description:
  // Return the center of the point cloud in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double *weights);
  virtual void InterpolateDerivs(double pcoords[3], double *derivs);

protected:
  vtkPolyLine();
  ~vtkPolyLine();

  vtkLine *Line;

private:
  vtkPolyLine(const vtkPolyLine&);  // Not implemented.
  void operator=(const vtkPolyLine&);  // Not implemented.
};

#endif


