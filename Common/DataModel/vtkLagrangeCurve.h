// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkLagrangeCurve
// .SECTION Description
// .SECTION See Also

#ifndef vtkLagrangeCurve_h
#define vtkLagrangeCurve_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderCurve.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkDoubleArray;
class vtkIdList;
class vtkLine;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeCurve : public vtkHigherOrderCurve
{
public:
  static vtkLagrangeCurve* New();
  vtkTypeMacro(vtkLagrangeCurve, vtkHigherOrderCurve);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_CURVE; }

  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

protected:
  vtkLine* GetApproximateLine(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;
  vtkLagrangeCurve();
  ~vtkLagrangeCurve() override;

private:
  vtkLagrangeCurve(const vtkLagrangeCurve&) = delete;
  void operator=(const vtkLagrangeCurve&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLagrangeCurve_h
