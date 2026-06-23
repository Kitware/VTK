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
#include "Basis_HGrad_PyrI2Basis.h"
#include "Basis_HGrad_PyrI2Gradient.h"
// #include "Basis_HGrad_PyrG1Basis.h"
// #include "Basis_HGrad_PyrG1Gradient.h"
// #include "Basis_HGrad_PyrG2Basis.h"
// #include "Basis_HGrad_PyrG2Gradient.h"
#include "Basis_HGrad_QuadC1Basis.h"
#include "Basis_HGrad_QuadC1Gradient.h"
#include "Basis_HGrad_QuadC2Basis.h"
#include "Basis_HGrad_QuadC2Gradient.h"
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
#include "Basis_HGrad_TetF2Basis.h"
#include "Basis_HGrad_TetF2Gradient.h"
// #include "Basis_HGrad_TetG1Basis.h"
// #include "Basis_HGrad_TetG1Gradient.h"
// #include "Basis_HGrad_TetG2Basis.h"
// #include "Basis_HGrad_TetG2Gradient.h"
// #include "Basis_HGrad_TetGnBasis.h"
// #include "Basis_HGrad_TetGnGradient.h"
#include "Basis_HGrad_TriC1Basis.h"
#include "Basis_HGrad_TriC1Gradient.h"
#include "Basis_HGrad_TriC2Basis.h"
#include "Basis_HGrad_TriC2Gradient.h"
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
#include "Basis_HGrad_WdgF2Basis.h"
#include "Basis_HGrad_WdgF2Gradient.h"
#include "Basis_HGrad_WdgI2Basis.h"
#include "Basis_HGrad_WdgI2Gradient.h"
// #include "Basis_HGrad_WdgG1Basis.h"
// #include "Basis_HGrad_WdgG1Gradient.h"
// #include "Basis_HGrad_WdgG2Basis.h"
// #include "Basis_HGrad_WdgG2Gradient.h"
// #include "Basis_HGrad_WdgGnBasis.h"
// #include "Basis_HGrad_WdgGnGradient.h"

#include <cmath>
#include <limits>
#include <type_traits>

#define RealT double

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

// Some kernels (esp. those of arbitrary order) need scratch space.
#define WORKSPACE(type, name, size)                                                                \
  static thread_local std::vector<type> name;                                                      \
  name.resize(size);

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

// clang-format off
/// Gauss point locations in 1-D, used by "G" basis/gradient functions.
std::vector<std::vector<RealT>> gpts{
  { 0 },
  { -0.577350269189625731,  0.577350269189625731 },
  { -0.774596669241483404,  0.000000000000000000,  0.774596669241483404 },
  { -0.861136311594052462, -0.339981043584856257,  0.339981043584856257,  0.861136311594052462 },
  { -0.906179845938663853, -0.538469310105682997,  0.000000000000000000,  0.538469310105682997,  0.906179845938663853 },
  { -0.932469514203152050, -0.661209386466264482, -0.238619186083196932,  0.238619186083196932,  0.661209386466264482,  0.932469514203152050},
};
// clang-format on

void EdgeC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeC1Basis.h"
}
void EdgeC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeC1Gradient.h"
}

void EdgeC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeC2Basis.h"
}
void EdgeC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeC2Gradient.h"
}

void EdgeG1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG1Basis.h"
}
void EdgeG1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG1Gradient.h"
}

void EdgeG2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG2Basis.h"
}
void EdgeG2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG2Gradient.h"
}

void EdgeG3Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG3Basis.h"
}
void EdgeG3Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG3Gradient.h"
}

void EdgeG4Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG4Basis.h"
}
void EdgeG4Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG4Gradient.h"
}

void EdgeG5Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG5Basis.h"
}
void EdgeG5Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/EdgeG5Gradient.h"
}

void HexC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexC1Basis.h"
}
void HexC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexC1Gradient.h"
}

void HexI2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexI2Basis.h"
}
void HexI2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexI2Gradient.h"
}

void HexC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexC2Basis.h"
}
void HexC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexC2Gradient.h"
}

void HexG1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexG1Basis.h"
}
void HexG1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexG1Gradient.h"
}

void HexG2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexG2Basis.h"
}
void HexG2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/HexG2Gradient.h"
}

