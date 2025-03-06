// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGOperationEvaluator_h
#define vtkDGOperationEvaluator_h

#include "vtkCompiler.h" // For export macro.
#include "vtkDGOperationState.h"
#include "vtkDGOperationStateEntry.h" // For API.
#include "vtkDoubleArray.h"           // For API.
#include "vtkMatrix3x3.h"             // For API.

/// Whether degrees of freedom (DOF) are shared between cells.
///
/// This enumeration is used as a template parameter to methods that evaluate
/// operators on cells to determine how to fetch degrees-of-freedom from the
/// provided arrays: SharedDOF indicates that a connectivity array is used so
/// that multiple cells can reference the same DOF. Discontinuous indicates
/// that no connectivity array exists and each tuple in the "values" array
/// holds DOF for every basis function for one entire cell.
enum vtkDGSharingType
{
  SharedDOF,    //!< Degrees of freedom (DOF) are shared.
  Discontinuous //!< Degrees of freedom are not shared.
};

/// Whether cells are stand-alone or sides of other cells.
enum vtkDGSideType
{
  Cells, //!< A cell specified by degrees of freedom held in arrays.
  Sides  //!< A side specified by a cell ID and an integer index into sides.
};

/// Which type of shape-function post-processing is required.
///
/// Some interpolation techniques require the shape attribute
/// (i.e., vtkCellGrid::GetShapeAttribute) to be evaluated and used
/// to transform an operator's values into world coordinates.
/// This enumeration indicates which (if any) scaling technique
/// should be used.
enum vtkDGShapeModifier
{
  InverseJacobian, ///!< For HCURL
  None,            ///!< For HGRAD
  ScaledJacobian   ///!< For HDIV
};

/**@class vtkDGOperationEvaluator
 *
 * Evaluate a vtkDGOperationEntry on a provided cell ID at provided parametric coordinates.
 *
 * This inherits vtkDGOperationState for all the scratch-space it provides and adds methods
 * to perform interpolation using the scratch space.
 */
template <typename InputIterator, typename OutputIterator, vtkDGSharingType DOFSharing,
  vtkDGSideType SourceType, vtkDGShapeModifier Modifier,
  vtkDGSharingType ShapeSharing = Discontinuous>
class VTK_ALWAYS_EXPORT VTK_WRAPEXCLUDE vtkDGOperationEvaluator : public vtkDGOperationState
{
public:
  using InputIteratorType = InputIterator;
  using OutputIteratorType = OutputIterator;

  vtkDGOperationEvaluator(
    // Attribute arrays/operation
    vtkDGOperatorEntry& op, vtkDataArray* connectivity, vtkDataArray* values,
    vtkDataArray* sideConn, vtkTypeUInt64 offset,
    // Shape arrays/operation
    vtkDGOperatorEntry shapeGradient = vtkDGOperatorEntry(),
    vtkDataArray* shapeConnectivity = nullptr, vtkDataArray* shapeValues = nullptr)
    : vtkDGOperationState(
        op, connectivity, values, sideConn, offset, shapeGradient, shapeConnectivity, shapeValues)
  {
    if (!op)
    {
      throw std::logic_error("Must have non-null operator.");
    }
    if (Modifier != None && !shapeGradient)
    {
      throw std::logic_error("Must have non-null shape gradient operator.");
    }
    this->BasisTuple.resize(op.NumberOfFunctions * op.OperatorSize);
    int ncc = 0;
    if (this->CellConnectivity)
    {
      ncc = this->CellConnectivity->GetNumberOfComponents();
      this->ConnTuple.resize(ncc);
    }
    else if (DOFSharing == SharedDOF)
    {
      throw std::logic_error("DOF sharing requires a cell-connectivity array.");
    }
    int nvc = this->CellValues->GetNumberOfComponents();
    if (DOFSharing == SharedDOF)
    {
      this->NumberOfValuesPerFunction = nvc;
      this->ValueTuple.resize(nvc * ncc);
    }
    else
    {
      this->NumberOfValuesPerFunction = nvc / this->OpEntry.NumberOfFunctions;
      this->ValueTuple.resize(nvc);
    }

    // If we must also evaluate the shape-attribute modifier for each
    // result value, then prepare tuples to hold shape data.
    if (Modifier != None)
    {
      this->Jacobian.resize(9); // TODO: Handle 2-d Jacobians differently eventually.
      this->ShapeBasisTuple.resize(shapeGradient.NumberOfFunctions * shapeGradient.OperatorSize);
      int nsc = 0;
      if (this->ShapeConnectivity)
      {
        nsc = this->ShapeConnectivity->GetNumberOfComponents();
        this->ShapeConnTuple.resize(nsc);
      }
      else if (ShapeSharing == SharedDOF)
      {
        throw std::logic_error("Shape DOF-sharing requires a shape-connectivity array.");
      }
      int nvs = this->ShapeValues->GetNumberOfComponents();
      if (ShapeSharing == SharedDOF)
      {
        this->NumberOfShapeValuesPerFunction = nvs;
        this->ShapeValueTuple.resize(nvs * nsc);
      }
      else
      {
        this->NumberOfShapeValuesPerFunction = nvs / this->ShapeGradientEntry.NumberOfFunctions;
        this->ShapeValueTuple.resize(nvs);
      }
    }
  }

