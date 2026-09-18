// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGHGradOperators.h"
#include "vtkDGCell.h"
#include "vtkDGOperatorEntry.h"
#include "vtkMath.h" // For JacobiPolynomial

// Also include the basis codes as strings we can pass as shader code.
#include "Basis_HGrad_EdgeC1Basis.h"
#include "Basis_HGrad_EdgeC1Gradient.h"
#include "Basis_HGrad_EdgeC2Basis.h"
#include "Basis_HGrad_EdgeC2Gradient.h"
#include "Basis_HGrad_EdgeCnBasis.h"
#include "Basis_HGrad_EdgeCnGradient.h"
#include "Basis_HGrad_EdgeG1Basis.h"
#include "Basis_HGrad_EdgeG1Gradient.h"
#include "Basis_HGrad_EdgeG2Basis.h"
#include "Basis_HGrad_EdgeG2Gradient.h"
#include "Basis_HGrad_EdgeG3Basis.h"
#include "Basis_HGrad_EdgeG3Gradient.h"
#include "Basis_HGrad_EdgeG4Basis.h"
#include "Basis_HGrad_EdgeG4Gradient.h"
#include "Basis_HGrad_EdgeG5Basis.h"
#include "Basis_HGrad_EdgeG5Gradient.h"
#include "Basis_HGrad_HexC1Basis.h"
#include "Basis_HGrad_HexC1Gradient.h"
#include "Basis_HGrad_HexC2Basis.h"
#include "Basis_HGrad_HexC2Gradient.h"
#include "Basis_HGrad_HexCnBasis.h"
#include "Basis_HGrad_HexCnGradient.h"
#include "Basis_HGrad_HexG1Basis.h"
#include "Basis_HGrad_HexG1Gradient.h"
#include "Basis_HGrad_HexG2Basis.h"
#include "Basis_HGrad_HexG2Gradient.h"
#include "Basis_HGrad_HexGnBasis.h"
#include "Basis_HGrad_HexGnGradient.h"
#include "Basis_HGrad_HexI2Basis.h"
#include "Basis_HGrad_HexI2Gradient.h"
#include "Basis_HGrad_PyrC1Basis.h"
#include "Basis_HGrad_PyrC1Gradient.h"
#include "Basis_HGrad_PyrC2Basis.h"
#include "Basis_HGrad_PyrC2Gradient.h"
#include "Basis_HGrad_PyrF2Basis.h"
#include "Basis_HGrad_PyrF2Gradient.h"
#include "Basis_HGrad_PyrGnBasis.h"
#include "Basis_HGrad_PyrGnGradient.h"
#include "Basis_HGrad_PyrI2Basis.h"
#include "Basis_HGrad_PyrI2Gradient.h"
#include "Basis_HGrad_QuadC1Basis.h"
#include "Basis_HGrad_QuadC1Gradient.h"
#include "Basis_HGrad_QuadC2Basis.h"
#include "Basis_HGrad_QuadC2Gradient.h"
#include "Basis_HGrad_QuadCnBasis.h"
#include "Basis_HGrad_QuadCnGradient.h"
#include "Basis_HGrad_QuadG1Basis.h"
#include "Basis_HGrad_QuadG1Gradient.h"
#include "Basis_HGrad_QuadG2Basis.h"
#include "Basis_HGrad_QuadG2Gradient.h"
#include "Basis_HGrad_QuadGnBasis.h"
#include "Basis_HGrad_QuadGnGradient.h"
#include "Basis_HGrad_TetC1Basis.h"
#include "Basis_HGrad_TetC1Gradient.h"
#include "Basis_HGrad_TetC2Basis.h"
#include "Basis_HGrad_TetC2Gradient.h"
#include "Basis_HGrad_TetCnBasis.h"
#include "Basis_HGrad_TetCnGradient.h"
#include "Basis_HGrad_TetF2Basis.h"
#include "Basis_HGrad_TetF2Gradient.h"
#include "Basis_HGrad_TetGnBasis.h"
#include "Basis_HGrad_TetGnGradient.h"
#include "Basis_HGrad_TriC1Basis.h"
#include "Basis_HGrad_TriC1Gradient.h"
#include "Basis_HGrad_TriC2Basis.h"
#include "Basis_HGrad_TriC2Gradient.h"
#include "Basis_HGrad_TriCnBasis.h"
#include "Basis_HGrad_TriCnGradient.h"
#include "Basis_HGrad_TriG1Basis.h"
#include "Basis_HGrad_TriG1Gradient.h"
#include "Basis_HGrad_TriG2Basis.h"
#include "Basis_HGrad_TriG2Gradient.h"
#include "Basis_HGrad_TriG3Basis.h"
#include "Basis_HGrad_TriG3Gradient.h"
#include "Basis_HGrad_TriG4Basis.h"
#include "Basis_HGrad_TriG4Gradient.h"
#include "Basis_HGrad_TriG5Basis.h"
#include "Basis_HGrad_TriG5Gradient.h"
#include "Basis_HGrad_WdgC1Basis.h"
#include "Basis_HGrad_WdgC1Gradient.h"
#include "Basis_HGrad_WdgC2Basis.h"
#include "Basis_HGrad_WdgC2Gradient.h"
#include "Basis_HGrad_WdgCnBasis.h"
#include "Basis_HGrad_WdgCnGradient.h"
#include "Basis_HGrad_WdgF2Basis.h"
#include "Basis_HGrad_WdgF2Gradient.h"
#include "Basis_HGrad_WdgG1Basis.h"
#include "Basis_HGrad_WdgG1Gradient.h"
#include "Basis_HGrad_WdgG2Basis.h"
#include "Basis_HGrad_WdgG2Gradient.h"
#include "Basis_HGrad_WdgG3Basis.h"
#include "Basis_HGrad_WdgG3Gradient.h"
#include "Basis_HGrad_WdgG4Basis.h"
#include "Basis_HGrad_WdgG4Gradient.h"
#include "Basis_HGrad_WdgG5Basis.h"
#include "Basis_HGrad_WdgG5Gradient.h"
#include "Basis_HGrad_WdgI2Basis.h"
#include "Basis_HGrad_WdgI2Gradient.h"

#include <cmath>
#include <limits>
#include <type_traits>

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

#define vtkBasisOrderHeader(oo)                                                                    \
  vtkBasisHeader();                                                                                \
  constexpr int order = oo;

// Scratch space for the arbitrary-order kernels, which cannot size their
// temporaries at compile time. The buffers persist between invocations so
// that a tight evaluation loop does not allocate; `resize` on an unchanged
// size is free. When these kernels are compiled as GLSL this macro is
// redefined to declare a fixed-size array; see
// vtkDGOperatorEntry::GetShaderString().
#define WORKSPACE(type, name, size)                                                                \
  static thread_local std::vector<type> name;                                                      \
  name.resize(size);

#define power(x, y) std::pow(x, y)

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