void HexG3Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/HexGnBasis.h"
}
void HexG3Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/HexGnGradient.h"
}

void HexG4Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/HexGnBasis.h"
}
void HexG4Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/HexGnGradient.h"
}

void HexG5Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/HexGnBasis.h"
}
void HexG5Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/HexGnGradient.h"
}

void PyrC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrC1Basis.h"
}
void PyrC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrC1Gradient.h"
}

void PyrI2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrI2Basis.h"
}
void PyrI2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrI2Gradient.h"
}

void PyrC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrC2Basis.h"
}
void PyrC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrC2Gradient.h"
}

void PyrF2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrF2Basis.h"
}
void PyrF2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrF2Gradient.h"
}

#if 0
void PyrG1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrG1Basis.h"
}
void PyrG1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrG1Gradient.h"
}

void PyrG2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrG2Basis.h"
}
void PyrG2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/PyrG2Gradient.h"
}
#endif

void QuadC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadC1Basis.h"
}
void QuadC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadC1Gradient.h"
}

void QuadC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadC2Basis.h"
}
void QuadC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadC2Gradient.h"
}

void QuadG1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadG1Basis.h"
}
void QuadG1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadG1Gradient.h"
}

void QuadG2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadG2Basis.h"
}
void QuadG2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/QuadG2Gradient.h"
}

void QuadG3Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/QuadGnBasis.h"
}
void QuadG3Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/QuadGnGradient.h"
}

void QuadG4Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/QuadGnBasis.h"
}
void QuadG4Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/QuadGnGradient.h"
}

void QuadG5Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/QuadGnBasis.h"
}
void QuadG5Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/QuadGnGradient.h"
}

void TetC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetC1Basis.h"
}
void TetC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetC1Gradient.h"
}

void TetC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetC2Basis.h"
}
void TetC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetC2Gradient.h"
}

void TetF2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetF2Basis.h"
}
void TetF2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetF2Gradient.h"
}

#if 0
void TetG1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetG1Basis.h"
}
void TetG1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetG1Gradient.h"
}

void TetG2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetG2Basis.h"
}
void TetG2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TetG2Gradient.h"
}

void TetG3Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/TetGnBasis.h"
}
void TetG3Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/TetGnGradient.h"
}

void TetG4Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/TetGnBasis.h"
}
void TetG4Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/TetGnGradient.h"
}

void TetG5Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/TetGnBasis.h"
}
void TetG5Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/TetGnGradient.h"
}
#endif

void TriC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriC1Basis.h"
}
void TriC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriC1Gradient.h"
}

void TriC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriC2Basis.h"
}
void TriC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriC2Gradient.h"
}

void TriG1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG1Basis.h"
}
void TriG1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG1Gradient.h"
}

void TriG2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG2Basis.h"
}
void TriG2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG2Gradient.h"
}

void TriG3Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG3Basis.h"
}
void TriG3Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG3Gradient.h"
}

void TriG4Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG4Basis.h"
}
void TriG4Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG4Gradient.h"
}

void TriG5Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG5Basis.h"
}
void TriG5Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/TriG5Gradient.h"
}

void WdgC1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgC1Basis.h"
}
void WdgC1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgC1Gradient.h"
}

void WdgI2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgI2Basis.h"
}
void WdgI2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgI2Gradient.h"
}

void WdgC2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgC2Basis.h"
}
void WdgC2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgC2Gradient.h"
}

void WdgF2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgF2Basis.h"
}
void WdgF2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgF2Gradient.h"
}

#if 0
void WdgG1Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgG1Basis.h"
}
void WdgG1Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgG1Gradient.h"
}

void WdgG2Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgG2Basis.h"
}
void WdgG2Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisHeader();
#include "Basis/HGrad/WdgG2Gradient.h"
}

void WdgG3Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/WdgGnBasis.h"
}
void WdgG3Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(3);
#include "Basis/HGrad/WdgGnGradient.h"
}

void WdgG4Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/WdgGnBasis.h"
}
void WdgG4Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(4);
#include "Basis/HGrad/WdgGnGradient.h"
}

void WdgG5Basis(const std::array<double, 3>& param, std::vector<double>& basis)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/WdgGnBasis.h"
}
void WdgG5Gradient(const std::array<double, 3>& param, std::vector<double>& basisGradient)
{
  vtkBasisOrderHeader(5);
#include "Basis/HGrad/WdgGnGradient.h"
}
#endif