  void CloneInto(vtkDGOperationStateEntryBase& entry) const override
  {
    if (auto* typedEntry =
          dynamic_cast<vtkDGOperationStateEntry<InputIterator, OutputIterator>*>(&entry))
    {
      vtkDGOperationEvaluator<InputIterator, OutputIterator, DOFSharing, SourceType, Modifier,
        ShapeSharing>::prepEntry(*typedEntry, this->OpEntry, this->CellConnectivity,
        this->CellValues, this->SideConnectivity, this->Offset, this->ShapeGradientEntry,
        this->ShapeConnectivity, this->ShapeValues);
    }
    // TODO: else throw exception?
  }

  /// Compute the inner product of this->BasisTuple and this->ValueTuple, storing
  /// the result in the \a tt-th tuple of \a result.
  void InnerProduct(vtkTypeUInt64 tt, OutputIterator& outIter) const
  {
    auto xx(outIter[tt]);
    int nc = static_cast<int>(xx.size());
    // Zero out the tuple:
    for (int ii = 0; ii < nc; ++ii)
    {
      xx[ii] = 0.;
    }
    // Sum the inner product of the basis and the coefficient values into the tuple:
    for (int ii = 0; ii < this->NumberOfValuesPerFunction; ++ii)
    {
      for (int jj = 0; jj < this->OpEntry.OperatorSize; ++jj)
      {
        for (int kk = 0; kk < this->OpEntry.NumberOfFunctions; ++kk)
        {
          xx[ii * this->OpEntry.OperatorSize + jj] +=
            this->BasisTuple[kk * this->OpEntry.OperatorSize + jj] *
            this->ValueTuple[kk * this->NumberOfValuesPerFunction + ii];
        }
      }
    }
  }

  /// Compute the inner product of this->ShapeBasisTuple and this->ShapeValueTuple, storing
  /// the result in this->Jacobian.
  void ShapeInnerProduct() const
  {
    constexpr int nc = 9; // TODO: Use cell dimension instead (9 for 3-d, 4 for 2-d)?
    assert(nc == this->ShapeGradientEntry.OperatorSize * this->NumberOfShapeValuesPerFunction);
    // Zero out the tuple:
    for (int ii = 0; ii < nc; ++ii)
    {
      this->Jacobian[ii] = 0.;
    }
    for (int ii = 0; ii < this->NumberOfShapeValuesPerFunction; ++ii)
    {
      for (int jj = 0; jj < this->ShapeGradientEntry.OperatorSize; ++jj)
      {
        for (int kk = 0; kk < this->ShapeGradientEntry.NumberOfFunctions; ++kk)
        {
          this->Jacobian[jj + this->NumberOfShapeValuesPerFunction * ii] +=
            this->ShapeBasisTuple[kk * this->ShapeGradientEntry.OperatorSize + jj] *
            this->ShapeValueTuple[kk * this->NumberOfShapeValuesPerFunction + ii];
        }
      }
    }
  }

  ///  Compute the shape-attribute Jacobian matrix, storing it in this->Jacobian.
  void ComputeJacobian() const
  {
    if (ShapeSharing == SharedDOF)
    {
      if (this->LastShapeCellId != this->LastCellId)
      {
        this->ShapeConnectivity->GetUnsignedTuple(this->LastCellId, this->ShapeConnTuple.data());
        std::size_t nc = this->ShapeConnTuple.size();
        int nv = this->ShapeValues->GetNumberOfComponents();
        for (std::size_t jj = 0; jj < nc; ++jj)
        {
          this->ShapeValues->GetTuple(
            this->ShapeConnTuple[jj], this->ShapeValueTuple.data() + nv * jj);
        }
        this->LastShapeCellId = this->LastCellId;
      }
    }
    else if (ShapeSharing == Discontinuous)
    {
      if (this->LastShapeCellId != this->LastCellId)
      {
        this->ShapeValues->GetTuple(this->LastCellId, this->ShapeValueTuple.data());
        this->LastShapeCellId = this->LastCellId;
      }
    }
    else
    {
      throw std::logic_error("Invalid shape DOF-sharing enumerant.");
    }
    this->ShapeGradientEntry.Op(this->RST, this->ShapeBasisTuple);
    this->ShapeInnerProduct();
  }

