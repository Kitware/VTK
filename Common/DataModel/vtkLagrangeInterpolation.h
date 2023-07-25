// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkLagrangeInterpolation
// .SECTION Description
// .SECTION See Also
#ifndef vtkLagrangeInterpolation_h
#define vtkLagrangeInterpolation_h

#include "vtkCommonDataModelModule.h" // For export macro.
#include "vtkHigherOrderInterpolation.h"
#include "vtkSmartPointer.h" // For API.

#include <vector> // For scratch storage.

// Define this to include support for a "complete" (21- vs 18-point) wedge.
#define VTK_21_POINT_WEDGE true

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkVector2i;
class vtkVector3d;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeInterpolation : public vtkHigherOrderInterpolation
{
public:
  static vtkLagrangeInterpolation* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkLagrangeInterpolation, vtkHigherOrderInterpolation);

  static void EvaluateShapeFunctions(int order, double pcoord, double* shape);
  static void EvaluateShapeAndGradient(int order, double pcoord, double* shape, double* grad);

  static int Tensor1ShapeFunctions(const int order[1], const double* pcoords, double* shape);
  static int Tensor1ShapeDerivatives(const int order[1], const double* pcoords, double* derivs);

  static int Tensor2ShapeFunctions(const int order[2], const double* pcoords, double* shape);
  static int Tensor2ShapeDerivatives(const int order[2], const double* pcoords, double* derivs);

  static int Tensor3ShapeFunctions(const int order[3], const double* pcoords, double* shape);
  static int Tensor3ShapeDerivatives(const int order[3], const double* pcoords, double* derivs);

  void Tensor3EvaluateDerivative(const int order[3], const double* pcoords, vtkPoints* points,
    const double* fieldVals, int fieldDim, double* fieldDerivs) override;

  static void WedgeShapeFunctions(
    const int order[3], vtkIdType numberOfPoints, const double* pcoords, double* shape);
  static void WedgeShapeDerivatives(
    const int order[3], vtkIdType numberOfPoints, const double* pcoords, double* derivs);

  void WedgeEvaluate(const int order[3], vtkIdType numberOfPoints, const double* pcoords,
    double* fieldVals, int fieldDim, double* fieldAtPCoords) override;

  void WedgeEvaluateDerivative(const int order[3], const double* pcoords, vtkPoints* points,
    const double* fieldVals, int fieldDim, double* fieldDerivs) override;

protected:
  vtkLagrangeInterpolation();
  ~vtkLagrangeInterpolation() override;

private:
  vtkLagrangeInterpolation(const vtkLagrangeInterpolation&) = delete;
  void operator=(const vtkLagrangeInterpolation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLagrangeInterpolation_h
