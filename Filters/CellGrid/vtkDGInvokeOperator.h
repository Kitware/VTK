// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGInvokeOperator_h
#define vtkDGInvokeOperator_h

#include "vtkCellAttribute.h"         // For CellTypeInfo.
#include "vtkDGOperatorEntry.h"       // For ivars.
#include "vtkDataArray.h"             // For implementation.
#include "vtkFiltersCellGridModule.h" // For export macro.

#include <array>
#include <cstring> // For std::memset.
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class   vtkDGInvokeOperator
 * @brief   Invoke a DG-cell operator, weighting basis functions by coefficients.
 *
 * This class takes a vtkDGOperatorEntry and a vtkCellAttribute::CellTypeInfo;
 * it owns vectors for connectivity values (as needed, based on DOFSharing)
 * and coefficients.
 * It fetches coefficients; invokes the operator; and then computes the inner
 * product of the operator's values (basis functions, gradients, etc.) and
 * coefficients provided by the grid (via CellTypeInfo).
 *
 * Because each instance of this class holds tuple storage, it can be re-used
 * to amortize the cost of allocating the tuple storage. You can make
 * thread-local static instances of this class and (as long as the operator
 * entry's functor is thread-safe), call this object in threads.
 * This is the intended use of vtkDGInvokeOperator in vtkSMPTools-based
 * workers for query-responders.
 *
 * The vtkDGOperatorEntry determines the number of basis functions and the
 * size of each basis-function's output (generally a scalar or vector value
 * per basis function). The cell-type info determines the number of
 * coefficients (this may be 1 coefficient per basis function or multiple
 * coefficients per basis function. If the number of coefficient values is
 * not evenly divided by the number of basis functions, the Invoke()
 * method will return false.
 *
 * Note that this class does **not** pre-allocate storage for the resulting
 * inner product (i.e., the value of the function). Instead, you are
 * encouraged to provide a reference to external storage to hold the
 * final value. The output iterator must point to enough pre-allocated
 * storage to hold all the resulting values.
 */
class VTKFILTERSCELLGRID_EXPORT vtkDGInvokeOperator
{
public:
  vtkDGInvokeOperator() = default;
  vtkDGInvokeOperator(const vtkDGInvokeOperator&) = default;

  struct FetchUnsharedCellDOF
  {
    vtkDataArray* Coefficients{ nullptr };

    FetchUnsharedCellDOF() = default;
    FetchUnsharedCellDOF(vtkDataArray* vals, vtkDataArray* conn) { this->Initialize(vals, conn); }

    void Initialize(vtkDataArray* vals, vtkDataArray* conn = nullptr)
    {
      (void)conn;
      this->Coefficients = vals;
    }

    void operator()(vtkIdType ii, std::vector<double>& tuple)
    {
      // tuple.resize(this->Coefficients->GetNumberOfComponents());
      this->Coefficients->GetTuple(ii, tuple.data());
    }
  };

  struct FetchSharedCellDOF
  {
    vtkDataArray* Coefficients{ nullptr };
    vtkDataArray* Connectivity{ nullptr };
    vtkIdType Stride{ 0 };
    std::vector<vtkTypeInt64> ConnTuple;

    FetchSharedCellDOF() = default;
    FetchSharedCellDOF(vtkDataArray* vals, vtkDataArray* conn) { this->Initialize(vals, conn); }

    void Initialize(vtkDataArray* vals, vtkDataArray* conn)
    {
      this->Coefficients = vals;
      this->Connectivity = conn;
      this->Stride = vals->GetNumberOfComponents();
      this->ConnTuple.resize(conn->GetNumberOfComponents());
    }

    void operator()(vtkIdType ii, std::vector<double>& tuple)
    {
      this->Connectivity->GetIntegerTuple(ii, this->ConnTuple.data());
      auto valIt = tuple.begin();
      for (const auto& valId : this->ConnTuple)
      {
        this->Coefficients->GetTuple(valId, &(*valIt));
        valIt += this->Stride;
      }
    }
  };

  /// Given input cells and parametric coordinates to iterate, evaluate the operator
  /// and store the results in the output iterator.
  ///
  /// This allocates storage to hold operator output (e.g., basis function values) and
  /// coefficients for a single cell. Then, it calls \a op and computes the inner product
  /// of the operator output and the cell-coefficients, storing the results at the
  /// output iterator. Note that the output iterator must provide random access.
  template <typename InputIterator, typename OutputIterator>
  bool InvokeOp(const vtkDGOperatorEntry& op, const vtkCellAttribute::CellTypeInfo& info,
    InputIterator begin, InputIterator end, OutputIterator out);

  /// @name InternalMethods Internal methods.
  /// These methods are called by InvokeOp() depending on whether the attribute has
  /// shared degrees of freedom (DOF) or not.
  //@{
  template <typename InputIterator, typename OutputIterator>
  bool InvokeSharedDOF(const vtkDGOperatorEntry& op, const vtkCellAttribute::CellTypeInfo& info,
    InputIterator begin, InputIterator end, OutputIterator out);
  template <typename InputIterator, typename OutputIterator>
  bool InvokeUnsharedDOF(const vtkDGOperatorEntry& op, const vtkCellAttribute::CellTypeInfo& info,
    InputIterator begin, InputIterator end, OutputIterator out);

  /// Invoke the operator \a op once on \a rst and compute the inner product.
  ///
  /// This method stores the result in \a out.
  /// This method advances \a out by \a stride each time it is invoked.
  ///
  /// Both InvokeSharedDOF and InvokUnsharedDOF call this method as they
  /// iterate the inputs.
  template <typename OutputIterator>
  void InnerProduct(int coefficientsPerDOF, int stride, const vtkDGOperatorEntry& op,
    const std::array<double, 3>& rst, OutputIterator& out);
  //@}

  /// This is a convenience method that makes invoking the operator from python
  /// (as well as C++) simpler.
  ///
  /// Unlike the templated iterator-based methods above, this requires the
  /// cell IDs and parametric coordinates to be repeated even if only one
  /// varies over the \a num requested invocations.
  bool Invoke(const vtkDGOperatorEntry& op, const vtkCellAttribute::CellTypeInfo& info,
    std::size_t num, const vtkIdType* cellIds, const double* rst, double* result);

  /// Hold the function coefficients for a single cell.
  std::vector<double> CoeffTuple;

  /// Hold the function values for a single (r,s,t) evaluation.
  std::vector<double> OperatorTuple;

  /// Fetch values into CoeffTuple when DOF are shared among cells by connectivity.
  FetchSharedCellDOF SharedFetcher;

  /// Fetch values into CoeffTuple when DOF are not shared.
  FetchUnsharedCellDOF DiscontinuousFetcher;
};

template <typename InputIterator, typename OutputIterator>
bool vtkDGInvokeOperator::InvokeOp(const vtkDGOperatorEntry& op,
  const vtkCellAttribute::CellTypeInfo& info, InputIterator begin, InputIterator end,
  OutputIterator out)
{
  if (info.DOFSharing.IsValid())
  {
    return this->InvokeSharedDOF(op, info, begin, end, out);
  }
  else
  {
    return this->InvokeUnsharedDOF(op, info, begin, end, out);
  }
}

template <typename InputIterator, typename OutputIterator>
bool vtkDGInvokeOperator::InvokeSharedDOF(const vtkDGOperatorEntry& op,
  const vtkCellAttribute::CellTypeInfo& info, InputIterator begin, InputIterator end,
  OutputIterator out)
{
  using namespace vtk::literals;

  if (!op)
  {
    return false;
  }
  auto valsit = info.ArraysByRole.find("values"_token);
  if (valsit == info.ArraysByRole.end() || !valsit->second)
  {
    return false;
  }
  auto connit = info.ArraysByRole.find("connectivity"_token);
  if (connit == info.ArraysByRole.end() || !connit->second)
  {
    return false;
  }
  this->SharedFetcher.Initialize(
    vtkDataArray::SafeDownCast(valsit->second), vtkDataArray::SafeDownCast(connit->second));
  int coefficientsPerDOF = valsit->second->GetNumberOfComponents();
  this->CoeffTuple.resize(connit->second->GetNumberOfComponents() * coefficientsPerDOF);
  this->OperatorTuple.resize(op.OperatorSize * op.NumberOfFunctions);
  // There must be exactly one connectivity entry per operator function.
  if (connit->second->GetNumberOfComponents() != op.NumberOfFunctions)
  {
    return false;
  }
  int stride = op.OperatorSize * coefficientsPerDOF;
  vtkIdType lastCellId = -1; // Start with an invalid cell ID.
  for (auto initer = begin; initer != end; ++initer)
  {
    if (lastCellId != initer.GetCellId())
    {
      lastCellId = initer.GetCellId();
      this->SharedFetcher(lastCellId, this->CoeffTuple);
    }
    this->InnerProduct(coefficientsPerDOF, stride, op, initer.GetParameter(), out);
  }
  return true;
}

template <typename InputIterator, typename OutputIterator>
bool vtkDGInvokeOperator::InvokeUnsharedDOF(const vtkDGOperatorEntry& op,
  const vtkCellAttribute::CellTypeInfo& info, InputIterator begin, InputIterator end,
  OutputIterator out)
{
  using namespace vtk::literals;

  if (!op)
  {
    return false;
  }
  auto valsit = info.ArraysByRole.find("values"_token);
  if (valsit == info.ArraysByRole.end() || !valsit->second)
  {
    return false;
  }
  this->DiscontinuousFetcher.Initialize(vtkDataArray::SafeDownCast(valsit->second));
  int totalCoefficients = valsit->second->GetNumberOfComponents();
  /// The total number of coefficients per cell must be a multiple of the number
  /// of function values returned by the operator:
  if (totalCoefficients % op.NumberOfFunctions != 0)
  {
    return false;
  }
  int coefficientsPerDOF = totalCoefficients / op.NumberOfFunctions;
  this->CoeffTuple.resize(totalCoefficients);
  this->OperatorTuple.resize(op.OperatorSize * op.NumberOfFunctions);
  int stride = op.OperatorSize * coefficientsPerDOF;
  vtkIdType lastCellId = -1; // Start with an invalid cell ID.
  for (auto initer = begin; initer != end; ++initer)
  {
    if (lastCellId != initer.GetCellId())
    {
      lastCellId = initer.GetCellId();
      this->DiscontinuousFetcher(lastCellId, this->CoeffTuple);
    }
    this->InnerProduct(coefficientsPerDOF, stride, op, initer.GetParameter(), out);
  }
  return true;
}

template <typename OutputIterator>
void vtkDGInvokeOperator::InnerProduct(int coefficientsPerDOF, int stride,
  const vtkDGOperatorEntry& op, const std::array<double, 3>& rst, OutputIterator& out)
{
  // Compute the values for \a op at \a rst.
  op.Op(rst, this->OperatorTuple);
  // Zero the output for this input cell-id+(r,s,t)-tuple.
  std::memset(&(out[0]), 0, sizeof(double) * stride);
  // Now sum the "inner product" of the coeffient tuple and operator tuple into the output.
  for (int ii = 0; ii < coefficientsPerDOF; ++ii)
  {
    for (int jj = 0; jj < op.NumberOfFunctions; ++jj)
    {
      for (int kk = 0; kk < op.OperatorSize; ++kk)
      {
        out[ii * op.OperatorSize + kk] += this->CoeffTuple[jj * coefficientsPerDOF + ii] *
          this->OperatorTuple[jj * op.OperatorSize + kk];
      }
    }
  }
  out += stride;
}

VTK_ABI_NAMESPACE_END
#endif // vtkDGInvokeOperator_h
