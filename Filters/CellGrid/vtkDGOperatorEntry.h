// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGOperatorEntry_h
#define vtkDGOperatorEntry_h

#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkStringToken.h"           // For ivar.

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class   vtkDGOperatorEntry
 * @brief   A record for a basis in a function space that is specific to one cell shape.
 *
 * OperatorName → vtkCellAttribute::CellTypeInfo (FS, Basis, Order) → CellShape →
 * vtkDGOperatorEntry.
 *
 * OperatorName: one of "Basis"_token, "BasisGradient"_token, "Curl"_token, "Div"_token, etc.
 * FunctionSpace: one of "constant"_token, "HGRAD"_token, "HCURL"_token, "HDIV"_token.
 * Basis: one of "I"_token, "C"_token, "F"_token.
 * Order: a non-negative integer.
 * CellShape: one of "vtkDGHex"_token, "vtkDGQuad"_token, etc. but may also include
 *    "vtkDeRhamCell"_token, or "vtkDGCell"_token. In this way, if a cell does not
 *    have its own operator it can see whether a more generic version exists.
 *    This accommodates the "constant" function space where all shapes produce the
 *    same function.
 */
class VTKFILTERSCELLGRID_EXPORT vtkDGOperatorEntry
{
public:
  using OperatorFunction = std::function<void(const std::array<double, 3>, std::vector<double>&)>;

  vtkDGOperatorEntry() = default;
  vtkDGOperatorEntry(const vtkDGOperatorEntry&) = default;
  vtkDGOperatorEntry(int numFunc, int opSize, OperatorFunction op, const std::string& shader)
    : NumberOfFunctions(numFunc)
    , OperatorSize(opSize)
    , Op(op)
    , ShaderOp(shader)
  {
  }

  /// Entries may be implicitly converted to booleans.
  ///
  /// When the function used to invoke the operation is null,
  /// the conversion returns false. Otherwise it returns true.
  operator bool() const { return !!this->Op; }

  /// Return a glsl string that defines this operator.
  std::string GetShaderString(
    const std::string& functionName, const std::string& parameterName) const;

  /// The number of functions in the basis.
  ///
  /// Note that each basis function may evaluate to a scalar or a vector.
  /// See FunctionSize for more information.
  int NumberOfFunctions{ 0 };

  /// The number of coordinates each operator-function evaluates to.
  ///
  /// For H(grad) and constant function spaces, this is 1.
  /// For H(curl) and H(div), this is 3.
  int OperatorSize{ 1 };

  /// A function you may call to evaluate the operator.
  ///
  /// You pass in the parametric coordinates (always an (r, s, t) 3-tuple) and a
  /// vector to hold the function values.
  ///
  /// The `std::vector<double>` you pass to the \a Op functor must be
  /// resized to hold at least nn = `NumberOfFunctions` * `OperatorSize`
  /// entries **before** you invoke \a Op. The first nn entries will
  /// have new values written to them.
  OperatorFunction Op;

  /// A string holding the source code to evaluate all the basis functions.
  std::string ShaderOp;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGOperatorEntry_h