  // Compute the inverse Jacobian and multiply the \a ii-th tuple of result by it.
  //
  // This performs the multiplication in place.
  void ApplyInverseJacobian(vtkTypeUInt64 ii, OutputIterator& outIter) const
  {
    this->ComputeJacobian();
    // Invert Jacobian and multiply result's ii-th tuple by it.
    std::array<double, 9> inverseJacobian;
    // Transpose required; oddly, ApplyScaledJacobian and ApplyInverseJacobian
    // cannot both use the same Jacobian matrix.
    vtkMatrix3x3::Transpose(this->Jacobian.data(), this->Jacobian.data());
    vtkMatrix3x3::Invert(this->Jacobian.data(), inverseJacobian.data());
    auto rr(outIter[ii]);
    const int nc = static_cast<int>(rr.size());
    if (nc % 3 != 0)
    {
      throw std::logic_error("Jacobian must apply to vector or matrix values.");
    }
    for (int vv = 0; vv < nc / 3; ++vv)
    {
      vtkTypeUInt64 mm = 3 * vv;
      double* vec = rr.data() + mm;
      vtkMatrix3x3::MultiplyPoint(inverseJacobian.data(), vec, vec);
    }
  }

  // Compute the Jacobian scaled by its determinant; multiply the \a ii-th tuple of result by it.
  //
  // This performs the multiplication in place.
  void ApplyScaledJacobian(vtkTypeUInt64 ii, OutputIterator& outIter) const
  {
    this->ComputeJacobian();
    // Compute the Jacobian and multiply result's ii-th tuple
    // by the Jacobian normalized by its determinant.
    double norm = 1.0 / vtkMatrix3x3::Determinant(this->Jacobian.data());
    std::array<double, 3> vec;
    auto rr(outIter[ii]);
    const int nc = static_cast<int>(rr.size());
    if (nc % 3 != 0)
    {
      throw std::logic_error("Jacobian must apply to vector or matrix values.");
    }
    for (int vv = 0; vv < nc / 3; ++vv)
    {
      vtkTypeUInt64 mm = 3 * vv;
      vec = { { rr[mm], rr[mm + 1], rr[mm + 2] } };
      // TODO: Is this J^T * vec? or J*vec? Depends on row-major vs column-major Jacobian.
      // clang-format off
      rr[mm    ] = norm * (this->Jacobian[0] * vec[0] + this->Jacobian[1] * vec[1] + this->Jacobian[2] * vec[2]);
      rr[mm + 1] = norm * (this->Jacobian[3] * vec[0] + this->Jacobian[4] * vec[1] + this->Jacobian[5] * vec[2]);
      rr[mm + 2] = norm * (this->Jacobian[6] * vec[0] + this->Jacobian[7] * vec[1] + this->Jacobian[8] * vec[2]);
      // clang-format on
    }
  }

