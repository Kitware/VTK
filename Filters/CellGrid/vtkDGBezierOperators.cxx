// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGBezierOperators.h"
#include "vtkDGCell.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDebugLeaks.h"

// Also include the basis codes as strings we can pass as shader code.
#include "Basis_Bezier_EdgeCnBasis.h"
#include "Basis_Bezier_EdgeCnGradient.h"
#include "Basis_Bezier_HexCnBasis.h"
#include "Basis_Bezier_HexCnGradient.h"
#include "Basis_Bezier_QuadCnBasis.h"
#include "Basis_Bezier_QuadCnGradient.h"
#include "Basis_Bezier_TetCnBasis.h"
#include "Basis_Bezier_TetCnGradient.h"
#include "Basis_Bezier_TriCnBasis.h"
#include "Basis_Bezier_TriCnGradient.h"
#include "Basis_Bezier_WdgCnBasis.h"
#include "Basis_Bezier_WdgCnGradient.h"

#include <cmath>
#include <limits>

#define RealT double

// Boilerplate shared by every operator function: unpack the parametric
// coordinates and provide the constants basis functions may reference.
// The `(void)` casts keep compilers quiet about the ones a given basis
// happens not to use.
#define vtkBasisHeader()                                                                           \
  double rr = param[0];                                                                            \
  double ss = param[1];                                                                            \
  double tt = param[2];                                                                            \
  constexpr double eps = std::numeric_limits<RealT>::epsilon();                                    \
  (void)rr;                                                                                        \
  (void)ss;                                                                                        \
  (void)tt;                                                                                        \
  (void)eps

// Scratch space for the arbitrary-order kernels, which cannot size their
// temporaries at compile time. The buffers persist between invocations so
// that a tight evaluation loop does not allocate; `resize` on an unchanged
// size is free. When these kernels are compiled as GLSL this macro is
// redefined to declare a fixed-size array; see
// vtkDGOperatorEntry::GetShaderString().
#define WORKSPACE(type, name, size)                                                                \
  thread_local std::vector<type> name;                                                             \
  name.resize(size)

namespace vtk
{
namespace basis
{
namespace bezier
{
VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

void EdgeCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/EdgeCnBasis.h"
}
void EdgeCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/EdgeCnGradient.h"
}

void QuadCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/QuadCnBasis.h"
}
void QuadCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/QuadCnGradient.h"
}

void HexCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/HexCnBasis.h"
}
void HexCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/HexCnGradient.h"
}

void TriCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/TriCnBasis.h"
}
void TriCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/TriCnGradient.h"
}

void TetCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/TetCnBasis.h"
}
void TetCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/TetCnGradient.h"
}

void WdgCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/WdgCnBasis.h"
}
void WdgCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/Bezier/WdgCnGradient.h"
}

bool UnregisterOperators()
{
  bool didUnregister = false;
  auto& opMap = vtkDGCell::GetOperators();
  auto basisIt = opMap.find("Basis"_token);
  if (basisIt != opMap.end())
  {
    didUnregister |= basisIt->second.erase("Bezier"_token) > 0;
  }
  auto gradientIt = opMap.find("BasisGradient"_token);
  if (gradientIt != opMap.end())
  {
    didUnregister |= gradientIt->second.erase("Bezier"_token) > 0;
  }
  return didUnregister;
}

bool RegisterOperators()
{
  auto& opMap = vtkDGCell::GetOperators();
  bool addFinalizer = opMap["Basis"_token].find("Bezier"_token) == opMap["Basis"_token].end();
  auto& basisMap = opMap["Basis"_token]["Bezier"_token];
  auto& gradMap = opMap["BasisGradient"_token]["Bezier"_token];

  // Every Bernstein-Bezier basis accepts an arbitrary order, so all of these are
  // registered under -1; there are no hand-written low-order variants to take
  // precedence. The pyramid is absent for the same reason it is absent from the
  // arbitrary-order Lagrange bases: its parameter space degenerates at the apex,
  // so a polynomial basis of arbitrary order does not exist for it.
  // clang-format off
  // # Basis functions
  basisMap["A"_token][-1]["vtkDGEdge"_token]  = { vtkDGOperatorEntry::TensorProductFunctionCount, 1, EdgeCnBasis, Basis_Bezier_EdgeCnBasis };
  basisMap["A"_token][-1]["vtkDGQuad"_token]  = { vtkDGOperatorEntry::TensorProductFunctionCount, 1, QuadCnBasis, Basis_Bezier_QuadCnBasis };
  basisMap["A"_token][-1]["vtkDGHex"_token]   = { vtkDGOperatorEntry::TensorProductFunctionCount, 1, HexCnBasis, Basis_Bezier_HexCnBasis };
  basisMap["A"_token][-1]["vtkDGTri"_token]   = { vtkDGOperatorEntry::SimplexFunctionCount, 1, TriCnBasis, Basis_Bezier_TriCnBasis };
  basisMap["A"_token][-1]["vtkDGTet"_token]   = { vtkDGOperatorEntry::SimplexFunctionCount, 1, TetCnBasis, Basis_Bezier_TetCnBasis };
  basisMap["A"_token][-1]["vtkDGWdg"_token]   = { vtkDGOperatorEntry::WedgeFunctionCount, 1, WdgCnBasis, Basis_Bezier_WdgCnBasis };

  // # Gradients of basis functions
  gradMap["A"_token][-1]["vtkDGEdge"_token]  = { vtkDGOperatorEntry::TensorProductFunctionCount, 3, EdgeCnGradient, Basis_Bezier_EdgeCnGradient };
  gradMap["A"_token][-1]["vtkDGQuad"_token]  = { vtkDGOperatorEntry::TensorProductFunctionCount, 3, QuadCnGradient, Basis_Bezier_QuadCnGradient };
  gradMap["A"_token][-1]["vtkDGHex"_token]   = { vtkDGOperatorEntry::TensorProductFunctionCount, 3, HexCnGradient, Basis_Bezier_HexCnGradient };
  gradMap["A"_token][-1]["vtkDGTri"_token]   = { vtkDGOperatorEntry::SimplexFunctionCount, 3, TriCnGradient, Basis_Bezier_TriCnGradient };
  gradMap["A"_token][-1]["vtkDGTet"_token]   = { vtkDGOperatorEntry::SimplexFunctionCount, 3, TetCnGradient, Basis_Bezier_TetCnGradient };
  gradMap["A"_token][-1]["vtkDGWdg"_token]   = { vtkDGOperatorEntry::WedgeFunctionCount, 3, WdgCnGradient, Basis_Bezier_WdgCnGradient };
  // clang-format on

  if (addFinalizer)
  {
    vtkDebugLeaks::AddFinalizer([]() { UnregisterOperators(); });
  }
  return true;
}

VTK_ABI_NAMESPACE_END
} // namespace bezier
} // namespace basis
} // namespace vtk
