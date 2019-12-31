/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierInterpolation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkBezierInterpolation.h"
#include <array>
#include <functional>

#include "vtkBezierTriangle.h"
#include "vtkBezierWedge.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include <numeric> // std::accumulate

vtkStandardNewMacro(vtkBezierInterpolation);

vtkBezierInterpolation::vtkBezierInterpolation()
  : vtkHigherOrderInterpolation()
{
}

vtkBezierInterpolation::~vtkBezierInterpolation() = default;

void vtkBezierInterpolation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

static constexpr vtkIdType binomials[]{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 4, 6, 4, 1, 0, 0, 0, 0,
  0, 0, 1, 5, 10, 10, 5, 1, 0, 0, 0, 0, 0, 1, 6, 15, 20, 15, 6, 1, 0, 0, 0, 0, 1, 7, 21, 35, 35, 21,
  7, 1, 0, 0, 0, 1, 8, 28, 56, 70, 56, 28, 8, 1, 0, 0, 1, 9, 36, 84, 126, 126, 84, 36, 9, 1, 0, 1,
  10, 45, 120, 210, 252, 210, 120, 45, 10, 1 };

static vtkIdType BinomialCoefficient(const int n, int k)
{
  if (n <= 10)
  {
    return binomials[(n * 11) + k];
  }
  else if ((k < 0) || (k > n))
  {
    return 0;
  }
  else
  {
    if (k > n - k)
      k = n - k;
    int num = 1;
    int den = 1;

    for (int i = 1; i <= k; ++i)
    {
      num *= n - (k - i);
      den *= i;
    }
    // This division should always result in an integer.
    return num / den;
  }
}

static vtkIdType NumberOfSimplexFunctions(const int dim, const int deg)
{
  return BinomialCoefficient(dim + deg, dim);
}

static vtkVector3i unflattenTri(const int deg, const vtkIdType flat)
{
  int d = deg;
  int j = 0;
  int row_end = d;
  while (flat > row_end && j < d)
  {
    ++j;
    row_end += d - j + 1;
  }
  const int row_start = row_end - (d - j);
  const int i = flat - row_start;
  return { i, j, d - i - j };
}

static vtkVector3i unflattenTetrahedron(const int deg, const vtkIdType flat)
{
  int n_before_this_level = 0;
  int level = 0;
  for (; level < deg; ++level)
  {
    const int n_on_this_level = NumberOfSimplexFunctions(2, deg - level);
    if (n_before_this_level + n_on_this_level > flat)
    {
      break;
    }
    else
    {
      n_before_this_level += n_on_this_level;
    }
  }
  // cout << "deg " << deg << " level " << level << " flat " << flat <<  " deg - level " << ( deg -
  // level ) << " f  lat - n_before_this_level " << ( flat - n_before_this_level ) << std::endl;
  const auto cv_tri = unflattenTri(deg - level, flat - n_before_this_level);
  return { cv_tri[0], cv_tri[1], level };
}

int vtkBezierInterpolation::flattenSimplex(const int dim, const int deg, const vtkVector3i coord)
{
  switch (dim)
  {
    case 2:
    {
      return ((deg + 1) * (deg + 2) - (deg + 1 - coord[1]) * (deg + 2 - coord[1])) / 2 + coord[0];
    }
    case 3:
    {
      int num_before_level = 0;
      for (int i = 0; i < coord[2]; ++i)
      {
        num_before_level += NumberOfSimplexFunctions(2, deg - i);
      }
      return num_before_level + flattenSimplex(2, deg - coord[2], coord);
    }
    default:
      throw "flattenSimplex unsupported dim";
  }
}

vtkVector3i vtkBezierInterpolation::unflattenSimplex(
  const int dim, const int deg, const vtkIdType flat)
{
  switch (dim)
  {
    case 2:
      return unflattenTri(deg, flat);
    case 3:
      return unflattenTetrahedron(deg, flat);
    default:
      throw "unflattenSimplex unsupported dim";
  }
}

void iterateSimplex(
  const int dim, const int deg, std::function<void(const vtkVector3i, const int)> callback)
{
  switch (dim)
  {
    case 1:
    {
      for (int i = 0, nfuncs = deg + 1; i < nfuncs; ++i)
      {
        callback({ i, 0, 0 }, i);
      }
    }
    break;
    case 2:
    {
      for (int i = 0, nfuncs = ((deg + 1) * (deg + 2) / 2); i < nfuncs; ++i)
      {
        callback(vtkBezierInterpolation::unflattenSimplex(2, deg, i), i);
      }
    }
    break;
    case 3:
    {
      for (int i = 0, nfuncs = ((deg + 1) * (deg + 2) * (deg + 3) / 6); i < nfuncs; ++i)
      {
        callback(vtkBezierInterpolation::unflattenSimplex(3, deg, i), i);
      }
    }
    break;
  }
}