bool RegisterOperators()
{
  auto& opMap = vtkDGCell::GetOperators();
  auto& basisMap = opMap["Basis"_token]["HGRAD"_token];
  auto& gradMap = opMap["BasisGradient"_token]["HGRAD"_token];

  // clang-format off
  // # Basis functions
  basisMap["C"_token][1]["vtkDGEdge"_token] = {  2, 1, EdgeC1Basis, Basis_HGrad_EdgeC1Basis };
  basisMap["C"_token][2]["vtkDGEdge"_token] = {  3, 1, EdgeC2Basis, Basis_HGrad_EdgeC2Basis };
  basisMap["G"_token][1]["vtkDGEdge"_token] = {  2, 1, EdgeG1Basis, Basis_HGrad_EdgeG1Basis };
  basisMap["G"_token][2]["vtkDGEdge"_token] = {  3, 1, EdgeG2Basis, Basis_HGrad_EdgeG2Basis };
  basisMap["G"_token][3]["vtkDGEdge"_token] = {  4, 1, EdgeG3Basis, Basis_HGrad_EdgeG3Basis };
  basisMap["G"_token][4]["vtkDGEdge"_token] = {  5, 1, EdgeG4Basis, Basis_HGrad_EdgeG4Basis };
  basisMap["G"_token][5]["vtkDGEdge"_token] = {  6, 1, EdgeG5Basis, Basis_HGrad_EdgeG5Basis };

  basisMap["C"_token][1]["vtkDGHex"_token]  = {  8, 1, HexC1Basis,  Basis_HGrad_HexC1Basis };
  basisMap["I"_token][2]["vtkDGHex"_token]  = { 20, 1, HexI2Basis,  Basis_HGrad_HexI2Basis };
  basisMap["C"_token][2]["vtkDGHex"_token]  = { 27, 1, HexC2Basis,  Basis_HGrad_HexC2Basis };
  basisMap["G"_token][1]["vtkDGHex"_token]  = {  8, 1, HexG1Basis,  Basis_HGrad_HexG1Basis };
  basisMap["G"_token][2]["vtkDGHex"_token]  = { 27, 1, HexG2Basis,  Basis_HGrad_HexG2Basis };
  basisMap["G"_token][3]["vtkDGHex"_token]  = { 64, 1, HexG3Basis,  Basis_HGrad_HexGnBasis };
  basisMap["G"_token][4]["vtkDGHex"_token]  = {125, 1, HexG4Basis,  Basis_HGrad_HexGnBasis };
  basisMap["G"_token][5]["vtkDGHex"_token]  = {216, 1, HexG5Basis,  Basis_HGrad_HexGnBasis };

  basisMap["C"_token][1]["vtkDGPyr"_token]  = {  5, 1, PyrC1Basis,  Basis_HGrad_PyrC1Basis };
  basisMap["I"_token][2]["vtkDGPyr"_token]  = { 13, 1, PyrI2Basis,  Basis_HGrad_PyrI2Basis };
  basisMap["C"_token][2]["vtkDGPyr"_token]  = { 18, 1, PyrC2Basis,  Basis_HGrad_PyrC2Basis };
  basisMap["F"_token][2]["vtkDGPyr"_token]  = { 19, 1, PyrF2Basis,  Basis_HGrad_PyrF2Basis };
  // basisMap["G"_token][1]["vtkDGPyr"_token]  = {  5, 1, PyrG1Basis,  Basis_HGrad_PyrG1Basis };
  // basisMap["G"_token][2]["vtkDGPyr"_token]  = { 18, 1, PyrG2Basis,  Basis_HGrad_PyrG2Basis };

  basisMap["C"_token][1]["vtkDGQuad"_token] = {  4, 1, QuadC1Basis, Basis_HGrad_QuadC1Basis };
  basisMap["C"_token][2]["vtkDGQuad"_token] = {  9, 1, QuadC2Basis, Basis_HGrad_QuadC2Basis };
  basisMap["G"_token][1]["vtkDGQuad"_token] = {  4, 1, QuadG1Basis, Basis_HGrad_QuadG1Basis };
  basisMap["G"_token][2]["vtkDGQuad"_token] = {  9, 1, QuadG2Basis, Basis_HGrad_QuadG2Basis };
  basisMap["G"_token][3]["vtkDGQuad"_token] = { 16, 1, QuadG3Basis, Basis_HGrad_QuadGnBasis };
  basisMap["G"_token][4]["vtkDGQuad"_token] = { 25, 1, QuadG4Basis, Basis_HGrad_QuadGnBasis };
  basisMap["G"_token][5]["vtkDGQuad"_token] = { 36, 1, QuadG5Basis, Basis_HGrad_QuadGnBasis };

  basisMap["C"_token][1]["vtkDGTet"_token]  = {  4, 1, TetC1Basis,  Basis_HGrad_TetC1Basis };
  basisMap["C"_token][2]["vtkDGTet"_token]  = { 10, 1, TetC2Basis,  Basis_HGrad_TetC2Basis };
  basisMap["F"_token][2]["vtkDGTet"_token]  = { 15, 1, TetF2Basis,  Basis_HGrad_TetF2Basis };
  // basisMap["G"_token][1]["vtkDGTet"_token]  = {  4, 1, TetG1Basis,  Basis_HGrad_TetG1Basis };
  // basisMap["G"_token][2]["vtkDGTet"_token]  = { 10, 1, TetG2Basis,  Basis_HGrad_TetG2Basis };
  // basisMap["G"_token][3]["vtkDGTet"_token]  = { 20, 1, TetG3Basis,  Basis_HGrad_TetGnBasis };
  // basisMap["G"_token][4]["vtkDGTet"_token]  = { 35, 1, TetG4Basis,  Basis_HGrad_TetGnBasis };
  // basisMap["G"_token][5]["vtkDGTet"_token]  = { 56, 1, TetG5Basis,  Basis_HGrad_TetGnBasis };

  basisMap["C"_token][1]["vtkDGTri"_token]  = {  3, 1, TriC1Basis,  Basis_HGrad_TriC1Basis };
  basisMap["C"_token][2]["vtkDGTri"_token]  = {  6, 1, TriC2Basis,  Basis_HGrad_TriC2Basis };
  basisMap["G"_token][1]["vtkDGTri"_token]  = {  3, 1, TriG1Basis,  Basis_HGrad_TriG1Basis };
  basisMap["G"_token][2]["vtkDGTri"_token]  = {  6, 1, TriG2Basis,  Basis_HGrad_TriG2Basis };
  basisMap["G"_token][3]["vtkDGTri"_token]  = { 10, 1, TriG3Basis,  Basis_HGrad_TriG3Basis };
  basisMap["G"_token][4]["vtkDGTri"_token]  = { 15, 1, TriG4Basis,  Basis_HGrad_TriG4Basis };
  basisMap["G"_token][5]["vtkDGTri"_token]  = { 21, 1, TriG5Basis,  Basis_HGrad_TriG5Basis };

  basisMap["C"_token][1]["vtkDGWdg"_token]  = {  6, 1, WdgC1Basis,  Basis_HGrad_WdgC1Basis };
  basisMap["I"_token][2]["vtkDGWdg"_token]  = { 15, 1, WdgI2Basis,  Basis_HGrad_WdgI2Basis };
  basisMap["C"_token][2]["vtkDGWdg"_token]  = { 18, 1, WdgC2Basis,  Basis_HGrad_WdgC2Basis };
  basisMap["F"_token][2]["vtkDGWdg"_token]  = { 21, 1, WdgF2Basis,  Basis_HGrad_WdgF2Basis };
  // basisMap["G"_token][1]["vtkDGWdg"_token]  = {  6, 1, WdgG1Basis,  Basis_HGrad_WdgG1Basis };
  // basisMap["G"_token][2]["vtkDGWdg"_token]  = { 18, 1, WdgG2Basis,  Basis_HGrad_WdgG2Basis };
  // basisMap["G"_token][3]["vtkDGWdg"_token]  = { 40, 1, WdgG3Basis,  Basis_HGrad_WdgGnBasis };
  // basisMap["G"_token][4]["vtkDGWdg"_token]  = { 75, 1, WdgG4Basis,  Basis_HGrad_WdgGnBasis };
  // basisMap["G"_token][5]["vtkDGWdg"_token]  = {126, 1, WdgG5Basis,  Basis_HGrad_WdgGnBasis };

  // # Gradients of basis functions
  gradMap["C"_token][1]["vtkDGEdge"_token] = {  2, 3, EdgeC1Gradient, Basis_HGrad_EdgeC1Gradient };
  gradMap["C"_token][2]["vtkDGEdge"_token] = {  3, 3, EdgeC2Gradient, Basis_HGrad_EdgeC2Gradient };
  gradMap["G"_token][1]["vtkDGEdge"_token] = {  2, 3, EdgeG1Gradient, Basis_HGrad_EdgeG1Gradient };
  gradMap["G"_token][2]["vtkDGEdge"_token] = {  3, 3, EdgeG2Gradient, Basis_HGrad_EdgeG2Gradient };
  gradMap["G"_token][3]["vtkDGEdge"_token] = {  4, 3, EdgeG3Gradient, Basis_HGrad_EdgeG3Gradient };
  gradMap["G"_token][4]["vtkDGEdge"_token] = {  5, 3, EdgeG4Gradient, Basis_HGrad_EdgeG4Gradient };
  gradMap["G"_token][5]["vtkDGEdge"_token] = {  6, 3, EdgeG5Gradient, Basis_HGrad_EdgeG5Gradient };

  gradMap["C"_token][1]["vtkDGHex"_token]  = {  8, 3, HexC1Gradient,  Basis_HGrad_HexC1Gradient };
  gradMap["I"_token][2]["vtkDGHex"_token]  = { 20, 3, HexI2Gradient,  Basis_HGrad_HexI2Gradient };
  gradMap["C"_token][2]["vtkDGHex"_token]  = { 27, 3, HexC2Gradient,  Basis_HGrad_HexC2Gradient };
  gradMap["G"_token][1]["vtkDGHex"_token]  = {  8, 3, HexG1Gradient,  Basis_HGrad_HexG1Gradient };
  gradMap["G"_token][2]["vtkDGHex"_token]  = { 27, 3, HexG2Gradient,  Basis_HGrad_HexG2Gradient };
  gradMap["G"_token][3]["vtkDGHex"_token]  = { 64, 3, HexG3Gradient,  Basis_HGrad_HexGnGradient };
  gradMap["G"_token][4]["vtkDGHex"_token]  = {125, 3, HexG4Gradient,  Basis_HGrad_HexGnGradient };
  gradMap["G"_token][5]["vtkDGHex"_token]  = {216, 3, HexG5Gradient,  Basis_HGrad_HexGnGradient };

  gradMap["C"_token][1]["vtkDGPyr"_token]  = {  5, 3, PyrC1Gradient,  Basis_HGrad_PyrC1Gradient };
  gradMap["I"_token][2]["vtkDGPyr"_token]  = { 13, 3, PyrI2Gradient,  Basis_HGrad_PyrI2Gradient };
  gradMap["C"_token][2]["vtkDGPyr"_token]  = { 18, 3, PyrC2Gradient,  Basis_HGrad_PyrC2Gradient };
  gradMap["F"_token][2]["vtkDGPyr"_token]  = { 19, 3, PyrF2Gradient,  Basis_HGrad_PyrF2Gradient };
  // gradMap["G"_token][1]["vtkDGPyr"_token]  = {  5, 3, PyrG1Gradient,  Basis_HGrad_PyrG1Gradient };
  // gradMap["G"_token][2]["vtkDGPyr"_token]  = { 18, 3, PyrG2Gradient,  Basis_HGrad_PyrG2Gradient };

  gradMap["C"_token][1]["vtkDGQuad"_token] = {  4, 3, QuadC1Gradient, Basis_HGrad_QuadC1Gradient };
  gradMap["C"_token][2]["vtkDGQuad"_token] = {  9, 3, QuadC2Gradient, Basis_HGrad_QuadC2Gradient };
  gradMap["G"_token][1]["vtkDGQuad"_token] = {  4, 3, QuadG1Gradient, Basis_HGrad_QuadG1Gradient };
  gradMap["G"_token][2]["vtkDGQuad"_token] = {  9, 3, QuadG2Gradient, Basis_HGrad_QuadG2Gradient };
  gradMap["G"_token][3]["vtkDGQuad"_token] = { 16, 3, QuadG3Gradient, Basis_HGrad_QuadGnGradient };
  gradMap["G"_token][4]["vtkDGQuad"_token] = { 25, 3, QuadG4Gradient, Basis_HGrad_QuadGnGradient };
  gradMap["G"_token][5]["vtkDGQuad"_token] = { 36, 3, QuadG5Gradient, Basis_HGrad_QuadGnGradient };

  gradMap["C"_token][1]["vtkDGTet"_token]  = {  4, 3, TetC1Gradient,  Basis_HGrad_TetC1Gradient };
  gradMap["C"_token][2]["vtkDGTet"_token]  = { 10, 3, TetC2Gradient,  Basis_HGrad_TetC2Gradient };
  gradMap["F"_token][2]["vtkDGTet"_token]  = { 15, 3, TetF2Gradient,  Basis_HGrad_TetF2Gradient };
  // gradMap["G"_token][1]["vtkDGTet"_token]  = {  4, 3, TetG1Gradient,  Basis_HGrad_TetG1Gradient };
  // gradMap["G"_token][2]["vtkDGTet"_token]  = { 10, 3, TetG2Gradient,  Basis_HGrad_TetG2Gradient };
  // gradMap["G"_token][3]["vtkDGTet"_token]  = { 20, 3, TetG3Gradient,  Basis_HGrad_TetGnGradient };
  // gradMap["G"_token][4]["vtkDGTet"_token]  = { 35, 3, TetG4Gradient,  Basis_HGrad_TetGnGradient };
  // gradMap["G"_token][5]["vtkDGTet"_token]  = { 56, 3, TetG5Gradient,  Basis_HGrad_TetGnGradient };

  gradMap["C"_token][1]["vtkDGTri"_token]  = {  3, 3, TriC1Gradient,  Basis_HGrad_TriC1Gradient };
  gradMap["C"_token][2]["vtkDGTri"_token]  = {  6, 3, TriC2Gradient,  Basis_HGrad_TriC2Gradient };
  gradMap["G"_token][1]["vtkDGTri"_token]  = {  3, 3, TriG1Gradient,  Basis_HGrad_TriG1Gradient };
  gradMap["G"_token][2]["vtkDGTri"_token]  = {  6, 3, TriG2Gradient,  Basis_HGrad_TriG2Gradient };
  gradMap["G"_token][3]["vtkDGTri"_token]  = { 10, 3, TriG3Gradient,  Basis_HGrad_TriG3Gradient };
  gradMap["G"_token][4]["vtkDGTri"_token]  = { 15, 3, TriG4Gradient,  Basis_HGrad_TriG4Gradient };
  gradMap["G"_token][5]["vtkDGTri"_token]  = { 21, 3, TriG5Gradient,  Basis_HGrad_TriG5Gradient };

  gradMap["C"_token][1]["vtkDGWdg"_token]  = {  6, 3, WdgC1Gradient,  Basis_HGrad_WdgC1Gradient };
  gradMap["I"_token][2]["vtkDGWdg"_token]  = { 15, 3, WdgI2Gradient,  Basis_HGrad_WdgI2Gradient };
  gradMap["C"_token][2]["vtkDGWdg"_token]  = { 18, 3, WdgC2Gradient,  Basis_HGrad_WdgC2Gradient };
  gradMap["F"_token][2]["vtkDGWdg"_token]  = { 21, 3, WdgF2Gradient,  Basis_HGrad_WdgF2Gradient };
  // gradMap["G"_token][1]["vtkDGWdg"_token]  = {  6, 3, WdgG1Gradient,  Basis_HGrad_WdgG1Gradient };
  // gradMap["G"_token][2]["vtkDGWdg"_token]  = { 18, 3, WdgG2Gradient,  Basis_HGrad_WdgG2Gradient };
  // gradMap["G"_token][3]["vtkDGWdg"_token]  = { 40, 3, WdgG3Gradient,  Basis_HGrad_WdgGnGradient };
  // gradMap["G"_token][4]["vtkDGWdg"_token]  = { 75, 3, WdgG4Gradient,  Basis_HGrad_WdgGnGradient };
  // gradMap["G"_token][5]["vtkDGWdg"_token]  = {126, 3, WdgG5Gradient,  Basis_HGrad_WdgGnGradient };

  // clang-format on
  return true;
}

VTK_ABI_NAMESPACE_END
} // namespace hgrad
} // namespace basis
} // namespace vtk
