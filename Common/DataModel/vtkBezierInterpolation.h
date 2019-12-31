/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierInterpolation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBezierInterpolation
// .SECTION Description
// .SECTION See Also
#ifndef vtkBezierInterpolation_h
#define vtkBezierInterpolation_h

#include "vtkCommonDataModelModule.h" // For export macro.
#include "vtkHigherOrderInterpolation.h"
#include "vtkSmartPointer.h" // For API.
#include "vtkVector.h"       // For flattenSimplex

#include <vector> // For scratch storage.

// Define this to include support for a "complete" (21- vs 18-point) wedge.
#define VTK_21_POINT_WEDGE true

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
  // Chapter 3, pg 25. given a dimmension ( 2 triangle, 3 tetrahedron ) and the degree of the
  // simplex flatten a simplicial bezier function's coordinate to an integer
  static int flattenSimplex(const int dim, const int deg, const vtkVector3i coord);

  // given a dimmension ( 2 triangle, 3 tetrahedron ) and the degree of the simplex,
  // unflatten a simplicial bezier function integer to a simplicial coordinate
  static vtkVector3i unflattenSimplex(const int dim, const int deg, const vtkIdType flat);

  // simplicial version of deCasteljau
  static void deCasteljauSimplex(
    const int dim, const int deg, const double* pcoords, double* weights);
  static void deCasteljauSimplexDeriv(
    const int dim, const int deg, const double* pcoords, double* weights);

  static void EvaluateShapeFunctions(int order, double pcoord, double* shape);
  static void EvaluateShapeAndGradient(int order, double pcoord, double* shape, double* grad);

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
  vtkBezierInterpolation();
  ~vtkBezierInterpolation() override;

private:
  vtkBezierInterpolation(const vtkBezierInterpolation&) = delete;
  void operator=(const vtkBezierInterpolation&) = delete;
};

#endif // vtkBezierInterpolation_h
