// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkHigherOrderInterpolation
// .SECTION Description
// .SECTION See Also
#ifndef vtkHigherOrderInterpolation_h
#define vtkHigherOrderInterpolation_h

#include "vtkCommonDataModelModule.h" // For export macro.
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For API.

#include <vector> // For scratch storage.

// Define this to include support for a "complete" (21- vs 18-point) wedge.
#define VTK_21_POINT_WEDGE true

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkVector2i;
class vtkVector3d;
class vtkHigherOrderTriangle;

class VTKCOMMONDATAMODEL_EXPORT vtkHigherOrderInterpolation : public vtkObject
{
public:
  // static vtkHigherOrderInterpolation* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkHigherOrderInterpolation, vtkObject);

  static int Tensor1ShapeFunctions(const int order[1], const double* pcoords, double* shape,
    void (*function_evaluate_shape_functions)(int, double, double*));
  static int Tensor1ShapeDerivatives(const int order[1], const double* pcoords, double* derivs,
    void (*function_evaluate_shape_and_gradient)(int, double, double*, double*));

  static int Tensor2ShapeFunctions(const int order[2], const double* pcoords, double* shape,
    void (*function_evaluate_shape_functions)(int, double, double*));
  static int Tensor2ShapeDerivatives(const int order[2], const double* pcoords, double* derivs,
    void (*function_evaluate_shape_and_gradient)(int, double, double*, double*));

  static int Tensor3ShapeFunctions(const int order[3], const double* pcoords, double* shape,
    void (*function_evaluate_shape_functions)(int, double, double*));
  static int Tensor3ShapeDerivatives(const int order[3], const double* pcoords, double* derivs,
    void (*function_evaluate_shape_and_gradient)(int, double, double*, double*));

  virtual void Tensor3EvaluateDerivative(const int order[3], const double* pcoords,
    vtkPoints* points, const double* fieldVals, int fieldDim, double* fieldDerivs) = 0;

  void Tensor3EvaluateDerivative(const int order[3], const double* pcoords, vtkPoints* points,
    const double* fieldVals, int fieldDim, double* fieldDerivs,
    void (*function_evaluate_shape_and_gradient)(int, double, double*, double*));

  static void WedgeShapeFunctions(const int order[3], vtkIdType numberOfPoints,
    const double* pcoords, double* shape, vtkHigherOrderTriangle& tri,
    void (*function_evaluate_shape_functions)(int, double, double*));
  static void WedgeShapeDerivatives(const int order[3], vtkIdType numberOfPoints,
    const double* pcoords, double* derivs, vtkHigherOrderTriangle& tri,
    void (*function_evaluate_shape_and_gradient)(int, double, double*, double*));

  /**
   * Compute the inverse of the Jacobian and put the values in `inverse`. Returns
   * 1 for success and 0 for failure (i.e. couldn't invert the Jacobian).
   */
  int JacobianInverse(vtkPoints* points, const double* derivs, double** inverse);
  int JacobianInverseWedge(vtkPoints* points, const double* derivs, double** inverse);

  virtual void WedgeEvaluate(const int order[3], vtkIdType numberOfPoints, const double* pcoords,
    double* fieldVals, int fieldDim, double* fieldAtPCoords) = 0;

  void WedgeEvaluate(const int order[3], vtkIdType numberOfPoints, const double* pcoords,
    double* fieldVals, int fieldDim, double* fieldAtPCoords, vtkHigherOrderTriangle& tri,
    void (*function_evaluate_shape_functions)(int, double, double*));

  virtual void WedgeEvaluateDerivative(const int order[3], const double* pcoords, vtkPoints* points,
    const double* fieldVals, int fieldDim, double* fieldDerivs) = 0;

  void WedgeEvaluateDerivative(const int order[3], const double* pcoords, vtkPoints* points,
    const double* fieldVals, int fieldDim, double* fieldDerivs, vtkHigherOrderTriangle& tri,
    void (*function_evaluate_shape_and_gradient)(int, double, double*, double*));

  static vtkVector3d GetParametricHexCoordinates(int vertexId);
  static vtkVector2i GetPointIndicesBoundingHexEdge(int edgeId);
  static int GetVaryingParameterOfHexEdge(int edgeId);
  static vtkVector2i GetFixedParametersOfHexEdge(int edgeId);

  static const int* GetPointIndicesBoundingHexFace(int faceId) VTK_SIZEHINT(4);
  static const int* GetEdgeIndicesBoundingHexFace(int faceId) VTK_SIZEHINT(4);
  static vtkVector2i GetVaryingParametersOfHexFace(int faceId);
  static int GetFixedParameterOfHexFace(int faceId);

  static vtkVector3d GetParametricWedgeCoordinates(int vertexId);
  static vtkVector2i GetPointIndicesBoundingWedgeEdge(int edgeId);
  static int GetVaryingParameterOfWedgeEdge(int edgeId);
  static vtkVector2i GetFixedParametersOfWedgeEdge(int edgeId);

  static const int* GetPointIndicesBoundingWedgeFace(int faceId) VTK_SIZEHINT(4);
  static const int* GetEdgeIndicesBoundingWedgeFace(int faceId) VTK_SIZEHINT(4);
  static vtkVector2i GetVaryingParametersOfWedgeFace(int faceId);
  static int GetFixedParameterOfWedgeFace(int faceId);

  static void AppendCurveCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[1]);
  static void AppendQuadrilateralCollocationPoints(
    vtkSmartPointer<vtkPoints>& pts, const int order[2]);
  static void AppendHexahedronCollocationPoints(
    vtkSmartPointer<vtkPoints>& pts, const int order[3]);
  static void AppendWedgeCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[3]);

  template <int N>
  static int NumberOfIntervals(const int order[N]);

protected:
  vtkHigherOrderInterpolation();
  ~vtkHigherOrderInterpolation() override;

  void PrepareForOrder(const int order[3], vtkIdType numberOfPoints);

  std::vector<double> ShapeSpace;
  std::vector<double> DerivSpace;

private:
  vtkHigherOrderInterpolation(const vtkHigherOrderInterpolation&) = delete;
  void operator=(const vtkHigherOrderInterpolation&) = delete;
};

template <int N>
int vtkHigherOrderInterpolation::NumberOfIntervals(const int order[N])
{
  int ni = 1;
  for (int n = 0; n < N; ++n)
  {
    ni *= order[n];
  }
  return ni;
}

VTK_ABI_NAMESPACE_END
#endif // vtkHigherOrderInterpolation_h