/// Polynomial function used by simplicial "G" basis/gradient functions.
inline RealT jacobi(int nn, RealT alpha, RealT beta, RealT xx)
{
  return vtkMath::JacobiPolynomial(nn, alpha, beta, xx);
}

inline RealT jacobi_dx(int nn, RealT alpha, RealT beta, RealT xx)
{
  return vtkMath::JacobiPolynomialDerivative(nn, alpha, beta, xx);
}

// clang-format off
/// Gauss point locations in 1-D, used by "G" basis/gradient functions.
std::array<double, 36> gpts{
  0.,                     0.,                    0.,                    0.,                    0.,                    0.,
  -0.577350269189625731,  0.577350269189625731,  0.,                    0.,                    0.,                    0.,
  -0.774596669241483404,  0.000000000000000000,  0.774596669241483404,  0.,                    0.,                    0.,
  -0.861136311594052462, -0.339981043584856257,  0.339981043584856257,  0.861136311594052462,  0.,                    0.,
  -0.906179845938663853, -0.538469310105682997,  0.000000000000000000,  0.538469310105682997,  0.906179845938663853,  0.,
  -0.932469514203152050, -0.661209386466264482, -0.238619186083196932,  0.238619186083196932,  0.661209386466264482,  0.932469514203152050
};
// clang-format on

inline RealT gaussPoint(unsigned int order, unsigned int index)
{
  return gpts[6 * order + index];
}

// NOLINTBEGIN(readability-duplicate-include)
void EdgeC1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeC1Basis.h"
}
void EdgeC1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeC1Gradient.h"
}

void EdgeC2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeC2Basis.h"
}
void EdgeC2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeC2Gradient.h"
}

void EdgeCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeCnBasis.h"
}
void EdgeCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeCnGradient.h"
}

void EdgeG1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG1Basis.h"
}
void EdgeG1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG1Gradient.h"
}

void EdgeG2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG2Basis.h"
}
void EdgeG2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG2Gradient.h"
}

void EdgeG3Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG3Basis.h"
}
void EdgeG3Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG3Gradient.h"
}

void EdgeG4Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG4Basis.h"
}
void EdgeG4Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG4Gradient.h"
}

void EdgeG5Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG5Basis.h"
}
void EdgeG5Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG5Gradient.h"
}

void HexC1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexC1Basis.h"
}
void HexC1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexC1Gradient.h"
}

void HexI2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexI2Basis.h"
}
void HexI2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexI2Gradient.h"
}

void HexC2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexC2Basis.h"
}
void HexC2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexC2Gradient.h"
}

void HexCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexCnBasis.h"
}
void HexCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexCnGradient.h"
}

void HexG1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexG1Basis.h"
}
void HexG1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexG1Gradient.h"
}

void HexG2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexG2Basis.h"
}
void HexG2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexG2Gradient.h"
}

void HexG3Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/HexGnBasis.h"
}
void HexG3Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/HexGnGradient.h"
}

void HexG4Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/HexGnBasis.h"
}
void HexG4Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/HexGnGradient.h"
}

void HexG5Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/HexGnBasis.h"
}
void HexG5Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/HexGnGradient.h"
}

void PyrC1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrC1Basis.h"
}
void PyrC1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrC1Gradient.h"
}

void PyrI2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrI2Basis.h"
}
void PyrI2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrI2Gradient.h"
}

void PyrC2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrC2Basis.h"
}
void PyrC2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrC2Gradient.h"
}

void PyrF2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrF2Basis.h"
}
void PyrF2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrF2Gradient.h"
}

void PyrG1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(1);
#include "Basis/HGrad/PyrGnBasis.h"
}
void PyrG1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(1);
#include "Basis/HGrad/PyrGnGradient.h"
}

void PyrG2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(2);
#include "Basis/HGrad/PyrGnBasis.h"
}
void PyrG2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(2);
#include "Basis/HGrad/PyrGnGradient.h"
}

void PyrG3Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/PyrGnBasis.h"
}
void PyrG3Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/PyrGnGradient.h"
}

void PyrG4Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/PyrGnBasis.h"
}
void PyrG4Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/PyrGnGradient.h"
}

void PyrG5Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/PyrGnBasis.h"
}
void PyrG5Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/PyrGnGradient.h"
}

void QuadC1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadC1Basis.h"
}
void QuadC1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadC1Gradient.h"
}

void QuadC2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadC2Basis.h"
}
void QuadC2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadC2Gradient.h"
}

void QuadCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadCnBasis.h"
}
void QuadCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadCnGradient.h"
}

void QuadG1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadG1Basis.h"
}
void QuadG1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadG1Gradient.h"
}

void QuadG2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadG2Basis.h"
}
void QuadG2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadG2Gradient.h"
}

void QuadG3Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/QuadGnBasis.h"
}
void QuadG3Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/QuadGnGradient.h"
}

void QuadG4Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/QuadGnBasis.h"
}
void QuadG4Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/QuadGnGradient.h"
}

void QuadG5Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/QuadGnBasis.h"
}
void QuadG5Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/QuadGnGradient.h"
}

void TetC1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetC1Basis.h"
}
void TetC1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetC1Gradient.h"
}

void TetC2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetC2Basis.h"
}
void TetC2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetC2Gradient.h"
}

void TetCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetCnBasis.h"
}
void TetCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetCnGradient.h"
}

void TetF2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetF2Basis.h"
}
void TetF2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetF2Gradient.h"
}

void TetG1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(1);
#include "Basis/HGrad/TetGnBasis.h"
}
void TetG1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(1);
#include "Basis/HGrad/TetGnGradient.h"
}

void TetG2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(2);
#include "Basis/HGrad/TetGnBasis.h"
}
void TetG2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(2);
#include "Basis/HGrad/TetGnGradient.h"
}

void TetG3Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/TetGnBasis.h"
}
void TetG3Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/TetGnGradient.h"
}

void TetG4Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/TetGnBasis.h"
}
void TetG4Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/TetGnGradient.h"
}

void TetG5Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/TetGnBasis.h"
}
void TetG5Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/TetGnGradient.h"
}

void TriC1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriC1Basis.h"
}
void TriC1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriC1Gradient.h"
}

void TriC2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriC2Basis.h"
}
void TriC2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriC2Gradient.h"
}

void TriCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriCnBasis.h"
}
void TriCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriCnGradient.h"
}

void TriG1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG1Basis.h"
}
void TriG1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG1Gradient.h"
}

void TriG2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG2Basis.h"
}
void TriG2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG2Gradient.h"
}

void TriG3Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG3Basis.h"
}
void TriG3Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG3Gradient.h"
}

void TriG4Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG4Basis.h"
}
void TriG4Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG4Gradient.h"
}

void TriG5Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG5Basis.h"
}
void TriG5Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG5Gradient.h"
}

void WdgC1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgC1Basis.h"
}
void WdgC1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgC1Gradient.h"
}

void WdgI2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgI2Basis.h"
}
void WdgI2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgI2Gradient.h"
}

void WdgC2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgC2Basis.h"
}
void WdgC2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgC2Gradient.h"
}