// FIXME this could be greatly optimized
void vtkBezierInterpolation::deCasteljauSimplex(
  const int dim, const int deg, const double pcoords[3], double* weights)
{
  const int basis_func_n = NumberOfSimplexFunctions(dim, deg);

  const std::array<double, 4> linear_basis = (dim == 2)
    ? std::array<double, 4>{ 1 - pcoords[0] - pcoords[1], pcoords[0], pcoords[1], 0 }
    : std::array<double, 4>{ 1 - pcoords[0] - pcoords[1] - pcoords[2], pcoords[0], pcoords[1],
        pcoords[2] };
  const int lin_degree = 1;
  const int sub_degree_length_max = NumberOfSimplexFunctions(dim, deg - 1);
  const int shape_func_length = NumberOfSimplexFunctions(dim, lin_degree);

  std::vector<double> coeffs(basis_func_n);
  std::vector<double> sub_coeffs(sub_degree_length_max);
  std::vector<double> shape_funcs(shape_func_length);

  for (int bi = 0; bi < basis_func_n; ++bi)
  {
    std::fill(coeffs.begin(), coeffs.end(), 0.0);
    coeffs[bi] = 1.0;

    for (int d = deg; d > 0; --d)
    {
      const int sub_degree = d - 1;
      const int sub_degree_length = NumberOfSimplexFunctions(dim, sub_degree);
      iterateSimplex(dim, sub_degree, [&](const vtkVector3i sub_degree_coord, const int sub_index) {
        iterateSimplex(
          dim, lin_degree, [&](const vtkVector3i lin_degree_coord, const int lin_index) {
            const vtkVector3i one_higher_coord = { sub_degree_coord[0] + lin_degree_coord[0],
              sub_degree_coord[1] + lin_degree_coord[1],
              sub_degree_coord[2] + lin_degree_coord[2] };
            const int idx = flattenSimplex(dim, sub_degree + 1, one_higher_coord);
            shape_funcs[lin_index] = coeffs[idx] * linear_basis[lin_index];
          });
        sub_coeffs[sub_index] = std::accumulate(shape_funcs.begin(), shape_funcs.end(), 0.);
      });
      for (int i = 0; i < sub_degree_length; ++i)
      {
        coeffs[i] = sub_coeffs[i];
      }
    }
    weights[bi] = coeffs[0];
  }
}

void vtkBezierInterpolation::deCasteljauSimplexDeriv(
  const int dim, const int deg, const double pcoords[3], double* weights)
{
  const int num_funcs = NumberOfSimplexFunctions(dim, deg - 1);
  std::vector<double> evals(num_funcs);
  deCasteljauSimplex(dim, deg - 1, pcoords, &evals[0]);
  for (int idim = 0; idim < dim; ++idim)
  {
    for (int ifunc = 0; ifunc < num_funcs; ++ifunc)
    {
      const vtkVector3i coord = unflattenSimplex(dim, deg - 1, ifunc);
      vtkVector3i next_coord = coord;
      next_coord[idim] += 1;

      const int flat_coord = flattenSimplex(dim, deg, coord);
      const int flat_next_coord = flattenSimplex(dim, deg, next_coord);
      weights[(num_funcs * idim) + ifunc] = deg * (evals[flat_next_coord] - evals[flat_coord]);
    }
  }
}

/// Evaluate 1-D shape functions for the given \a order at the given \a pcoord (in [0,1]).
void vtkBezierInterpolation::EvaluateShapeFunctions(
  const int order, const double pcoord, double* shape)
{
  const double u1 = (1.0 - pcoord);
  const double u2 = pcoord;

  std::vector<double> temp(order + 1);

  for (int ifunc_l = 0; ifunc_l <= order; ++ifunc_l)
  {
    std::fill(temp.begin(), temp.end(), 0.0);
    temp[order - ifunc_l] = 1.0;
    for (int ii = 1; ii <= order; ++ii)
    {
      for (int jj = order; jj >= ii; --jj)
      {
        temp[jj] = u1 * temp[jj] + u2 * temp[jj - 1];
      }
    }
    shape[ifunc_l] = temp[order];
  }
}

/// Evaluate 1-D shape functions and their derivatives for the given \a order at the given \a pcoord
/// (in [0,1]).
void vtkBezierInterpolation::EvaluateShapeAndGradient(
  const int order, const double pcoord, double* shape, double* derivs)
{
  std::vector<double> shape_deriv(order + 1);

  EvaluateShapeFunctions(order, pcoord, shape);
  EvaluateShapeFunctions(order - 1, pcoord, &shape_deriv[0]);

  for (int ifunc_l = 0; ifunc_l <= order; ++ifunc_l)
  {
    double val = 0;
    if (ifunc_l > 0)
      val += shape_deriv[ifunc_l - 1];
    if (ifunc_l < order)
      val -= shape_deriv[ifunc_l];
    derivs[ifunc_l] = val * order;
  }
}

