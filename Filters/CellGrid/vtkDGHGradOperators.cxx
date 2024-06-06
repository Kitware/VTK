// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGHGradOperators.h"
#include "vtkDGCell.h"
#include "vtkDGOperatorEntry.h"

// Also include the basis codes as strings we can pass as shader code.
#include "Basis_HGrad_EdgeC1Basis.h"
#include "Basis_HGrad_EdgeC1Gradient.h"
#include "Basis_HGrad_EdgeC2Basis.h"
#include "Basis_HGrad_EdgeC2Gradient.h"
#include "Basis_HGrad_HexC1Basis.h"
#include "Basis_HGrad_HexC1Gradient.h"
#include "Basis_HGrad_HexC2Basis.h"
#include "Basis_HGrad_HexC2Gradient.h"
#include "Basis_HGrad_HexI2Basis.h"
#include "Basis_HGrad_HexI2Gradient.h"
#include "Basis_HGrad_PyrC1Basis.h"
#include "Basis_HGrad_PyrC1Gradient.h"
#include "Basis_HGrad_PyrC2Basis.h"
#include "Basis_HGrad_PyrC2Gradient.h"
#include "Basis_HGrad_PyrF2Basis.h"
#include "Basis_HGrad_PyrF2Gradient.h"
#include "Basis_HGrad_PyrI2Basis.h"
#include "Basis_HGrad_PyrI2Gradient.h"
#include "Basis_HGrad_QuadC1Basis.h"
#include "Basis_HGrad_QuadC1Gradient.h"
#include "Basis_HGrad_QuadC2Basis.h"
#include "Basis_HGrad_QuadC2Gradient.h"
#include "Basis_HGrad_TetC1Basis.h"
#include "Basis_HGrad_TetC1Gradient.h"
#include "Basis_HGrad_TetC2Basis.h"
#include "Basis_HGrad_TetC2Gradient.h"
#include "Basis_HGrad_TetF2Basis.h"
#include "Basis_HGrad_TetF2Gradient.h"
#include "Basis_HGrad_TriC1Basis.h"
#include "Basis_HGrad_TriC1Gradient.h"
#include "Basis_HGrad_TriC2Basis.h"
#include "Basis_HGrad_TriC2Gradient.h"
#include "Basis_HGrad_WdgC1Basis.h"
#include "Basis_HGrad_WdgC1Gradient.h"
#include "Basis_HGrad_WdgC2Basis.h"
#include "Basis_HGrad_WdgC2Gradient.h"
#include "Basis_HGrad_WdgF2Basis.h"
#include "Basis_HGrad_WdgF2Gradient.h"
#include "Basis_HGrad_WdgI2Basis.h"
#include "Basis_HGrad_WdgI2Gradient.h"

#include <cmath>
#include <limits>
#include <type_traits>

#define RealT double

namespace vtk
{
namespace basis
{
namespace hgrad
{
VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

inline RealT abs(RealT x)
{
  return std::fabs(x);
}

void EdgeC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/EdgeC1Basis.h"
}
void EdgeC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/EdgeC1Gradient.h"
}

void EdgeC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/EdgeC2Basis.h"
}
void EdgeC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/EdgeC2Gradient.h"
}

void HexC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/HexC1Basis.h"
}
void HexC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/HexC1Gradient.h"
}

void HexI2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/HexI2Basis.h"
}
void HexI2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/HexI2Gradient.h"
}

void HexC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/HexC2Basis.h"
}
void HexC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/HexC2Gradient.h"
}

void PyrC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/PyrC1Basis.h"
}
void PyrC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/PyrC1Gradient.h"
}

void PyrI2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/PyrI2Basis.h"
}
void PyrI2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/PyrI2Gradient.h"
}

void PyrC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/PyrC2Basis.h"
}
void PyrC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/PyrC2Gradient.h"
}

void PyrF2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/PyrF2Basis.h"
}
void PyrF2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/PyrF2Gradient.h"
}

void QuadC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/QuadC1Basis.h"
}
void QuadC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/QuadC1Gradient.h"
}