void WdgCnBasis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgCnBasis.h"
}
void WdgCnGradient(const std::array<double, 3>& param, std::vector<double>& basisGradient,
  const std::vector<int>& order)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgCnGradient.h"
}

void WdgF2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgF2Basis.h"
}
void WdgF2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgF2Gradient.h"
}

void WdgG1Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(1);
#include "Basis/HGrad/WdgG1Basis.h"
}
void WdgG1Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(1);
#include "Basis/HGrad/WdgG1Gradient.h"
}

void WdgG2Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(2);
#include "Basis/HGrad/WdgG2Basis.h"
}
void WdgG2Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(2);
#include "Basis/HGrad/WdgG2Gradient.h"
}

void WdgG3Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/WdgG3Basis.h"
}
void WdgG3Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/WdgG3Gradient.h"
}

void WdgG4Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/WdgG4Basis.h"
}
void WdgG4Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/WdgG4Gradient.h"
}

void WdgG5Basis(
  const std::array<double, 3>& param, std::vector<double>& basis, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/WdgG5Basis.h"
}
void WdgG5Gradient(
  const std::array<double, 3>& param, std::vector<double>& basisGradient, const std::vector<int>&)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/WdgG5Gradient.h"
}
// NOLINTEND(readability-duplicate-include)

bool RegisterOperators()
{
  auto& opMap = vtkDGCell::GetOperators();
  auto& basisMap = opMap["Basis"_token]["HGRAD"_token];
  auto& gradMap = opMap["BasisGradient"_token]["HGRAD"_token];

  // clang-format off
  // # Basis functions
  basisMap["C"_token][1]["vtkDGEdge"_token] = {  2, 1, EdgeC1Basis, Basis_HGrad_EdgeC1Basis };
  basisMap["C"_token][2]["vtkDGEdge"_token] = {  3, 1, EdgeC2Basis, Basis_HGrad_EdgeC2Basis };
  basisMap["A"_token][-1]["vtkDGEdge"_token] = { vtkDGOperatorEntry::TensorProductFunctionCount, 1, EdgeCnBasis, Basis_HGrad_EdgeCnBasis };
  basisMap["G"_token][1]["vtkDGEdge"_token] = {  2, 1, EdgeG1Basis, Basis_HGrad_EdgeG1Basis };
  basisMap["G"_token][2]["vtkDGEdge"_token] = {  3, 1, EdgeG2Basis, Basis_HGrad_EdgeG2Basis };
  basisMap["G"_token][3]["vtkDGEdge"_token] = {  4, 1, EdgeG3Basis, Basis_HGrad_EdgeG3Basis };
  basisMap["G"_token][4]["vtkDGEdge"_token] = {  5, 1, EdgeG4Basis, Basis_HGrad_EdgeG4Basis };
  basisMap["G"_token][5]["vtkDGEdge"_token] = {  6, 1, EdgeG5Basis, Basis_HGrad_EdgeG5Basis };

  basisMap["C"_token][1]["vtkDGHex"_token]  = {  8, 1, HexC1Basis,  Basis_HGrad_HexC1Basis };
  basisMap["C"_token][2]["vtkDGHex"_token]  = { 27, 1, HexC2Basis,  Basis_HGrad_HexC2Basis };
  basisMap["A"_token][-1]["vtkDGHex"_token]  = { vtkDGOperatorEntry::TensorProductFunctionCount, 1, HexCnBasis,  Basis_HGrad_HexCnBasis };
  basisMap["I"_token][2]["vtkDGHex"_token]  = { 20, 1, HexI2Basis,  Basis_HGrad_HexI2Basis };
  basisMap["G"_token][1]["vtkDGHex"_token]  = {  8, 1, HexG1Basis,  Basis_HGrad_HexG1Basis };
  basisMap["G"_token][2]["vtkDGHex"_token]  = { 27, 1, HexG2Basis,  Basis_HGrad_HexG2Basis };
  basisMap["G"_token][3]["vtkDGHex"_token]  = { 64, 1, HexG3Basis,  Basis_HGrad_HexGnBasis };
  basisMap["G"_token][4]["vtkDGHex"_token]  = {125, 1, HexG4Basis,  Basis_HGrad_HexGnBasis };
  basisMap["G"_token][5]["vtkDGHex"_token]  = {216, 1, HexG5Basis,  Basis_HGrad_HexGnBasis };

  basisMap["C"_token][1]["vtkDGPyr"_token]  = {  5, 1, PyrC1Basis,  Basis_HGrad_PyrC1Basis };
  basisMap["C"_token][2]["vtkDGPyr"_token]  = { 18, 1, PyrC2Basis,  Basis_HGrad_PyrC2Basis };
  basisMap["I"_token][2]["vtkDGPyr"_token]  = { 13, 1, PyrI2Basis,  Basis_HGrad_PyrI2Basis };
  basisMap["F"_token][2]["vtkDGPyr"_token]  = { 19, 1, PyrF2Basis,  Basis_HGrad_PyrF2Basis };
  basisMap["G"_token][1]["vtkDGPyr"_token]  = {  5, 1, PyrG1Basis,  Basis_HGrad_PyrGnBasis };
  basisMap["G"_token][2]["vtkDGPyr"_token]  = { 14, 1, PyrG2Basis,  Basis_HGrad_PyrGnBasis };
  basisMap["G"_token][3]["vtkDGPyr"_token]  = { 30, 1, PyrG3Basis,  Basis_HGrad_PyrGnBasis };
  basisMap["G"_token][4]["vtkDGPyr"_token]  = { 55, 1, PyrG4Basis,  Basis_HGrad_PyrGnBasis };
  basisMap["G"_token][5]["vtkDGPyr"_token]  = { 91, 1, PyrG5Basis,  Basis_HGrad_PyrGnBasis };

  basisMap["C"_token][1]["vtkDGQuad"_token] = {  4, 1, QuadC1Basis, Basis_HGrad_QuadC1Basis };
  basisMap["C"_token][2]["vtkDGQuad"_token] = {  9, 1, QuadC2Basis, Basis_HGrad_QuadC2Basis };
  basisMap["A"_token][-1]["vtkDGQuad"_token] = { vtkDGOperatorEntry::TensorProductFunctionCount, 1, QuadCnBasis, Basis_HGrad_QuadCnBasis };
  basisMap["G"_token][1]["vtkDGQuad"_token] = {  4, 1, QuadG1Basis, Basis_HGrad_QuadG1Basis };
  basisMap["G"_token][2]["vtkDGQuad"_token] = {  9, 1, QuadG2Basis, Basis_HGrad_QuadG2Basis };
  basisMap["G"_token][3]["vtkDGQuad"_token] = { 16, 1, QuadG3Basis, Basis_HGrad_QuadGnBasis };
  basisMap["G"_token][4]["vtkDGQuad"_token] = { 25, 1, QuadG4Basis, Basis_HGrad_QuadGnBasis };
  basisMap["G"_token][5]["vtkDGQuad"_token] = { 36, 1, QuadG5Basis, Basis_HGrad_QuadGnBasis };

  basisMap["C"_token][1]["vtkDGTet"_token]  = {  4, 1, TetC1Basis,  Basis_HGrad_TetC1Basis };
  basisMap["C"_token][2]["vtkDGTet"_token]  = { 10, 1, TetC2Basis,  Basis_HGrad_TetC2Basis };
  basisMap["A"_token][-1]["vtkDGTet"_token]  = { vtkDGOperatorEntry::SimplexFunctionCount, 1, TetCnBasis,  Basis_HGrad_TetCnBasis };
  basisMap["F"_token][2]["vtkDGTet"_token]  = { 15, 1, TetF2Basis,  Basis_HGrad_TetF2Basis };
  basisMap["G"_token][1]["vtkDGTet"_token]  = {  4, 1, TetG1Basis,  Basis_HGrad_TetGnBasis };
  basisMap["G"_token][2]["vtkDGTet"_token]  = { 10, 1, TetG2Basis,  Basis_HGrad_TetGnBasis };
  basisMap["G"_token][3]["vtkDGTet"_token]  = { 20, 1, TetG3Basis,  Basis_HGrad_TetGnBasis };
  basisMap["G"_token][4]["vtkDGTet"_token]  = { 35, 1, TetG4Basis,  Basis_HGrad_TetGnBasis };
  basisMap["G"_token][5]["vtkDGTet"_token]  = { 56, 1, TetG5Basis,  Basis_HGrad_TetGnBasis };

  basisMap["C"_token][1]["vtkDGTri"_token]  = {  3, 1, TriC1Basis,  Basis_HGrad_TriC1Basis };
  basisMap["C"_token][2]["vtkDGTri"_token]  = {  6, 1, TriC2Basis,  Basis_HGrad_TriC2Basis };
  basisMap["A"_token][-1]["vtkDGTri"_token]  = { vtkDGOperatorEntry::SimplexFunctionCount, 1, TriCnBasis,  Basis_HGrad_TriCnBasis };
  basisMap["G"_token][1]["vtkDGTri"_token]  = {  3, 1, TriG1Basis,  Basis_HGrad_TriG1Basis };
  basisMap["G"_token][2]["vtkDGTri"_token]  = {  6, 1, TriG2Basis,  Basis_HGrad_TriG2Basis };
  basisMap["G"_token][3]["vtkDGTri"_token]  = { 10, 1, TriG3Basis,  Basis_HGrad_TriG3Basis };
  basisMap["G"_token][4]["vtkDGTri"_token]  = { 15, 1, TriG4Basis,  Basis_HGrad_TriG4Basis };
  basisMap["G"_token][5]["vtkDGTri"_token]  = { 21, 1, TriG5Basis,  Basis_HGrad_TriG5Basis };

  basisMap["C"_token][1]["vtkDGWdg"_token]  = {  6, 1, WdgC1Basis,  Basis_HGrad_WdgC1Basis };
  basisMap["C"_token][2]["vtkDGWdg"_token]  = { 18, 1, WdgC2Basis,  Basis_HGrad_WdgC2Basis };
  basisMap["A"_token][-1]["vtkDGWdg"_token]  = { vtkDGOperatorEntry::WedgeFunctionCount, 1, WdgCnBasis,  Basis_HGrad_WdgCnBasis };
  basisMap["I"_token][2]["vtkDGWdg"_token]  = { 15, 1, WdgI2Basis,  Basis_HGrad_WdgI2Basis };
  basisMap["F"_token][2]["vtkDGWdg"_token]  = { 21, 1, WdgF2Basis,  Basis_HGrad_WdgF2Basis };
  basisMap["G"_token][1]["vtkDGWdg"_token]  = {  6, 1, WdgG1Basis,  Basis_HGrad_WdgG1Basis };
  basisMap["G"_token][2]["vtkDGWdg"_token]  = { 18, 1, WdgG2Basis,  Basis_HGrad_WdgG2Basis };
  basisMap["G"_token][3]["vtkDGWdg"_token]  = { 40, 1, WdgG3Basis,  Basis_HGrad_WdgG3Basis };
  basisMap["G"_token][4]["vtkDGWdg"_token]  = { 75, 1, WdgG4Basis,  Basis_HGrad_WdgG4Basis };
  basisMap["G"_token][5]["vtkDGWdg"_token]  = {126, 1, WdgG5Basis,  Basis_HGrad_WdgG5Basis };

  // # Gradients of basis functions
  gradMap["C"_token][1]["vtkDGEdge"_token] = {  2, 3, EdgeC1Gradient, Basis_HGrad_EdgeC1Gradient };
  gradMap["C"_token][2]["vtkDGEdge"_token] = {  3, 3, EdgeC2Gradient, Basis_HGrad_EdgeC2Gradient };
  gradMap["A"_token][-1]["vtkDGEdge"_token] = { vtkDGOperatorEntry::TensorProductFunctionCount, 3, EdgeCnGradient, Basis_HGrad_EdgeCnGradient };
  gradMap["G"_token][1]["vtkDGEdge"_token] = {  2, 3, EdgeG1Gradient, Basis_HGrad_EdgeG1Gradient };
  gradMap["G"_token][2]["vtkDGEdge"_token] = {  3, 3, EdgeG2Gradient, Basis_HGrad_EdgeG2Gradient };
  gradMap["G"_token][3]["vtkDGEdge"_token] = {  4, 3, EdgeG3Gradient, Basis_HGrad_EdgeG3Gradient };
  gradMap["G"_token][4]["vtkDGEdge"_token] = {  5, 3, EdgeG4Gradient, Basis_HGrad_EdgeG4Gradient };
  gradMap["G"_token][5]["vtkDGEdge"_token] = {  6, 3, EdgeG5Gradient, Basis_HGrad_EdgeG5Gradient };

  gradMap["C"_token][1]["vtkDGHex"_token]  = {  8, 3, HexC1Gradient,  Basis_HGrad_HexC1Gradient };
  gradMap["C"_token][2]["vtkDGHex"_token]  = { 27, 3, HexC2Gradient,  Basis_HGrad_HexC2Gradient };
  gradMap["A"_token][-1]["vtkDGHex"_token]  = { vtkDGOperatorEntry::TensorProductFunctionCount, 3, HexCnGradient,  Basis_HGrad_HexCnGradient };
  gradMap["I"_token][2]["vtkDGHex"_token]  = { 20, 3, HexI2Gradient,  Basis_HGrad_HexI2Gradient };
  gradMap["G"_token][1]["vtkDGHex"_token]  = {  8, 3, HexG1Gradient,  Basis_HGrad_HexG1Gradient };
  gradMap["G"_token][2]["vtkDGHex"_token]  = { 27, 3, HexG2Gradient,  Basis_HGrad_HexG2Gradient };
  gradMap["G"_token][3]["vtkDGHex"_token]  = { 64, 3, HexG3Gradient,  Basis_HGrad_HexGnGradient };
  gradMap["G"_token][4]["vtkDGHex"_token]  = {125, 3, HexG4Gradient,  Basis_HGrad_HexGnGradient };
  gradMap["G"_token][5]["vtkDGHex"_token]  = {216, 3, HexG5Gradient,  Basis_HGrad_HexGnGradient };

  gradMap["C"_token][1]["vtkDGPyr"_token]  = {  5, 3, PyrC1Gradient,  Basis_HGrad_PyrC1Gradient };
  gradMap["C"_token][2]["vtkDGPyr"_token]  = { 18, 3, PyrC2Gradient,  Basis_HGrad_PyrC2Gradient };
  gradMap["I"_token][2]["vtkDGPyr"_token]  = { 13, 3, PyrI2Gradient,  Basis_HGrad_PyrI2Gradient };
  gradMap["F"_token][2]["vtkDGPyr"_token]  = { 19, 3, PyrF2Gradient,  Basis_HGrad_PyrF2Gradient };
  gradMap["G"_token][1]["vtkDGPyr"_token]  = {  5, 3, PyrG1Gradient,  Basis_HGrad_PyrGnGradient };
  gradMap["G"_token][2]["vtkDGPyr"_token]  = { 14, 3, PyrG2Gradient,  Basis_HGrad_PyrGnGradient };
  gradMap["G"_token][3]["vtkDGPyr"_token]  = { 30, 3, PyrG3Gradient,  Basis_HGrad_PyrGnGradient };
  gradMap["G"_token][4]["vtkDGPyr"_token]  = { 55, 3, PyrG4Gradient,  Basis_HGrad_PyrGnGradient };
  gradMap["G"_token][5]["vtkDGPyr"_token]  = { 91, 3, PyrG5Gradient,  Basis_HGrad_PyrGnGradient };

  gradMap["C"_token][1]["vtkDGQuad"_token] = {  4, 3, QuadC1Gradient, Basis_HGrad_QuadC1Gradient };
  gradMap["C"_token][2]["vtkDGQuad"_token] = {  9, 3, QuadC2Gradient, Basis_HGrad_QuadC2Gradient };
  gradMap["A"_token][-1]["vtkDGQuad"_token] = { vtkDGOperatorEntry::TensorProductFunctionCount, 3, QuadCnGradient, Basis_HGrad_QuadCnGradient };
  gradMap["G"_token][1]["vtkDGQuad"_token] = {  4, 3, QuadG1Gradient, Basis_HGrad_QuadG1Gradient };
  gradMap["G"_token][2]["vtkDGQuad"_token] = {  9, 3, QuadG2Gradient, Basis_HGrad_QuadG2Gradient };
  gradMap["G"_token][3]["vtkDGQuad"_token] = { 16, 3, QuadG3Gradient, Basis_HGrad_QuadGnGradient };
  gradMap["G"_token][4]["vtkDGQuad"_token] = { 25, 3, QuadG4Gradient, Basis_HGrad_QuadGnGradient };
  gradMap["G"_token][5]["vtkDGQuad"_token] = { 36, 3, QuadG5Gradient, Basis_HGrad_QuadGnGradient };

  gradMap["C"_token][1]["vtkDGTet"_token]  = {  4, 3, TetC1Gradient,  Basis_HGrad_TetC1Gradient };
  gradMap["C"_token][2]["vtkDGTet"_token]  = { 10, 3, TetC2Gradient,  Basis_HGrad_TetC2Gradient };
  gradMap["A"_token][-1]["vtkDGTet"_token]  = { vtkDGOperatorEntry::SimplexFunctionCount, 3, TetCnGradient,  Basis_HGrad_TetCnGradient };
  gradMap["F"_token][2]["vtkDGTet"_token]  = { 15, 3, TetF2Gradient,  Basis_HGrad_TetF2Gradient };
  gradMap["G"_token][1]["vtkDGTet"_token]  = {  4, 3, TetG1Gradient,  Basis_HGrad_TetGnGradient };
  gradMap["G"_token][2]["vtkDGTet"_token]  = { 10, 3, TetG2Gradient,  Basis_HGrad_TetGnGradient };
  gradMap["G"_token][3]["vtkDGTet"_token]  = { 20, 3, TetG3Gradient,  Basis_HGrad_TetGnGradient };
  gradMap["G"_token][4]["vtkDGTet"_token]  = { 35, 3, TetG4Gradient,  Basis_HGrad_TetGnGradient };
  gradMap["G"_token][5]["vtkDGTet"_token]  = { 56, 3, TetG5Gradient,  Basis_HGrad_TetGnGradient };

  gradMap["C"_token][1]["vtkDGTri"_token]  = {  3, 3, TriC1Gradient,  Basis_HGrad_TriC1Gradient };
  gradMap["C"_token][2]["vtkDGTri"_token]  = {  6, 3, TriC2Gradient,  Basis_HGrad_TriC2Gradient };
  gradMap["A"_token][-1]["vtkDGTri"_token]  = { vtkDGOperatorEntry::SimplexFunctionCount, 3, TriCnGradient,  Basis_HGrad_TriCnGradient };
  gradMap["G"_token][1]["vtkDGTri"_token]  = {  3, 3, TriG1Gradient,  Basis_HGrad_TriG1Gradient };
  gradMap["G"_token][2]["vtkDGTri"_token]  = {  6, 3, TriG2Gradient,  Basis_HGrad_TriG2Gradient };
  gradMap["G"_token][3]["vtkDGTri"_token]  = { 10, 3, TriG3Gradient,  Basis_HGrad_TriG3Gradient };
  gradMap["G"_token][4]["vtkDGTri"_token]  = { 15, 3, TriG4Gradient,  Basis_HGrad_TriG4Gradient };
  gradMap["G"_token][5]["vtkDGTri"_token]  = { 21, 3, TriG5Gradient,  Basis_HGrad_TriG5Gradient };

  gradMap["C"_token][1]["vtkDGWdg"_token]  = {  6, 3, WdgC1Gradient,  Basis_HGrad_WdgC1Gradient };
  gradMap["C"_token][2]["vtkDGWdg"_token]  = { 18, 3, WdgC2Gradient,  Basis_HGrad_WdgC2Gradient };
  gradMap["A"_token][-1]["vtkDGWdg"_token]  = { vtkDGOperatorEntry::WedgeFunctionCount, 3, WdgCnGradient,  Basis_HGrad_WdgCnGradient };
  gradMap["I"_token][2]["vtkDGWdg"_token]  = { 15, 3, WdgI2Gradient,  Basis_HGrad_WdgI2Gradient };
  gradMap["F"_token][2]["vtkDGWdg"_token]  = { 21, 3, WdgF2Gradient,  Basis_HGrad_WdgF2Gradient };
  gradMap["G"_token][1]["vtkDGWdg"_token]  = {  6, 3, WdgG1Gradient,  Basis_HGrad_WdgG1Gradient };
  gradMap["G"_token][2]["vtkDGWdg"_token]  = { 18, 3, WdgG2Gradient,  Basis_HGrad_WdgG2Gradient };
  gradMap["G"_token][3]["vtkDGWdg"_token]  = { 40, 3, WdgG3Gradient,  Basis_HGrad_WdgG3Gradient };
  gradMap["G"_token][4]["vtkDGWdg"_token]  = { 75, 3, WdgG4Gradient,  Basis_HGrad_WdgG4Gradient };
  gradMap["G"_token][5]["vtkDGWdg"_token]  = {126, 3, WdgG5Gradient,  Basis_HGrad_WdgG5Gradient };

  // clang-format on
  return true;
}

