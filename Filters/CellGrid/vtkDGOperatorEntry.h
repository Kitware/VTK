// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGOperatorEntry_h
#define vtkDGOperatorEntry_h

#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkStringToken.h"           // For ivar.

#include <array>
#include <functional>
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
 * Basis: one of "I"_token, "C"_token, "F"_token, "A"_token. "I", "C", and "F"
 *    are the incomplete, complete, and full bases of a fixed order; "A" is a
 *    basis registered for an arbitrary order, which numbers its degrees of
 *    freedom lexicographically rather than corner-first the way "C" does at a
 *    fixed order (see vtkDGLagrangePoints).
 * Order: a non-negative integer, or -1 to register an operator that accepts any order.
 * CellShape: one of "vtkDGHex"_token, "vtkDGQuad"_token, etc. but may also include
 *    "vtkDeRhamCell"_token, or "vtkDGCell"_token. In this way, if a cell does not
 *    have its own operator it can see whether a more generic version exists.
 *    This accommodates the "constant" function space where all shapes produce the
 *    same function.
 *
 * @section arbitrary-order Arbitrary-order operators
 *
 * Most operators implement a single polynomial order and are registered under
 * that order. Some, those whose basis functions are written for an arbitrary
 * order, are instead registered under the order -1, which vtkDGCell treats as
 * a wildcard matching any order for which no exact registration exists.
 *
 * Such an operator cannot be evaluated until it is *bound* to a concrete
 * polynomial order with SetOrder(); until then its NumberOfFunctions is unknown.
 * vtkDGCell::GetOperatorEntry() performs this binding for you, returning a copy
 * of the registered entry that carries the order requested by the cell-attribute.
 * Every entry handed out by that method is therefore ready to evaluate and
 * reports a concrete NumberOfFunctions, whether or not its order is fixed.
 *
 * The order is a vector holding one polynomial order per parametric axis of the
 * cell, so that anisotropic bases (higher order along one axis than another) are
 * expressible. It is fixed for all cells sharing a vtkCellAttribute::CellTypeInfo;
 * order that varies from cell to cell is not supported.
 */
class VTKFILTERSCELLGRID_EXPORT vtkDGOperatorEntry
{
public:
  /// The signature of the functions that evaluate an operator.
  ///
  /// Operators take
  /// + the parametric coordinates (always an (r, s, t) 3-tuple) at which to evaluate;
  /// + a vector in which to store the operator's values; and
  /// + the polynomial order along each parametric axis of the cell.
  ///
  /// The order is empty for operators registered at a fixed order (they ignore it).
  using OperatorFunction = std::function<void(
    const std::array<double, 3>&, std::vector<double>&, const std::vector<int>&)>;

  /// The signature of functions reporting how many basis functions an
  /// arbitrary-order operator provides at a given order.
  using FunctionCounter = std::function<int(const std::vector<int>&)>;

  vtkDGOperatorEntry() = default;
  vtkDGOperatorEntry(const vtkDGOperatorEntry&) = default;
  vtkDGOperatorEntry& operator=(const vtkDGOperatorEntry&) = default;

  /// Construct an entry for an operator implementing a single polynomial order.
  vtkDGOperatorEntry(int numFunc, int opSize, OperatorFunction op, const std::string& shader)
    : NumberOfFunctions(numFunc)
    , OperatorSize(opSize)
    , Op(op)
    , ShaderOp(shader)
  {
  }

  /// Construct an entry for an operator implementing an arbitrary polynomial order.
  ///
  /// The entry must be bound to an order with SetOrder() before it can be evaluated.
  vtkDGOperatorEntry(
    FunctionCounter counter, int opSize, OperatorFunction op, const std::string& shader)
    : OperatorSize(opSize)
    , Op(op)
    , ShaderOp(shader)
    , FunctionCount(counter)
  {
  }

  /// Entries may be implicitly converted to booleans.
  ///
  /// The conversion returns false when the function used to invoke the
  /// operation is null or when the operation accepts an arbitrary polynomial
  /// order but has not been bound to one. Otherwise it returns true.
  operator bool() const { return !!this->Op && this->NumberOfFunctions > 0; }

  /// Return whether this operator implements an arbitrary polynomial order.
  bool IsOrderDependent() const { return !!this->FunctionCount; }

  /// Bind an order-dependent operator to a concrete polynomial \a order.
  ///
  /// The \a order holds one entry per parametric axis of the cell. This computes
  /// NumberOfFunctions for the given order and returns whether it is positive.
  ///
  /// Calling this on an operator that implements a single fixed order is a no-op
  /// that returns true; such operators always know their own size.
  bool SetOrder(const std::vector<int>& order);

  /// Evaluate this operator at the parametric coordinates \a rst.
  ///
  /// The \a values you pass must be resized to hold at least
  /// nn = `NumberOfFunctions` * `OperatorSize` entries **before** you call this
  /// method. The first nn entries will have new values written to them.
  void Evaluate(const std::array<double, 3>& rst, std::vector<double>& values) const
  {
    this->Op(rst, values, this->Order);
  }

  ///@{
  /// Ready-made FunctionCounter implementations, shared by the function spaces.
  ///
  /// These report how many basis functions a shape admits at a given order.
  /// A shape whose parameter space cannot express the requested order returns
  /// zero, which makes SetOrder() fail rather than silently misinterpret it.

  /// A tensor product with an independent order along each parametric axis:
  /// (n0 + 1)(n1 + 1)… functions, and 1 for a shape with no parametric axes.
  static int TensorProductFunctionCount(const std::vector<int>& order);

  /// A simplex, whose axes share a single total degree:
  /// (n+1)(n+2)…(n+d)/d! functions. An anisotropic order is rejected.
  static int SimplexFunctionCount(const std::vector<int>& order);

  /// A wedge: a triangle (the r- and s-axes) crossed with an edge (the t-axis),
  /// so the two counts multiply. The triangular axes must share an order.
  static int WedgeFunctionCount(const std::vector<int>& order);
  ///@}

  /// Return a glsl string that defines this operator.
  std::string GetShaderString(
    const std::string& functionName, const std::string& parameterName, int order) const;

  /// The number of functions in the basis.
  ///
  /// Note that each basis function may evaluate to a scalar or a vector.
  /// See OperatorSize for more information.
  ///
  /// For order-dependent operators this is zero until SetOrder() is called.
  int NumberOfFunctions{ 0 };

  /// The number of coordinates each operator-function evaluates to.
  ///
  /// For H(grad) and constant function spaces, this is 1.
  /// For H(curl) and H(div), this is 3.
  int OperatorSize{ 1 };

  /// A function you may call to evaluate the operator.
  ///
  /// Prefer Evaluate(), which passes this entry's Order for you.
  OperatorFunction Op;

  /// A string holding the source code to evaluate all the basis functions.
  std::string ShaderOp;

  /// The polynomial order along each parametric axis of the cell.
  ///
  /// This is empty for operators that implement a single fixed order and is
  /// otherwise assigned by SetOrder().
  std::vector<int> Order;

  /// For order-dependent operators, computes NumberOfFunctions from an Order.
  ///
  /// This is null for operators that implement a single fixed order.
  FunctionCounter FunctionCount;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGOperatorEntry_h