int vtkBezierInterpolation::Tensor1ShapeFunctions(
  const int order[1], const double* pcoords, double* shape)
{
  return vtkHigherOrderInterpolation::Tensor1ShapeFunctions(
    order, pcoords, shape, vtkBezierInterpolation::EvaluateShapeFunctions);
}

int vtkBezierInterpolation::Tensor1ShapeDerivatives(
  const int order[1], const double* pcoords, double* derivs)
{
  return vtkHigherOrderInterpolation::Tensor1ShapeDerivatives(
    order, pcoords, derivs, vtkBezierInterpolation::EvaluateShapeAndGradient);
}

/// Quadrilateral shape function computation
int vtkBezierInterpolation::Tensor2ShapeFunctions(
  const int order[2], const double pcoords[3], double* shape)
{
  return vtkHigherOrderInterpolation::Tensor2ShapeFunctions(
    order, pcoords, shape, vtkBezierInterpolation::EvaluateShapeFunctions);
}

// Quadrilateral shape-function derivatives
int vtkBezierInterpolation::Tensor2ShapeDerivatives(
  const int order[2], const double pcoords[2], double* derivs)
{
  return vtkHigherOrderInterpolation::Tensor2ShapeDerivatives(
    order, pcoords, derivs, vtkBezierInterpolation::EvaluateShapeAndGradient);
}

/// Hexahedral shape function computation
int vtkBezierInterpolation::Tensor3ShapeFunctions(
  const int order[3], const double pcoords[3], double* shape)
{
  return vtkHigherOrderInterpolation::Tensor3ShapeFunctions(
    order, pcoords, shape, vtkBezierInterpolation::EvaluateShapeFunctions);
}

int vtkBezierInterpolation::Tensor3ShapeDerivatives(
  const int order[3], const double pcoords[3], double* derivs)
{
  return vtkHigherOrderInterpolation::Tensor3ShapeDerivatives(
    order, pcoords, derivs, vtkBezierInterpolation::EvaluateShapeAndGradient);
}

void vtkBezierInterpolation::Tensor3EvaluateDerivative(const int order[3], const double* pcoords,
  vtkPoints* points, const double* fieldVals, int fieldDim, double* fieldDerivs)
{
  this->vtkHigherOrderInterpolation::Tensor3EvaluateDerivative(order, pcoords, points, fieldVals,
    fieldDim, fieldDerivs, vtkBezierInterpolation::EvaluateShapeAndGradient);
}

//// Wedge shape function computation
void vtkBezierInterpolation::WedgeShapeFunctions(
  const int order[3], const vtkIdType numberOfPoints, const double pcoords[3], double* shape)
{
  static vtkNew<vtkBezierTriangle> tri;
  vtkHigherOrderInterpolation::WedgeShapeFunctions(
    order, numberOfPoints, pcoords, shape, *tri, vtkBezierInterpolation::EvaluateShapeFunctions);
}

/// Wedge shape-function derivative evaluation
void vtkBezierInterpolation::WedgeShapeDerivatives(
  const int order[3], const vtkIdType numberOfPoints, const double pcoords[3], double* derivs)
{
  static vtkNew<vtkBezierTriangle> tri;
  vtkHigherOrderInterpolation::WedgeShapeDerivatives(
    order, numberOfPoints, pcoords, derivs, *tri, vtkBezierInterpolation::EvaluateShapeAndGradient);
}

void vtkBezierInterpolation::WedgeEvaluate(const int order[3], const vtkIdType numberOfPoints,
  const double* pcoords, double* fieldVals, int fieldDim, double* fieldAtPCoords)
{
  static vtkNew<vtkBezierTriangle> tri;
  this->vtkHigherOrderInterpolation::WedgeEvaluate(order, numberOfPoints, pcoords, fieldVals,
    fieldDim, fieldAtPCoords, *tri, vtkBezierInterpolation::EvaluateShapeFunctions);
}

void vtkBezierInterpolation::WedgeEvaluateDerivative(const int order[3], const double* pcoords,
  vtkPoints* points, const double* fieldVals, int fieldDim, double* fieldDerivs)
{
  static vtkNew<vtkBezierTriangle> tri;
  this->vtkHigherOrderInterpolation::WedgeEvaluateDerivative(order, pcoords, points, fieldVals,
    fieldDim, fieldDerivs, *tri, vtkBezierInterpolation::EvaluateShapeAndGradient);
}
