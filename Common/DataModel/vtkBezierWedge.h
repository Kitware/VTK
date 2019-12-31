/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierWedge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBezierWedge
 * @brief   A 3D cell that represents an arbitrary order Bezier wedge
 *
 * vtkBezierWedge is a concrete implementation of vtkCell to represent a
 * 3D wedge using Bezier shape functions of user specified order.
 * A wedge consists of two triangular and three quadrilateral faces.
 * The first six points of the wedge (0-5) are the "corner" points
 * where the first three points are the base of the wedge. This wedge
 * point ordering is opposite the vtkWedge ordering though in that
 * the base of the wedge defined by the first three points (0,1,2) form
 * a triangle whose normal points inward (toward the triangular face (3,4,5)).
 * While this is opposite the vtkWedge convention it is consistent with
 * every other cell type in VTK. The first 2 parametric coordinates of the
 * Bezier wedge or for the triangular base and vary between 0 and 1. The
 * third parametric coordinate is between the two triangular faces and goes
 * from 0 to 1 as well.
 */

#ifndef vtkBezierWedge_h
#define vtkBezierWedge_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderWedge.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkWedge;
class vtkIdList;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;
class vtkBezierCurve;
class vtkBezierInterpolation;
class vtkBezierQuadrilateral;
class vtkBezierTriangle;
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkBezierWedge : public vtkHigherOrderWedge
{
public:
  static vtkBezierWedge* New();
  vtkTypeMacro(vtkBezierWedge, vtkHigherOrderWedge);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_BEZIER_WEDGE; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  void EvaluateLocationProjectedNode(
    int& subId, const vtkIdType point_id, double x[3], double* weights);
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  void SetRationalWeightsFromPointData(vtkPointData* point_data, const vtkIdType numPts);

  virtual vtkHigherOrderQuadrilateral* getBdyQuad() override;
  virtual vtkHigherOrderTriangle* getBdyTri() override;
  virtual vtkHigherOrderCurve* getEdgeCell() override;
  virtual vtkHigherOrderInterpolation* getInterp() override;

  vtkDoubleArray* GetRationalWeights();

protected:
  vtkBezierWedge();
  ~vtkBezierWedge() override;
  vtkBezierTriangle* GetTriangularFace(int iAxis, int k);
  vtkBezierQuadrilateral* GetQuadrilateralFace(int di, int dj);

  vtkNew<vtkDoubleArray> RationalWeights;
  vtkNew<vtkBezierQuadrilateral> BdyQuad;
  vtkNew<vtkBezierTriangle> BdyTri;
  vtkNew<vtkBezierCurve> BdyEdge;
  vtkNew<vtkBezierInterpolation> Interp;
  vtkNew<vtkBezierCurve> EdgeCell;

private:
  vtkBezierWedge(const vtkBezierWedge&) = delete;
  void operator=(const vtkBezierWedge&) = delete;
};

#endif // vtkBezierWedge_h
