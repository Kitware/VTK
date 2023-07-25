// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLagrangeHexahedron
 * @brief   A 3D cell that represents an arbitrary order Lagrange hex
 *
 * vtkLagrangeHexahedron is a concrete implementation of vtkCell to represent a
 * 3D hexahedron using Lagrange shape functions of user specified order.
 *
 * @sa
 * vtkHexahedron
 */

#ifndef vtkLagrangeHexahedron_h
#define vtkLagrangeHexahedron_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderHexahedron.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkDoubleArray;
class vtkHexahedron;
class vtkIdList;
class vtkLagrangeCurve;
class vtkLagrangeInterpolation;
class vtkLagrangeQuadrilateral;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeHexahedron : public vtkHigherOrderHexahedron
{
public:
  static vtkLagrangeHexahedron* New();
  vtkTypeMacro(vtkLagrangeHexahedron, vtkHigherOrderHexahedron);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_HEXAHEDRON; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;
  vtkHigherOrderCurve* GetEdgeCell() override;
  vtkHigherOrderQuadrilateral* GetFaceCell() override;
  vtkHigherOrderInterpolation* GetInterpolation() override;

protected:
  vtkHexahedron* GetApproximateHex(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;
  vtkLagrangeHexahedron();
  ~vtkLagrangeHexahedron() override;

  vtkNew<vtkLagrangeQuadrilateral> FaceCell;
  vtkNew<vtkLagrangeCurve> EdgeCell;
  vtkNew<vtkLagrangeInterpolation> Interp;

private:
  vtkLagrangeHexahedron(const vtkLagrangeHexahedron&) = delete;
  void operator=(const vtkLagrangeHexahedron&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLagrangeHexahedron_h
