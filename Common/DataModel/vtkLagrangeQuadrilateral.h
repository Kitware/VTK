// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkLagrangeQuadrilateral
// .SECTION Description
// .SECTION See Also

#ifndef vtkLagrangeQuadrilateral_h
#define vtkLagrangeQuadrilateral_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderQuadrilateral.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkDoubleArray;
class vtkIdList;
class vtkLagrangeCurve;
class vtkPointData;
class vtkPoints;
class vtkQuad;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeQuadrilateral : public vtkHigherOrderQuadrilateral
{
public:
  static vtkLagrangeQuadrilateral* New();
  vtkTypeMacro(vtkLagrangeQuadrilateral, vtkHigherOrderQuadrilateral);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_QUADRILATERAL; }

  vtkCell* GetEdge(int edgeId) override;
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;
  vtkHigherOrderCurve* GetEdgeCell() override;

protected:
  // The version of GetApproximateQuad between Lagrange and Bezier is different because Bezier is
  // non-interpolatory
  vtkQuad* GetApproximateQuad(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;

  vtkLagrangeQuadrilateral();
  ~vtkLagrangeQuadrilateral() override;

  vtkNew<vtkLagrangeCurve> EdgeCell;

private:
  vtkLagrangeQuadrilateral(const vtkLagrangeQuadrilateral&) = delete;
  void operator=(const vtkLagrangeQuadrilateral&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLagrangeQuadrilateral_h