using LagrangePointMapType = std::map<vtkDGCell::Shape,
  std::unordered_map<vtkStringToken, std::unordered_map<int, std::vector<std::vector<double>>>>>;
LagrangePointMapType BuildLagrangePointMap()
{
  LagrangePointMapType lagrangePointMap;

  // clang-format off
  lagrangePointMap[vtkDGCell::Shape::Edge]["C"_token][1] = {{ -1. }, { +1. }};
  lagrangePointMap[vtkDGCell::Shape::Edge]["C"_token][2] = {{ -1. }, { +1. }, { 0. }};

  lagrangePointMap[vtkDGCell::Shape::Hexahedron]["C"_token][1] = {
    { -1., -1., -1. },
    { +1., -1., -1. },
    { +1., +1., -1. },
    { -1., +1., -1. },
    { -1., -1., +1. },
    { +1., -1., +1. },
    { +1., +1., +1. },
    { -1., +1., +1. }};
  lagrangePointMap[vtkDGCell::Shape::Hexahedron]["C"_token][2] = {
    // corners
    { -1., -1., -1. },
    { +1., -1., -1. },
    { +1., +1., -1. },
    { -1., +1., -1. },
    { -1., -1., +1. },
    { +1., -1., +1. },
    { +1., +1., +1. },
    { -1., +1., +1. },
    // t=-1 edges
    {  0., -1., -1. },
    { +1.,  0., -1. },
    {  0., +1., -1. },
    { -1.,  0., -1. },
    // t=0 edges
    { -1., -1.,  0. },
    { +1., -1.,  0. },
    { +1., +1.,  0. },
    { -1., +1.,  0. },
    // t=+1 edges
    {  0., -1., +1. },
    { +1.,  0., +1. },
    {  0., +1., +1. },
    { -1.,  0., +1. },
    // body center
    {  0.,  0.,  0. },
    // faces: -t, +t, -r, +r, -s, +s
    {  0.,  0., -1. },
    {  0.,  0., +1. },
    { -1.,  0.,  0. },
    { +1.,  0.,  0. },
    {  0., -1.,  0. },
    {  0., +1.,  0. }
  };
  lagrangePointMap[vtkDGCell::Shape::Hexahedron]["I"_token][2] = {
    // corners
    { -1., -1., -1. },
    { +1., -1., -1. },
    { +1., +1., -1. },
    { -1., +1., -1. },
    { -1., -1., +1. },
    { +1., -1., +1. },
    { +1., +1., +1. },
    { -1., +1., +1. },
    // t=-1 edges
    {  0., -1., -1. },
    { +1.,  0., -1. },
    {  0., +1., -1. },
    { -1.,  0., -1. },
    // t=0 edges
    { -1., -1.,  0. },
    { +1., -1.,  0. },
    { +1., +1.,  0. },
    { -1., +1.,  0. },
    // t=+1 edges
    {  0., -1., +1. },
    { +1.,  0., +1. },
    {  0., +1., +1. },
    { -1.,  0., +1. }
  };

  lagrangePointMap[vtkDGCell::Shape::Pyramid]["C"_token][1] = {
    { -1., -1.,  0. },
    { +1., -1.,  0. },
    { +1., +1.,  0. },
    { -1., +1.,  0. },
    {  0.,  0., +1. }
  };
  lagrangePointMap[vtkDGCell::Shape::Pyramid]["I"_token][2] = {
    // corners
    { -1., -1.,  0. },
    { +1., -1.,  0. },
    { +1., +1.,  0. },
    { -1., +1.,  0. },
    {  0.,  0., +1. },
    // mid-edge nodes of base
    {  0., -1.,  0. },
    { +1.,  0.,  0. },
    {  0., +1.,  0. },
    { -1.,  0.,  0. },
    // mid-edge nodes of center
    { -.5, -.5,  .5 },
    { +.5, -.5,  .5 },
    { +.5, +.5,  .5 },
    { -.5, +.5,  .5 },
  };
  // NB: The 18-node quadratic pyramid has no entry here. Unlike its I2 and F2
  //     siblings it is not a nodal basis: its face functions reach only 8/9 at
  //     the corresponding face centroid, so there is no set of points at which
  //     its degrees of freedom are the values taken on. Callers must fall back
  //     to sampling the basis wherever they need it.
  lagrangePointMap[vtkDGCell::Shape::Pyramid]["F"_token][2] = {
    // corners
    { +1., +1.,  0. },
    { -1., +1.,  0. },
    { -1., -1.,  0. },
    { +1., -1.,  0. },
    {  0.,  0., +1. },
    // mid-edge nodes of base
    {  0., +1.,  0. },
    { -1.,  0.,  0. },
    {  0., -1.,  0. },
    { +1.,  0.,  0. },
    // mid-edge nodes of center
    { +0.5, +0.5, 0.5 },
    { -0.5, +0.5, 0.5 },
    { -0.5, -0.5, 0.5 },
    { +0.5, -0.5, 0.5 },
    // mid-face (bottom)
    {  0.,  0.,  0. },
    // mid-face (sides)
    {   0.,  2/3., 1/3. },
    { -2/3.,  0,   1/3. },
    {   0,  -2/3., 1/3. },
    {  2/3.,  0.,  1/3. },
    // body center
    {   0.,   0.,  0.25 }
  };

  lagrangePointMap[vtkDGCell::Shape::Quadrilateral]["C"_token][1] = {
    { -1., -1. },
    { +1., -1. },
    { +1., +1. },
    { -1., +1. }
  };
  lagrangePointMap[vtkDGCell::Shape::Quadrilateral]["C"_token][2] = {
    // corners
    { -1., -1. },
    { +1., -1. },
    { +1., +1. },
    { -1., +1. },
    // mid-edge
    {  0., -1. },
    { +1.,  0. },
    {  0., +1. },
    { -1.,  0. },
    // body center
    {  0.,  0. }
  };

  lagrangePointMap[vtkDGCell::Shape::Tetrahedron]["C"_token][1] = {
    {  0.,  0.,  0. },
    { +1.,  0.,  0. },
    {  0., +1.,  0. },
    {  0.,  0., +1. } 
  };
  lagrangePointMap[vtkDGCell::Shape::Tetrahedron]["C"_token][2] = {
    // corners
    {  0.,  0.,  0. },
    { +1.,  0.,  0. },
    {  0., +1.,  0. },
    {  0.,  0., +1. },
    // mid-edge
    {  .5,  0.,  0. },
    {  .5,  .5,  0. },
    {  0.,  .5,  0. },
    {  0.,  0.,  .5 },
    {  .5,  0.,  .5 },
    {  0.,  .5,  .5 }
  };
  lagrangePointMap[vtkDGCell::Shape::Tetrahedron]["F"_token][2] = {
    // corners
    {  0.,  0.,  0. },
    { +1.,  0.,  0. },
    {  0., +1.,  0. },
    {  0.,  0., +1. },
    // mid-edge
    {  .5,  0.,  0. },
    {  .5,  .5,  0. },
    {  0.,  .5,  0. },
    {  0.,  0.,  .5 },
    {  .5,  0.,  .5 },
    {  0.,  .5,  .5 },
    // mid-face
    { 1/3., 1/3., 0.   },
    { 1/3., 0.,   1/3. },
    { 1/3., 1/3., 1/3. },
    { 0.,   1/3., 1/3. },
    // mid-body
    { 0.25, 0.25, 0.25 }
  };

  lagrangePointMap[vtkDGCell::Shape::Triangle]["C"_token][1] = {
    { 0., 0. },
    { 1., 0. },
    { 0., 1. }
  };
  lagrangePointMap[vtkDGCell::Shape::Triangle]["C"_token][2] = {
    // corners
    { 0., 0. },
    { 1., 0. },
    { 0., 1. },
    // mid-edge
    { .5, 0. },
    { .5, .5 },
    { 0., .5 }
  };

  lagrangePointMap[vtkDGCell::Shape::Wedge]["C"_token][1] = {
    {  0.,  0., -1. },
    { +1.,  0., -1. },
    {  0., +1., -1. },
    {  0.,  0., +1. },
    { +1.,  0., +1. },
    {  0., +1., +1. }
  };
  lagrangePointMap[vtkDGCell::Shape::Wedge]["I"_token][2] = {
    // corner
    {  0.,   0.,  -1. },
    { +1.,   0.,  -1. },
    {  0.,  +1.,  -1. },
    {  0.,   0.,  +1. },
    { +1.,   0.,  +1. },
    {  0.,  +1.,  +1. },
    // mid-edge, t=-1
    {  .5,   0.,  -1. },
    {  .5,   .5,  -1. },
    {  0.,   .5,  -1. },
    // mid-edge, t=0
    {  0.,   0.,   0. },
    { +1.,   0.,   0. },
    {  0.,  +1.,   0. },
    // mid-edge, t=+1
    {  .5,   0.,  +1. },
    {  .5,   .5,  +1. },
    {  0.,   .5,  +1. },
  };
  lagrangePointMap[vtkDGCell::Shape::Wedge]["C"_token][2] = {
    // corner
    {  0.,   0.,  -1. },
    { +1.,   0.,  -1. },
    {  0.,  +1.,  -1. },
    {  0.,   0.,  +1. },
    { +1.,   0.,  +1. },
    {  0.,  +1.,  +1. },
    // mid-edge, t=-1
    {  .5,   0.,  -1. },
    {  .5,   .5,  -1. },
    {  0.,   .5,  -1. },
    // mid-edge, t=0
    {  0.,   0.,   0. },
    { +1.,   0.,   0. },
    {  0.,  +1.,   0. },
    // mid-edge, t=+1
    {  .5,   0.,  +1. },
    {  .5,   .5,  +1. },
    {  0.,   .5,  +1. },
    // mid-face R/RS/S normal
    {  .5,   0.,   0. },
    {  .5,   .5,   0. },
    {  0.,   .5,   0. }
  };
  lagrangePointMap[vtkDGCell::Shape::Wedge]["F"_token][2] = {
    // corner
    {  0.,   0.,  -1. },
    { +1.,   0.,  -1. },
    {  0.,  +1.,  -1. },
    {  0.,   0.,  +1. },
    { +1.,   0.,  +1. },
    {  0.,  +1.,  +1. },
    // mid-edge, t=-1
    {  .5,   0.,  -1. },
    {  .5,   .5,  -1. },
    {  0.,   .5,  -1. },
    // mid-edge, t=+1
    {  .5,   0.,  +1. },
    {  .5,   .5,  +1. },
    {  0.,   .5,  +1. },
    // mid-edge, t=0
    {  0.,   0.,   0. },
    { +1.,   0.,   0. },
    {  0.,  +1.,   0. },
    // mid-face, T normal
    { 1/3., 1/3., -1. },
    { 1/3., 1/3., +1. },
    // mid-face R/RS/S normal
    {  .5,   0.,   0. },
    {  .5,   .5,   0. },
    {  0.,   .5,   0. },
    // body center
    { 1/3., 1/3.,  0. }
  };
  // clang-format on
  return lagrangePointMap;
}

