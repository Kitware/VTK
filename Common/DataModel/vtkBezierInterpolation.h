// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkBezierInterpolation
// .SECTION Description
// .SECTION See Also
#ifndef vtkBezierInterpolation_h
#define vtkBezierInterpolation_h

#include "vtkCommonDataModelModule.h" // For export macro.
#include "vtkHigherOrderInterpolation.h"
#include "vtkSmartPointer.h" // For API.
#include "vtkVector.h"       // For FlattenSimplex

#include <vector> // For scratch storage.

// Define this to include support for a "complete" (21- vs 18-point) wedge.
#define VTK_21_POINT_WEDGE true

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkVector2i;
class vtkVector3d;

class VTKCOMMONDATAMODEL_EXPORT vtkBezierInterpolation : public vtkHigherOrderInterpolation
{
public:
  static vtkBezierInterpolation* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkBezierInterpolation, vtkHigherOrderInterpolation);

  // see Geometrically Exact and Analysis Suitable Mesh Generation Using Rational Bernstein–Bezier
  // Elements https://scholar.colorado.edu/cgi/viewcontent.cgi?article=1170&context=mcen_gradetds
  // Chapter 3, pg 25. given a dimension ( 2 triangle, 3 tetrahedron ) and the degree of the
  // simplex flatten a simplicial bezier function's coordinate to an integer
  static int FlattenSimplex(int dim, int deg, vtkVector3i coord);

  // given a dimension ( 2 triangle, 3 tetrahedron ) and the degree of the simplex,
  // unflatten a simplicial bezier function integer to a simplicial coordinate
  static vtkVector3i UnFlattenSimplex(int dim, int deg, vtkIdType flat);

  // simplicial version of deCasteljau
  static void DeCasteljauSimplex(int dim, int deg, const double* pcoords, double* weights);
  static void DeCasteljauSimplexDeriv(int dim, int deg, const double* pcoords, double* weights);

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
  vtkBezierInterpolation();
  ~vtkBezierInterpolation() override;

private:
  vtkBezierInterpolation(const vtkBezierInterpolation&) = delete;
  void operator=(const vtkBezierInterpolation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkBezierInterpolation_h
