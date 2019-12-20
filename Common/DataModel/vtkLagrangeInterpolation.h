/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeInterpolation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkPoints;
class vtkVector2i;
class vtkVector3d;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeInterpolation : public vtkHigherOrderInterpolation
{
public:
  static vtkLagrangeInterpolation* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkLagrangeInterpolation, vtkHigherOrderInterpolation);

  static void EvaluateShapeFunctions(const int order, const double pcoord, double* shape);
  static void EvaluateShapeAndGradient(
    const int order, const double pcoord, double* shape, double* grad);

  static int Tensor1ShapeFunctions(const int order[1], const double* pcoords, double* shape);
  static int Tensor1ShapeDerivatives(const int order[1], const double* pcoords, double* derivs);

  static int Tensor2ShapeFunctions(const int order[2], const double* pcoords, double* shape);
  static int Tensor2ShapeDerivatives(const int order[2], const double* pcoords, double* derivs);

  static int Tensor3ShapeFunctions(const int order[3], const double* pcoords, double* shape);
  static int Tensor3ShapeDerivatives(const int order[3], const double* pcoords, double* derivs);

  virtual void Tensor3EvaluateDerivative(const int order[3], const double* pcoords,
    vtkPoints* points, const double* fieldVals, int fieldDim, double* fieldDerivs) override;

  static void WedgeShapeFunctions(
    const int order[3], const vtkIdType numberOfPoints, const double* pcoords, double* shape);
  static void WedgeShapeDerivatives(
    const int order[3], const vtkIdType numberOfPoints, const double* pcoords, double* derivs);

  virtual void WedgeEvaluate(const int order[3], const vtkIdType numberOfPoints,
    const double* pcoords, double* fieldVals, int fieldDim, double* fieldAtPCoords) override;

  virtual void WedgeEvaluateDerivative(const int order[3], const double* pcoords, vtkPoints* points,
    const double* fieldVals, int fieldDim, double* fieldDerivs) override;

protected:
  vtkLagrangeInterpolation();
  ~vtkLagrangeInterpolation() override;

private:
  vtkLagrangeInterpolation(const vtkLagrangeInterpolation&) = delete;
  void operator=(const vtkLagrangeInterpolation&) = delete;
};

#endif // vtkLagrangeInterpolation_h