bool FixedOrderLagrangePoints(vtkDGCell::Shape shape, vtkStringToken basis, int nominalOrder,
  std::vector<std::vector<double>>& points)
{
  // Initialized on first use. A function-local static is built exactly once even
  // if several threads arrive together, which a check for an empty file-scope
  // map would not be.
  static const LagrangePointMapType lagrangePointMap = BuildLagrangePointMap();
  points.clear();
  if (nominalOrder == 0)
  {
    // Constant cells have a single parametric point (the origin)
    // where the value for the entire cell is specified.
    std::vector<double> onePoint(vtkDGCell::GetShapeDimension(shape), 0.0);
    points.push_back(onePoint);
    return true;
  }

  // The simple cases are handled above. Now deal with non-simple
  // shape+order combinations via table lookup.
  auto shapeIt = lagrangePointMap.find(shape);
  if (shapeIt == lagrangePointMap.end())
  {
    return false;
  }
  auto basisIt = shapeIt->second.find(basis);
  if (basisIt == shapeIt->second.end())
  {
    return false;
  }
  auto orderIt = basisIt->second.find(nominalOrder);
  if (orderIt == basisIt->second.end())
  {
    return false;
  }
  points = orderIt->second;
  return true;
}

namespace
{

/// Nodes of the triangle's "G" bases, for orders 1 through 5.
///
/// The tensor-product shapes take their Gauss nodes from the 1-D \a gpts table
/// above, but a triangle's are the points of a symmetric rule and cannot be
/// built from a 1-D set. They are listed in the order the basis functions are
/// enumerated: TestCellGridLagrangePoints evaluates the basis at these points
/// and requires the identity matrix, which pins down both the points and their
/// pairing with the degrees of freedom.
const std::vector<std::vector<std::array<double, 2>>>& TriangleGaussNodes()
{
  // Initialized on first use, so construction is thread-safe.
  static const std::vector<std::vector<std::array<double, 2>>> nodes{
    // Triangle Gauss-point nodes, order 1-5 (3, 6, 10, 15, 21 points).
    {
      // order 1
      { 0.16666666666666666, 0.16666666666666666 },
      { 0.66666666666666663, 0.16666666666666669 },
      { 0.16666666666666669, 0.66666666666666663 },
    },
    {
      // order 2
      { 0.091576213509799997, 0.091576213509800011 },
      { 0.81684757298050015, 0.091576213509799956 },
      { 0.091576213509799886, 0.81684757298049993 },
      { 0.10810301816810003, 0.44594849091600036 },
      { 0.44594849091600008, 0.10810301816809999 },
      { 0.44594849091600008, 0.44594849091599997 },
    },
    {
      // order 3
      { 0.055564052669792995, 0.055564052669793078 },
      { 0.88887189466041217, 0.055564052669793543 },
      { 0.05556405266979314, 0.88887189466041316 },
      { 0.29553371173589277, 0.070255540518383647 },
      { 0.63421074774572062, 0.070255540518382578 },
      { 0.63421074774572328, 0.29553371173589488 },
      { 0.29553371173589377, 0.63421074774572328 },
      { 0.070255540518384049, 0.63421074774572062 },
      { 0.070255540518383786, 0.29553371173589266 },
      { 0.33333333333333276, 0.33333333333333165 },
    },
    {
      // order 4
      { 0.035870877695733973, 0.035870877695733883 },
      { 0.92825824460853401, 0.03587087769573391 },
      { 0.035870877695734556, 0.92825824460853468 },
      { 0.20150388188179977, 0.047312487011715879 },
      { 0.47430878777707719, 0.051382424445842879 },
      { 0.7511836311064809, 0.047312487011717537 },
      { 0.75118363110648489, 0.20150388188179871 },
      { 0.47430878777707736, 0.47430878777707941 },
      { 0.20150388188180132, 0.75118363110648489 },
      { 0.047312487011716101, 0.75118363110647346 },
      { 0.051382424445842588, 0.47430878777708124 },
      { 0.047312487011716343, 0.20150388188180202 },
      { 0.24172939576796645, 0.24172939576796684 },
      { 0.51654120846406848, 0.2417293957679677 },
      { 0.24172939576796787, 0.51654120846406137 },
    },
    {
      // order 5
      { 0.028112952182664158, 0.028112952182663919 },
      { 0.94377409563467429, 0.028112952182660831 },
      { 0.028112952182663454, 0.94377409563466119 },
      { 0.14856581227088814, 0.033533207700613903 },
      { 0.35719629861567614, 0.037824789609183222 },
      { 0.60497891177512486, 0.037824789609184437 },
      { 0.81790098002851497, 0.033533207700614784 },
      { 0.81790098002848755, 0.14856581227089186 },
      { 0.60497891177513641, 0.35719629861567298 },
      { 0.35719629861568036, 0.60497891177512952 },
      { 0.14856581227088492, 0.81790098002849354 },
      { 0.033533207700613361, 0.81790098002850942 },
      { 0.037824789609186282, 0.60497891177511576 },
      { 0.037824789609185484, 0.3571962986156802 },
      { 0.033533207700614028, 0.14856581227088839 },
      { 0.17713909846931261, 0.17713909846931619 },
      { 0.64572180306136384, 0.17713909846932788 },
      { 0.177139098469316, 0.64572180306138749 },
      { 0.4055085958674387, 0.1889828082651421 },
      { 0.40550859586744437, 0.40550859586742838 },
      { 0.18898280826513217, 0.40550859586741922 },
    },
  };
  return nodes;
}

} // anonymous namespace

