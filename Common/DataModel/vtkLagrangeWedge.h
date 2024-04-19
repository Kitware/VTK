// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLagrangeWedge
 * @brief   A 3D cell that represents an arbitrary order Lagrange wedge
 *
 * vtkLagrangeWedge is a concrete implementation of vtkCell to represent a
 * 3D wedge using Lagrange shape functions of user specified order.
 * A wedge consists of two triangular and three quadrilateral faces.
 * The first six points of the wedge (0-5) are the "corner" points
 * where the first three points are the base of the wedge. This wedge
 * point ordering is opposite the vtkWedge ordering though in that
 * the base of the wedge defined by the first three points (0,1,2) form
 * a triangle whose normal points inward (toward the triangular face (3,4,5)).
 * While this is opposite the vtkWedge convention it is consistent with
 * every other cell type in VTK. The first 2 parametric coordinates of the
 * Lagrange wedge or for the triangular base and vary between 0 and 1. The
 * third parametric coordinate is between the two triangular faces and goes
 * from 0 to 1 as well.
 */

#ifndef vtkLagrangeWedge_h
#define vtkLagrangeWedge_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderWedge.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkDoubleArray;
class vtkWedge;
class vtkIdList;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;
class vtkLagrangeCurve;
class vtkLagrangeInterpolation;
class vtkLagrangeQuadrilateral;
class vtkLagrangeTriangle;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeWedge : public vtkHigherOrderWedge
{
public:
  static vtkLagrangeWedge* New();
  vtkTypeMacro(vtkLagrangeWedge, vtkHigherOrderWedge);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_WEDGE; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  vtkHigherOrderQuadrilateral* GetBoundaryQuad() override;
  vtkHigherOrderTriangle* GetBoundaryTri() override;
  vtkHigherOrderCurve* GetEdgeCell() override;
  vtkHigherOrderInterpolation* GetInterpolation() override;

protected:
  vtkLagrangeWedge();
  ~vtkLagrangeWedge() override;

  vtkNew<vtkLagrangeQuadrilateral> BdyQuad;
  vtkNew<vtkLagrangeTriangle> BdyTri;
  vtkNew<vtkLagrangeCurve> BdyEdge;
  vtkNew<vtkLagrangeInterpolation> Interp;
  vtkNew<vtkLagrangeCurve> EdgeCell;

private:
  vtkLagrangeWedge(const vtkLagrangeWedge&) = delete;
  void operator=(const vtkLagrangeWedge&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLagrangeWedge_h