  void operator()(
    InputIterator& inIter, OutputIterator& outIter, vtkTypeUInt64 begin, vtkTypeUInt64 end) const
  {
    vtkTypeUInt64 currId;
    if (DOFSharing == SharedDOF && SourceType == Sides)
    {
      for (vtkTypeUInt64 ii = begin; ii != end; ++ii)
      {
        currId = inIter.GetCellId(ii);
        this->SideConnectivity->GetUnsignedTuple(currId - this->Offset, this->SideTuple.data());
        currId = this->SideTuple[0];
        if (this->LastCellId != currId)
        {
          this->CellConnectivity->GetUnsignedTuple(currId, this->ConnTuple.data());
          std::size_t nc = this->ConnTuple.size();
          int nv = this->CellValues->GetNumberOfComponents();
          for (std::size_t jj = 0; jj < nc; ++jj)
          {
            this->CellValues->GetTuple(this->ConnTuple[jj], this->ValueTuple.data() + nv * jj);
          }
          this->LastCellId = currId;
        }
        auto param = inIter.GetParameter(ii);
        for (int jj = 0; jj < 3; ++jj)
        {
          this->RST[jj] = param[jj];
        }
        this->OpEntry.Op(this->RST, this->BasisTuple);
        this->InnerProduct(ii, outIter);
        if (Modifier == InverseJacobian)
        {
          this->ApplyInverseJacobian(ii, outIter);
        }
        else if (Modifier == ScaledJacobian)
        {
          this->ApplyScaledJacobian(ii, outIter);
        }
      }
    }
    else if (DOFSharing == SharedDOF && SourceType == Cells)
    {
      for (vtkTypeUInt64 ii = begin; ii != end; ++ii)
      {
        currId = inIter.GetCellId(ii);
        if (this->LastCellId != currId)
        {
          // NB: We could ask for currId - this->Offset here, but perhaps we should
          //     assume this->Offset will always be 0 for the CellSpec?
          this->CellConnectivity->GetUnsignedTuple(currId, this->ConnTuple.data());
          std::size_t nc = this->ConnTuple.size();
          int nv = this->CellValues->GetNumberOfComponents();
          for (std::size_t jj = 0; jj < nc; ++jj)
          {
            this->CellValues->GetTuple(this->ConnTuple[jj], this->ValueTuple.data() + nv * jj);
          }
          this->LastCellId = currId;
        }
        auto param = inIter.GetParameter(ii);
        for (int jj = 0; jj < 3; ++jj)
        {
          this->RST[jj] = param[jj];
        }
        this->OpEntry.Op(this->RST, this->BasisTuple);
        this->InnerProduct(ii, outIter);
        if (Modifier == InverseJacobian)
        {
          this->ApplyInverseJacobian(ii, outIter);
        }
        else if (Modifier == ScaledJacobian)
        {
          this->ApplyScaledJacobian(ii, outIter);
        }
      }
    }
    else if (DOFSharing == Discontinuous && SourceType == Sides)
    {
      for (vtkTypeUInt64 ii = begin; ii != end; ++ii)
      {
        currId = inIter.GetCellId(ii);
        this->SideConnectivity->GetUnsignedTuple(currId - this->Offset, this->SideTuple.data());
        currId = this->SideTuple[0];
        if (this->LastCellId != currId)
        {
          this->CellValues->GetTuple(currId, this->ValueTuple.data());
          this->LastCellId = currId;
        }
        auto param = inIter.GetParameter(ii);
        for (int jj = 0; jj < 3; ++jj)
        {
          this->RST[jj] = param[jj];
        }
        this->OpEntry.Op(this->RST, this->BasisTuple);
        this->InnerProduct(ii, outIter);
        if (Modifier == InverseJacobian)
        {
          this->ApplyInverseJacobian(ii, outIter);
        }
        else if (Modifier == ScaledJacobian)
        {
          this->ApplyScaledJacobian(ii, outIter);
        }
      }
    }
    else // DOFSharing == Discontinuous && SourceType == Cells
    {
      for (vtkTypeUInt64 ii = begin; ii != end; ++ii)
      {
        currId = inIter.GetCellId(ii);
        // NB: We could subtract this->Offset from currId, but assume for
        //     now that CellSpec always has an offset of 0.
        if (this->LastCellId != currId)
        {
          this->CellValues->GetTuple(currId, this->ValueTuple.data());
          this->LastCellId = currId;
        }
        auto param = inIter.GetParameter(ii);
        for (int jj = 0; jj < 3; ++jj)
        {
          this->RST[jj] = param[jj];
        }
        this->OpEntry.Op(this->RST, this->BasisTuple);
        this->InnerProduct(ii, outIter);
        if (Modifier == InverseJacobian)
        {
          this->ApplyInverseJacobian(ii, outIter);
        }
        else if (Modifier == ScaledJacobian)
        {
          this->ApplyScaledJacobian(ii, outIter);
        }
      }
    }
  }

  /// Prepare an \a entry for evaluating \a op with the given data arrays
  /// and class template parameters.
  ///
  /// This creates a new instance of vtkDGOperationEvaluator with the
  /// given template parameters and passed arrays, then assigns its
  /// ownership (via unique pointer) to the \a entry.
  static void prepEntry(vtkDGOperationStateEntry<InputIterator, OutputIterator>& entry,
    vtkDGOperatorEntry op, vtkDataArray* conn, vtkDataArray* values, vtkDataArray* sides,
    vtkTypeUInt64 offset, vtkDGOperatorEntry shapeGradient = vtkDGOperatorEntry(),
    vtkDataArray* shapeConnectivity = nullptr, vtkDataArray* shapeValues = nullptr)
  {
    entry.State = std::unique_ptr<vtkDGOperationState>(new vtkDGOperationEvaluator<InputIterator,
      OutputIterator, DOFSharing, SourceType, Modifier, ShapeSharing>(
      op, conn, values, sides, offset, shapeGradient, shapeConnectivity, shapeValues));
    entry.Function = [&entry](InputIterator& inIter, OutputIterator& outIter, vtkTypeUInt64 begin,
                       vtkTypeUInt64 end)
    {
      auto* eval = reinterpret_cast<vtkDGOperationEvaluator<InputIterator, OutputIterator,
        DOFSharing, SourceType, Modifier, ShapeSharing>*>(entry.State.get());
      return (*eval)(inIter, outIter, begin, end);
    };
  }
};

#endif // vtkDGOperationEvaluator_h
// VTK-HeaderTest-Exclude: vtkDGOperationEvaluator.h
