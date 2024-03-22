// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGConstantOperators.h"
#include "vtkDGCell.h"
#include "vtkDGOperatorEntry.h"

#include "Basis_Constant_CellC0Basis.h"
#include "Basis_Constant_CellC0Gradient.h"

#include <cmath>
#include <limits>
#include <type_traits>

#define RealT double

namespace vtk
{
namespace basis
{
namespace constant
{
VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

void CellC0Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/Constant/CellC0Basis.h"
}

void CellC0Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/Constant/CellC0Gradient.h"
}

bool RegisterOperators()
{
  auto& opMap = vtkDGCell::GetOperators();
  auto& basisMap = opMap["Basis"_token]["constant"_token];
  auto& gradMap = opMap["BasisGradient"_token]["constant"_token];

  // clang-format off
  // # Basis functions
  basisMap["C"_token][0]["vtkDGCell"_token] = {  1, 1, CellC0Basis, Basis_Constant_CellC0Basis };
  // # Gradients of basis functions
  gradMap["C"_token][0]["vtkDGCell"_token] = {  1, 3, CellC0Gradient, Basis_Constant_CellC0Gradient };

  return true;
}

VTK_ABI_NAMESPACE_END
} // namespace constant
} // namespace basis
} // namespace vtk