void QuadC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/QuadC2Basis.h"
}
void QuadC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/QuadC2Gradient.h"
}

void TetC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TetC1Basis.h"
}
void TetC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TetC1Gradient.h"
}

void TetC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TetC2Basis.h"
}
void TetC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TetC2Gradient.h"
}

void TetF2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TetF2Basis.h"
}
void TetF2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TetF2Gradient.h"
}

void TriC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TriC1Basis.h"
}
void TriC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TriC1Gradient.h"
}

void TriC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TriC2Basis.h"
}
void TriC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/TriC2Gradient.h"
}

void WdgC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/WdgC1Basis.h"
}
void WdgC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/WdgC1Gradient.h"
}

void WdgI2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/WdgI2Basis.h"
}
void WdgI2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/WdgI2Gradient.h"
}

void WdgC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/WdgC2Basis.h"
}
void WdgC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/WdgC2Gradient.h"
}

void WdgF2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/WdgF2Basis.h"
}
void WdgF2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  double rr = param[0];
  double ss = param[1];
  double tt = param[2];
  constexpr double eps = std::numeric_limits<RealT>::epsilon();
  (void)rr;
  (void)ss;
  (void)tt;
  (void)eps;

#include "Basis/HGrad/WdgF2Gradient.h"
}

bool RegisterOperators()
{
  auto& opMap = vtkDGCell::GetOperators();
  auto& basisMap = opMap["Basis"_token]["HGRAD"_token];
  auto& gradMap = opMap["BasisGradient"_token]["HGRAD"_token];

  // clang-format off
  // # Basis functions
  basisMap["C"_token][1]["vtkDGEdge"_token] = {  2, 1, EdgeC1Basis, Basis_HGrad_EdgeC1Basis };
  basisMap["C"_token][2]["vtkDGEdge"_token] = {  3, 1, EdgeC2Basis, Basis_HGrad_EdgeC2Basis };

  basisMap["C"_token][1]["vtkDGHex"_token]  = {  8, 1, HexC1Basis,  Basis_HGrad_HexC1Basis };
  basisMap["I"_token][2]["vtkDGHex"_token]  = { 20, 1, HexI2Basis,  Basis_HGrad_HexI2Basis };
  basisMap["C"_token][2]["vtkDGHex"_token]  = { 27, 1, HexC2Basis,  Basis_HGrad_HexC2Basis };

  basisMap["C"_token][1]["vtkDGPyr"_token]  = {  5, 1, PyrC1Basis,  Basis_HGrad_PyrC1Basis };
  basisMap["I"_token][2]["vtkDGPyr"_token]  = { 13, 1, PyrI2Basis,  Basis_HGrad_PyrI2Basis };
  basisMap["C"_token][2]["vtkDGPyr"_token]  = { 18, 1, PyrC2Basis,  Basis_HGrad_PyrC2Basis };
  basisMap["F"_token][2]["vtkDGPyr"_token]  = { 19, 1, PyrF2Basis,  Basis_HGrad_PyrF2Basis };

  basisMap["C"_token][1]["vtkDGQuad"_token] = {  4, 1, QuadC1Basis, Basis_HGrad_QuadC1Basis };
  basisMap["C"_token][2]["vtkDGQuad"_token] = {  9, 1, QuadC2Basis, Basis_HGrad_QuadC2Basis };

  basisMap["C"_token][1]["vtkDGTet"_token]  = {  4, 1, TetC1Basis,  Basis_HGrad_TetC1Basis };
  basisMap["C"_token][2]["vtkDGTet"_token]  = { 10, 1, TetC2Basis,  Basis_HGrad_TetC2Basis };
  basisMap["F"_token][2]["vtkDGTet"_token]  = { 15, 1, TetF2Basis,  Basis_HGrad_TetF2Basis };

  basisMap["C"_token][1]["vtkDGTri"_token]  = {  3, 1, TriC1Basis,  Basis_HGrad_TriC1Basis };
  basisMap["C"_token][2]["vtkDGTri"_token]  = {  6, 1, TriC2Basis,  Basis_HGrad_TriC2Basis };

  basisMap["C"_token][1]["vtkDGWdg"_token]  = {  6, 1, WdgC1Basis,  Basis_HGrad_WdgC1Basis };
  basisMap["I"_token][2]["vtkDGWdg"_token]  = { 15, 1, WdgI2Basis,  Basis_HGrad_WdgI2Basis };
  basisMap["C"_token][2]["vtkDGWdg"_token]  = { 18, 1, WdgC2Basis,  Basis_HGrad_WdgC2Basis };
  basisMap["F"_token][2]["vtkDGWdg"_token]  = { 21, 1, WdgF2Basis,  Basis_HGrad_WdgF2Basis };

  // # Gradients of basis functions
  gradMap["C"_token][1]["vtkDGEdge"_token] = {  2, 3, EdgeC1Gradient, Basis_HGrad_EdgeC1Gradient };
  gradMap["C"_token][2]["vtkDGEdge"_token] = {  3, 3, EdgeC2Gradient, Basis_HGrad_EdgeC2Gradient };

  gradMap["C"_token][1]["vtkDGHex"_token]  = {  8, 3, HexC1Gradient,  Basis_HGrad_HexC1Gradient };
  gradMap["I"_token][2]["vtkDGHex"_token]  = { 20, 3, HexI2Gradient,  Basis_HGrad_HexI2Gradient };
  gradMap["C"_token][2]["vtkDGHex"_token]  = { 27, 3, HexC2Gradient,  Basis_HGrad_HexC2Gradient };

  gradMap["C"_token][1]["vtkDGPyr"_token]  = {  5, 3, PyrC1Gradient,  Basis_HGrad_PyrC1Gradient };
  gradMap["I"_token][2]["vtkDGPyr"_token]  = { 13, 3, PyrI2Gradient,  Basis_HGrad_PyrI2Gradient };
  gradMap["C"_token][2]["vtkDGPyr"_token]  = { 18, 3, PyrC2Gradient,  Basis_HGrad_PyrC2Gradient };
  gradMap["F"_token][2]["vtkDGPyr"_token]  = { 19, 3, PyrF2Gradient,  Basis_HGrad_PyrF2Gradient };

  gradMap["C"_token][1]["vtkDGQuad"_token] = {  4, 3, QuadC1Gradient, Basis_HGrad_QuadC1Gradient };
  gradMap["C"_token][2]["vtkDGQuad"_token] = {  9, 3, QuadC2Gradient, Basis_HGrad_QuadC2Gradient };

  gradMap["C"_token][1]["vtkDGTet"_token]  = {  4, 3, TetC1Gradient,  Basis_HGrad_TetC1Gradient };
  gradMap["C"_token][2]["vtkDGTet"_token]  = { 10, 3, TetC2Gradient,  Basis_HGrad_TetC2Gradient };
  gradMap["F"_token][2]["vtkDGTet"_token]  = { 15, 3, TetF2Gradient,  Basis_HGrad_TetF2Gradient };

  gradMap["C"_token][1]["vtkDGTri"_token]  = {  3, 3, TriC1Gradient,  Basis_HGrad_TriC1Gradient };
  gradMap["C"_token][2]["vtkDGTri"_token]  = {  6, 3, TriC2Gradient,  Basis_HGrad_TriC2Gradient };

  gradMap["C"_token][1]["vtkDGWdg"_token]  = {  6, 3, WdgC1Gradient,  Basis_HGrad_WdgC1Gradient };
  gradMap["I"_token][2]["vtkDGWdg"_token]  = { 15, 3, WdgI2Gradient,  Basis_HGrad_WdgI2Gradient };
  gradMap["C"_token][2]["vtkDGWdg"_token]  = { 18, 3, WdgC2Gradient,  Basis_HGrad_WdgC2Gradient };
  gradMap["F"_token][2]["vtkDGWdg"_token]  = { 21, 3, WdgF2Gradient,  Basis_HGrad_WdgF2Gradient };

  // clang-format on
  return true;
}

VTK_ABI_NAMESPACE_END
} // namespace hgrad
} // namespace basis
} // namespace vtk