bool FixedOrderGaussPoints(
  vtkDGCell::Shape shape, int nominalOrder, std::vector<std::vector<double>>& points)
{
  points.clear();
  // The "G" bases are registered at orders 1 through 5; gpts holds no other row.
  if (nominalOrder < 1 || nominalOrder > 5)
  {
    return false;
  }
  unsigned int order = static_cast<unsigned int>(nominalOrder);
  int count = nominalOrder + 1; // 1-D Gauss points along each axis

  switch (shape)
  {
    case vtkDGCell::Shape::Edge:
      for (int ii = 0; ii < count; ++ii)
      {
        points.push_back({ gaussPoint(order, ii) });
      }
      return true;

    // The prismatic shapes enumerate their degrees of freedom with the r-axis
    // varying fastest, matching the tensor-product basis functions.
    case vtkDGCell::Shape::Quadrilateral:
      for (int jj = 0; jj < count; ++jj)
      {
        for (int ii = 0; ii < count; ++ii)
        {
          points.push_back({ gaussPoint(order, ii), gaussPoint(order, jj) });
        }
      }
      return true;

    case vtkDGCell::Shape::Hexahedron:
      // The hexahedron's hand-written second-order basis numbers its degrees of
      // freedom the way the 27-node "C2" basis does - corners, then edges, then
      // the body center and the six face centers - rather than lexicographically
      // like the arbitrary-order kernel used at orders 3 and above. Reuse that
      // layout and simply move each point from the uniform lattice onto the
      // Gauss points, so the two orderings cannot drift apart.
      if (nominalOrder == 2)
      {
        std::vector<std::vector<double>> uniform;
        if (!FixedOrderLagrangePoints(shape, "C"_token, 2, uniform))
        {
          return false;
        }
        for (const auto& pt : uniform)
        {
          std::vector<double> mapped;
          mapped.reserve(pt.size());
          for (double coord : pt)
          {
            if (coord < -0.5)
            {
              coord = 0;
            }
            else if (coord > 0.5)
            {
              coord = 2;
            }
            else
            {
              coord = 1;
            }
            mapped.push_back(gaussPoint(order, coord));
          }
          points.push_back(mapped);
        }
        return true;
      }
      for (int kk = 0; kk < count; ++kk)
      {
        for (int jj = 0; jj < count; ++jj)
        {
          for (int ii = 0; ii < count; ++ii)
          {
            points.push_back(
              { gaussPoint(order, ii), gaussPoint(order, jj), gaussPoint(order, kk) });
          }
        }
      }
      return true;

    case vtkDGCell::Shape::Triangle:
      for (const auto& node : TriangleGaussNodes()[nominalOrder - 1])
      {
        points.push_back({ node[0], node[1] });
      }
      return true;

    // A wedge is a triangle crossed with an edge, with the triangle's index
    // varying fastest.
    case vtkDGCell::Shape::Wedge:
      for (int kk = 0; kk < count; ++kk)
      {
        for (const auto& node : TriangleGaussNodes()[nominalOrder - 1])
        {
          points.push_back({ node[0], node[1], gaussPoint(order, kk) });
        }
      }
      return true;

    // The tetrahedron's and pyramid's "G" bases are modal rather than nodal:
    // built from Jacobi polynomials, their functions do not reach 1 anywhere in
    // the cell, so there are no points to report. This mirrors the 18-node
    // quadratic pyramid handled by FixedOrderLagrangePoints().
    case vtkDGCell::Shape::Tetrahedron:
    case vtkDGCell::Shape::Pyramid:
    default:
      return false;
  }
}

VTK_ABI_NAMESPACE_END
} // namespace hgrad
} // namespace basis
} // namespace vtk
