// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kevin Tew
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkBezierCurve
// .SECTION Description
// .SECTION See Also

#ifndef vtkBezierCurve_h
#define vtkBezierCurve_h

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
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkBezierCurve : public vtkHigherOrderCurve
{
public:
  static vtkBezierCurve* New();
  vtkTypeMacro(vtkBezierCurve, vtkHigherOrderCurve);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override { return VTK_BEZIER_CURVE; }
  void SetRationalWeightsFromPointData(vtkPointData* point_data, vtkIdType numPts);
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  vtkDoubleArray* GetRationalWeights();

protected:
  vtkLine* GetApproximateLine(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;
  vtkBezierCurve();
  ~vtkBezierCurve() override;

  vtkNew<vtkDoubleArray> RationalWeights;

private:
  vtkBezierCurve(const vtkBezierCurve&) = delete;
  void operator=(const vtkBezierCurve&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkBezierCurve_h
