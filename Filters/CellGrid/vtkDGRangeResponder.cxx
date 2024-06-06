// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGRangeResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridRangeQuery.h"
#include "vtkDGHex.h"
#include "vtkDGInterpolateCalculator.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDGTet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"

#include <algorithm>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{

/// Fetch a single cell's attribute DOF values.
template <bool DOFSharing>
struct FetchCellDOF
{
};

template <>
struct FetchCellDOF<false>
{
  vtkDataArray* Values{ nullptr };

  FetchCellDOF(vtkDataArray* vals, vtkDataArray* conn)
    : Values(vals)
  {
    (void)conn;
  }

  void operator()(vtkIdType ii, std::vector<double>& tuple)
  {
    // tuple.resize(this->Values->GetNumberOfComponents());
    this->Values->GetTuple(ii, tuple.data());
  }
};

template <>
struct FetchCellDOF<true>
{
  vtkDataArray* Values{ nullptr };
  vtkDataArray* Conn{ nullptr };
  vtkIdType Stride{ 0 };
  std::vector<vtkTypeInt64> ConnTuple;

  FetchCellDOF(vtkDataArray* vals, vtkDataArray* conn)
    : Values(vals)
    , Conn(conn)
    , Stride(vals->GetNumberOfComponents())
  {
    this->ConnTuple.resize(conn->GetNumberOfComponents());
  }

  void operator()(vtkIdType ii, std::vector<double>& tuple)
  {
    this->Conn->GetIntegerTuple(ii, this->ConnTuple.data());
    auto valIt = tuple.begin();
    for (const auto& valId : this->ConnTuple)
    {
      this->Values->GetTuple(valId, &(*valIt));
      valIt += this->Stride;
    }
  }
};

/// Track whether exceptional values (±∞, NaN) have been encountered.
struct ExceptionalValues
{
  bool HasPositiveInfinity;
  bool HasNegativeInfinity;
  bool HasNaN;

  ExceptionalValues& operator*=(const ExceptionalValues& other)
  {
    this->HasPositiveInfinity |= other.HasPositiveInfinity;
    this->HasNegativeInfinity |= other.HasNegativeInfinity;
    this->HasNaN |= other.HasNaN;
    return *this;
  }

  /// Return true if any exceptional value was encountered.
  operator bool() const
  {
    return this->HasPositiveInfinity || this->HasNegativeInfinity || this->HasNaN;
  }
};

template <bool DOFSharing, bool Exceptions>
struct BaseRangeWorker
{
  // Input
  vtkDGCell* Cell{ nullptr };
  vtkCellAttribute* Attribute{ nullptr };

  // Thread-local
  vtkSMPThreadLocal<std::vector<double>> TLCellRange;
  vtkSMPThreadLocal<std::vector<double>> TLRange;
  vtkSMPThreadLocal<ExceptionalValues> TLExcept;

  // Output
  std::vector<double> ReducedRange;
  ExceptionalValues ReducedExcept;

  BaseRangeWorker(vtkDGCell* dgCell, vtkCellAttribute* attrib)
    : Cell(dgCell)
    , Attribute(attrib)
  {
  }

  void PrepRange(std::vector<double>& range)
  {
    int nn = this->Attribute->GetNumberOfComponents();
    range.resize(
      2 * (nn + 1)); // 2 * nn for components plus the L₂ norm. (L₁ can be computed later.)
    for (int ii = 0; ii < nn + 1; ++ii)
    {
      range[2 * ii] = VTK_DOUBLE_MAX;
      range[2 * ii + 1] = VTK_DOUBLE_MIN;
    }
  }

  void PrepExcept(ExceptionalValues& except)
  {
    except.HasPositiveInfinity = false;
    except.HasNegativeInfinity = false;
    except.HasNaN = false;
  }

  void Initialize()
  {
    auto& cellRange = this->TLCellRange.Local();
    auto& range = this->TLRange.Local();
    auto& except = this->TLExcept.Local();
    this->PrepRange(cellRange);
    this->PrepRange(range);
    this->PrepExcept(except);
  }

  /// Merge range \a bb into range \a aa (which may modify \a aa).
  ///
  /// If the ZeroCrossing template-parameter is true, this function
  /// will return true if all components of \a bb span the origin.
  /// This can be used to account for the L₂ norm cells that span
  /// the origin but have no points near it.
  template <bool ZeroCrossing = false>
  bool MergeRanges(int nn, std::vector<double>& aa, const std::vector<double>& bb) const
  {
    bool crossesOrigin = true;
    for (int ii = 0; ii < nn; ++ii)
    {
      if (ZeroCrossing && std::signbit(bb[2 * ii]) != std::signbit(bb[2 * ii + 1]))
      {
        crossesOrigin = false;
      }
      aa[2 * ii] = std::min(bb[2 * ii], aa[2 * ii]);
      aa[2 * ii + 1] = std::max(bb[2 * ii + 1], aa[2 * ii + 1]);
    }
    return crossesOrigin;
  }

  void Reduce()
  {
    this->PrepRange(this->ReducedRange);
    int nn = this->Attribute->GetNumberOfComponents();
    for (const auto& range : this->TLRange)
    {
      this->template MergeRanges<false>(nn + 1, this->ReducedRange, range);
    }

    this->PrepExcept(this->ReducedExcept);
    for (const auto& except : this->TLExcept)
    {
      this->ReducedExcept.HasPositiveInfinity |= except.HasPositiveInfinity;
      this->ReducedExcept.HasNegativeInfinity |= except.HasNegativeInfinity;
      this->ReducedExcept.HasNaN |= except.HasNaN;
    }
  }

  /// Call this upon completion of the worker to add its range information to the query.
  void CacheRanges(vtkCellGridRangeQuery* request)
  {
    int nn = static_cast<int>(this->ReducedRange.size() / 2) - 1;
    if (nn == 0)
    {
      return;
    }

    // Add the per-component ranges while computing the L₁ norm
    std::array<double, 2> l1Norm{ this->ReducedRange[0], this->ReducedRange[1] };
    for (int ii = 0; ii < nn; ++ii)
    {
      request->AddRange(ii, { this->ReducedRange[2 * ii], this->ReducedRange[2 * ii + 1] });
      if (l1Norm[0] > this->ReducedRange[2 * ii])
      {
        l1Norm[0] = this->ReducedRange[2 * ii];
      }
      if (l1Norm[1] < this->ReducedRange[2 * ii + 1])
      {
        l1Norm[1] = this->ReducedRange[2 * ii + 1];
      }
    }
    // Add the L₁ norm to the request.
    request->AddRange(-1, l1Norm);
    // Add the L₂ norm from the end of ReducedRange
    request->AddRange(-2, { this->ReducedRange[2 * nn], this->ReducedRange[2 * nn + 1] });
  }
};

template <bool DOFSharing, bool Exceptions>
struct CoefficientRangeWorker : public BaseRangeWorker<DOFSharing, Exceptions>
{
  // Input
  vtkDataArray* AttVals;
  vtkDataArray* AttConn;
  vtkDGOperatorEntry BasisOp;

  // Thread-local
  vtkSMPThreadLocal<std::vector<double>> TLTuple;
  vtkSMPThreadLocal<std::vector<double>> TLEval;

  // Output
  std::vector<double> ReducedRange;
  ExceptionalValues ReducedExcept;

  CoefficientRangeWorker(
    vtkDGCell* dgCell, vtkCellAttribute* attrib, const vtkCellAttribute::CellTypeInfo& fieldInfo)
    : BaseRangeWorker<DOFSharing, Exceptions>(dgCell, attrib)
  {
    const auto& arraysByRole = fieldInfo.ArraysByRole;
    auto valIt = arraysByRole.find("values"_token);
    if (valIt == arraysByRole.end())
    {
      vtkGenericWarningMacro("Attribute \"" << attrib->GetName().Data() << "\" missing values.");
      throw std::runtime_error("No coefficient data.");
    }

    this->AttVals = vtkDataArray::SafeDownCast(valIt->second);
    if (!this->AttVals)
    {
      vtkGenericWarningMacro(
        "Attribute \"" << attrib->GetName().Data() << "\" has improper values.");
      throw std::runtime_error("Coefficient data is wrong type.");
    }

    if (fieldInfo.DOFSharing.IsValid())
    {
      auto connIt = arraysByRole.find("connectivity"_token);
      if (connIt == arraysByRole.end())
      {
        vtkGenericWarningMacro(
          "Attribute \"" << attrib->GetName().Data() << "\" missing connectivity.");
        throw std::runtime_error("No connectivity data.");
      }
      this->AttConn = vtkDataArray::SafeDownCast(connIt->second);
      if (!this->AttConn || !this->AttConn->IsIntegral())
      {
        vtkGenericWarningMacro(
          "Attribute \"" << attrib->GetName().Data() << "\" improper connectivity.");
        throw std::runtime_error("Connectivity data is wrong type.");
      }
    }

    this->BasisOp = dgCell->GetOperatorEntry("Basis"_token, fieldInfo);
    if (!this->BasisOp)
    {
      vtkGenericWarningMacro(
        "No basis for \"" << attrib->GetName().Data() << "\" on " << dgCell->GetClassName() << ".");
      throw std::runtime_error("Connectivity data is wrong type.");
    }
  }

  void Initialize()
  {
    BaseRangeWorker<DOFSharing, Exceptions>::Initialize();
    auto& tuple = this->TLTuple.Local();
    auto& eval = this->TLEval.Local();
    if (DOFSharing)
    {
      tuple.resize(this->AttVals->GetNumberOfComponents() * this->AttConn->GetNumberOfComponents());
    }
    else
    {
      tuple.resize(this->AttVals->GetNumberOfComponents());
    }
    eval.resize(this->Attribute->GetNumberOfComponents());
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& cellRange = this->TLCellRange.Local();
    auto& range = this->TLRange.Local();
    auto& except = this->TLExcept.Local();
    auto& tuple = this->TLTuple.Local();

    FetchCellDOF<DOFSharing> dofFetcher(this->AttVals, this->AttConn);
    int cc = this->Attribute->GetNumberOfComponents();
    int tt = static_cast<int>(tuple.size());
    int nn = static_cast<int>(range.size() / 2);
    ExceptionalValues dofExcept;

    // Loop over all cells we are assigned:
    for (vtkIdType ii = begin; ii < end; ++ii)
    {
      dofFetcher(ii, tuple);
      this->PrepRange(cellRange);
      // Loop over all DOF in cell ii:
      for (int jj = 0; jj < tt; jj += cc)
      {
        // Loop over all coefficients for DOF jj:
        double magnitudeSquared = 0.;
        this->PrepExcept(dofExcept);
        for (int kk = 0; kk < cc; ++kk)
        {
          double compValue = tuple[jj + kk];
          if (Exceptions && std::isinf(compValue))
          {
            if (compValue < 0.)
            {
              except.HasNegativeInfinity = true;
            }
            else
            {
              except.HasPositiveInfinity = true;
            }
          }
          else if (Exceptions && std::isnan(compValue))
          {
            except.HasNaN = true;
          }
          else
          {
            cellRange[2 * kk] = std::min(cellRange[2 * kk], compValue);
            cellRange[2 * kk + 1] = std::max(cellRange[2 * kk + 1], compValue);
            magnitudeSquared += compValue * compValue;
          }
        }
        if (!dofExcept)
        {
          // Tuple had no exceptional values, so we can compute its magnitude.
          double magnitude = std::sqrt(magnitudeSquared);
          range[2 * cc] = std::min(range[2 * cc], magnitude);
          range[2 * cc + 1] = std::max(range[2 * cc + 1], magnitude);
        }
        else
        {
          // Some exceptional values were encountered; update the thread state.
          except *= dofExcept;
        }
      }
      // Finally, if any component-range for the cell crossed zero, then
      // we should ensure the L₂ norm includes zero. This way, if a
      // cell spans the origin, we capture its continuous range properly.
      bool crossedOrigin = this->template MergeRanges<Exceptions>(nn, range, cellRange);
      if (Exceptions && crossedOrigin)
      {
        range[2 * cc] = 0.;
      }
    }
  }

  // using Reduce = typename BaseRangeWorker<DOFSharing, Exceptions>::Reduce;
};

// This worker is for DeRham (and perhaps other) attributes that require
// evaluation of a vector-valued shape-attribute basis to be combined with
// the attribute's coefficients to produce a valid range.
template <bool DOFSharing, bool Exceptions>
struct EvaluatorRangeWorker : public BaseRangeWorker<DOFSharing, Exceptions>
{
  const vtkCellAttribute::CellTypeInfo& CellAttInfo;

  // The parametric coordinates to evaluate within each cell to approximate the range.
  // For HCURL fields, this is mid-edge points.
  // For HDIV fields, this is mid-face points.
  vtkNew<vtkDoubleArray> Locations;

  vtkSMPThreadLocal<vtkSmartPointer<vtkDGInterpolateCalculator>> TLInterp;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdTypeArray>> TLCellId;
  vtkSMPThreadLocal<vtkSmartPointer<vtkDoubleArray>> TLCellRange;

  EvaluatorRangeWorker(
    vtkDGCell* dgCell, vtkCellAttribute* attrib, const vtkCellAttribute::CellTypeInfo& fieldInfo)
    : BaseRangeWorker<DOFSharing, Exceptions>(dgCell, attrib)
    , CellAttInfo(fieldInfo)
  {
    // For cells of this shape, generate the parametric points at which
    // we will evaluate the attribute in order to bound its range.
    // For now, we used a fixed set of points. For HCURL, we sample
    // mid-edge points (the average parametric coordinate along each
    // side that is a curve). For HDIV, we sample mid-face points
    // (the average parametric coordinate along each side that is a
    // surface).
    this->Locations->SetNumberOfComponents(3);
    int fsDim = 0;
    int numSides = 0;
    if (fieldInfo.FunctionSpace == "HCURL"_token)
    {
      fsDim = 1;
    }
    else if (fieldInfo.FunctionSpace == "HDIV"_token)
    {
      fsDim = dgCell->GetDimension() - 1;
    }
    else if (fieldInfo.FunctionSpace == "HGRAD"_token)
    {
      fsDim = 0;
    }
    else
    {
      vtkGenericWarningMacro("Unhandled function space " << fieldInfo.FunctionSpace.Data() << ".");
      throw std::runtime_error("Unhandled function space " + fieldInfo.FunctionSpace.Data());
    }
    numSides = dgCell->GetNumberOfSidesOfDimension(fsDim);
    this->Locations->SetNumberOfTuples(numSides);
    int nst = dgCell->GetNumberOfSideTypes();
    vtkIdType pp = 0;
    for (int ii = 0; ii < nst; ++ii)
    {
      auto sideRange = dgCell->GetSideRangeForType(ii);
      auto sideShape = dgCell->GetSideShape(sideRange.first);
      int sideDim = vtkDGCell::GetShapeDimension(sideShape);
      if (sideDim < fsDim)
      {
        break;
      }
      else if (sideDim > fsDim)
      {
        continue;
      }
      for (int sideId = sideRange.first; sideId < sideRange.second; ++sideId)
      {
        vtkVector3d ctr = dgCell->GetParametricCenterOfSide(sideId);
        this->Locations->SetTuple(pp, ctr.GetData());
        ++pp;
      }
    }
  }

  void Initialize()
  {
    BaseRangeWorker<DOFSharing, Exceptions>::Initialize();

    auto& calc = this->TLInterp.Local();
    if (!calc)
    {
      auto dgc = vtkSmartPointer<vtkDGInterpolateCalculator>::New();
      auto prep = dgc->PrepareForGrid(this->Cell, this->Attribute);
      calc = vtkDGInterpolateCalculator::SafeDownCast(prep);
    }

    auto& cellIds = this->TLCellId.Local();
    if (!cellIds)
    {
      cellIds = vtkSmartPointer<vtkIdTypeArray>::New();
    }
    cellIds->SetNumberOfTuples(this->Locations->GetNumberOfTuples());

    auto& cellRange = this->TLCellRange.Local();
    if (!cellRange)
    {
      cellRange = vtkSmartPointer<vtkDoubleArray>::New();
    }
    cellRange->SetNumberOfComponents(this->Attribute->GetNumberOfComponents());
    cellRange->SetNumberOfTuples(this->Locations->GetNumberOfTuples());
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& calc = this->TLInterp.Local();
    auto& cellIds = this->TLCellId.Local();
    auto& cellRange = this->TLCellRange.Local();
    vtkIdType nn = cellRange->GetNumberOfTuples();
    int cc = cellRange->GetNumberOfComponents();
    ExceptionalValues dofExcept;
    auto& range = this->TLRange.Local();
    auto& except = this->TLExcept.Local();
    for (vtkIdType cell = begin; cell < end; ++cell)
    {
      cellIds->FillComponent(0, cell);
      calc->Evaluate(cellIds, this->Locations, cellRange);
      for (int ii = 0; ii < nn; ++ii)
      {
        // Loop over all coefficients for DOF jj:
        double magnitudeSquared = 0.;
        this->PrepExcept(dofExcept);
        double* tuple = cellRange->GetTuple(ii);
        for (int kk = 0; kk < cc; ++kk)
        {
          double compValue = tuple[kk];
          if (Exceptions && std::isinf(compValue))
          {
            if (compValue < 0.)
            {
              except.HasNegativeInfinity = true;
            }
            else
            {
              except.HasPositiveInfinity = true;
            }
          }
          else if (Exceptions && std::isnan(compValue))
          {
            except.HasNaN = true;
          }
          else
          {
            range[2 * kk] = std::min(range[2 * kk], compValue);
            range[2 * kk + 1] = std::max(range[2 * kk + 1], compValue);
            magnitudeSquared += compValue * compValue;
          }
        }
        if (!dofExcept)
        {
          // Tuple had no exceptional values, so we can compute its magnitude.
          double magnitude = std::sqrt(magnitudeSquared);
          range[2 * cc] = std::min(range[2 * cc], magnitude);
          range[2 * cc + 1] = std::max(range[2 * cc + 1], magnitude);
        }
        else
        {
          // Some exceptional values were encountered; update the thread state.
          except *= dofExcept;
        }
      }
    }
  }
};

} // anonymous namespace

vtkStandardNewMacro(vtkDGRangeResponder);

template <bool DOFSharing, bool FiniteRange>
bool vtkDGRangeResponder::ConstantRange(vtkDGCell* dgCell, vtkCellAttribute* attribute,
  const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDataArray* values,
  vtkCellGridRangeQuery* request)
{
  (void)values;
  // NB: This will compute the range of the cells (not sides).
  vtkIdType numTuples = dgCell->GetCellSpec().Connectivity->GetNumberOfTuples();
  CoefficientRangeWorker<DOFSharing, FiniteRange> computeRange(dgCell, attribute, cellTypeInfo);
  vtkSMPTools::For(0, numTuples, computeRange);
  computeRange.CacheRanges(request);
  return true;
}

template <bool DOFSharing, bool FiniteRange>
bool vtkDGRangeResponder::HGradRange(vtkDGCell* dgCell, vtkCellAttribute* attribute,
  const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDataArray* values,
  vtkCellGridRangeQuery* request)
{
  (void)values;
  // NB: This will compute the range of the cells (not sides).
  vtkIdType numTuples = dgCell->GetCellSpec().Connectivity->GetNumberOfTuples();
  CoefficientRangeWorker<DOFSharing, FiniteRange> computeRange(dgCell, attribute, cellTypeInfo);
  vtkSMPTools::For(0, numTuples, computeRange);
  computeRange.CacheRanges(request);
  return true;
}

template <bool DOFSharing, bool FiniteRange>
bool vtkDGRangeResponder::HCurlRange(vtkDGCell* dgCell, vtkCellAttribute* attribute,
  const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDataArray* values,
  vtkCellGridRangeQuery* request)
{
  (void)values;
  // NB: This will compute the range of the cells (not sides).
  vtkIdType numTuples = dgCell->GetCellSpec().Connectivity->GetNumberOfTuples();
  EvaluatorRangeWorker<DOFSharing, FiniteRange> computeRange(dgCell, attribute, cellTypeInfo);
  vtkSMPTools::For(0, numTuples, computeRange);
  computeRange.CacheRanges(request);
  return true;
}

template <bool DOFSharing, bool FiniteRange>
bool vtkDGRangeResponder::HDivRange(vtkDGCell* dgCell, vtkCellAttribute* attribute,
  const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDataArray* values,
  vtkCellGridRangeQuery* request)
{
  (void)values;
  // NB: This will compute the range of the cells (not sides).
  vtkIdType numTuples = dgCell->GetCellSpec().Connectivity->GetNumberOfTuples();
  CoefficientRangeWorker<DOFSharing, FiniteRange> computeRange(dgCell, attribute, cellTypeInfo);
  vtkSMPTools::For(0, numTuples, computeRange);
  computeRange.CacheRanges(request);
  return true;
}

#define vtkDGDispatchRange(rangeMethod, dgCell, attrib, cellAttInfo, vals, request)                \
  if (dofSharing)                                                                                  \
  {                                                                                                \
    if (isIntegral)                                                                                \
    {                                                                                              \
      ok = this->rangeMethod<true, true>(dgCell, attrib, cellAttInfo, vals, request);              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      ok = this->rangeMethod<true, false>(dgCell, attrib, cellAttInfo, vals, request);             \
    }                                                                                              \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    if (isIntegral)                                                                                \
    {                                                                                              \
      ok = this->rangeMethod<false, true>(dgCell, attrib, cellAttInfo, vals, request);             \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      ok = this->rangeMethod<false, false>(dgCell, attrib, cellAttInfo, vals, request);            \
    }                                                                                              \
  }

bool vtkDGRangeResponder::Query(
  vtkCellGridRangeQuery* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)cellType;
  (void)caches;

  std::string cellTypeName = cellType->GetClassName();

  auto* attribute = request->GetCellAttribute();
  if (!attribute)
  {
    return false;
  }

  vtkStringToken cellTypeToken(cellTypeName);
  auto cellTypeInfo = attribute->GetCellTypeInfo(cellTypeToken);

  static std::unordered_set<vtkStringToken> constantFS{ { "constant"_token, "CONSTANT"_token,
    "Constant"_token } };
  static std::unordered_set<vtkStringToken> pointBasedFS{ { "Lagrange"_token, "lagrange"_token,
    "HGRAD"_token, "HGrad"_token, "Hgrad"_token, "hgrad"_token } };
  static std::unordered_set<vtkStringToken> edgeBasedFS{ { "HCURL"_token, "HCurl"_token,
    "Hcurl"_token, "hcurl"_token } };
  static std::unordered_set<vtkStringToken> faceBasedFS{ { "HDIV"_token, "HDiv"_token, "Hdiv"_token,
    "hdiv"_token } };

  auto it = cellTypeInfo.ArraysByRole.find("values");
  if (it == cellTypeInfo.ArraysByRole.end() || !vtkDataArray::SafeDownCast(it->second))
  {
    vtkErrorMacro("No array in \"values\" role or the array was not a vtkDataArray.");
    return false;
  }
  auto* values = vtkDataArray::SafeDownCast(it->second);
  bool dofSharing = cellTypeInfo.DOFSharing.IsValid();
  bool isIntegral = values->IsIntegral();
  bool ok = true;

  //clang-format off
  if (constantFS.find(cellTypeInfo.FunctionSpace) != constantFS.end())
  {
    vtkDGDispatchRange(
      ConstantRange, vtkDGCell::SafeDownCast(cellType), attribute, cellTypeInfo, values, request);
  }
  if (pointBasedFS.find(cellTypeInfo.FunctionSpace) != pointBasedFS.end())
  {
    vtkDGDispatchRange(
      HGradRange, vtkDGCell::SafeDownCast(cellType), attribute, cellTypeInfo, values, request);
  }
  else if (edgeBasedFS.find(cellTypeInfo.FunctionSpace) != edgeBasedFS.end())
  {
    vtkDGDispatchRange(
      HCurlRange, vtkDGCell::SafeDownCast(cellType), attribute, cellTypeInfo, values, request);
  }
  else if (faceBasedFS.find(cellTypeInfo.FunctionSpace) != faceBasedFS.end())
  {
    vtkDGDispatchRange(
      HDivRange, vtkDGCell::SafeDownCast(cellType), attribute, cellTypeInfo, values, request);
  }
  //clang-format on

  if (!ok)
  {
    vtkWarningMacro("Unsupported function space \"" << cellTypeInfo.FunctionSpace.Data() << "\".");
    return false;
  }
  return true;
#if 0
  if (!cellTypeInfo.DOFSharing.IsValid())
  {
    // Discontinuous (non-shared) cell data.
    if (edgeBasedFS.find(cellTypeInfo.FunctionSpace) != edgeBasedFS.end() ||
      faceBasedFS.find(cellTypeInfo.FunctionSpace) != faceBasedFS.end())
    {
      std::cout << "DG range approx \"" << attribute->GetName().Data() << "\"\n";
      return request->GetFiniteRange() ?
        this->HCurl< true, false>(attribute, nullptr, vals, request) :
        this->HCurl<false, false>(attribute, nullptr, vals, request);
    }
    else
    {
      // We can ignore the connectivity and just iterate over components in the per-cell values array.
      // TODO: FIXME: For HDIV/HCURL, we need to multiply each value by the
      //       magnitude of the inverse Jacobian at its parametric center.
      return request->GetFiniteRange() ? this->DiscontinuousLagrange<true>(attribute, vals, request)
                                       : this->DiscontinuousLagrange<false>(attribute, vals, request);
  }
  else
  {
    // Continuous (shared) cell data.
    if (pointBasedFS.find(cellTypeInfo.FunctionSpace) != pointBasedFS.end())
    {
      // Use the connectivity to index into the per-corner values array.
      return request->GetFiniteRange()
        ? this->ContinuousLagrange<true>(attribute, conn, vals, request)
        : this->ContinuousLagrange<false>(attribute, conn, vals, request);
    }
    else if (edgeBasedFS.find(cellTypeInfo.FunctionSpace) != edgeBasedFS.end())
    {
      // TODO: FIXME: Properly handle. Need to multiply each value by the
      // magnitude of the inverse Jacobian at its parametric center.
      return request->GetFiniteRange()
        ? this->ContinuousLagrange<true>(attribute, conn, vals, request)
        : this->ContinuousLagrange<false>(attribute, conn, vals, request);
    }
    else if (faceBasedFS.find(cellTypeInfo.FunctionSpace) != faceBasedFS.end())
    {
      // TODO: FIXME: Properly handle. Need to multiply each value by the
      // magnitude of the inverse Jacobian at its parametric center.
      return request->GetFiniteRange()
        ? this->ContinuousLagrange<true>(attribute, conn, vals, request)
        : this->ContinuousLagrange<false>(attribute, conn, vals, request);
    }
    else
    {
      vtkWarningMacro("Unsupported function space \"" << cellTypeInfo.FunctionSpace.Data() << "\".");
      return false;
    }
  }
#endif
}

#if 0
template<bool FiniteRange, bool DOFSharing>
bool vtkDGRangeResponder::HCurl(
  vtkCellAttribute* attribute,
  vtkDataArray* conn,
  vtkDataArray* values,
  vtkCellGridRangeQuery* request)
{
  int componentIndex = request->GetComponent();
  int cc = attribute->GetNumberOfComponents();
  vtkIdType mm = values->GetNumberOfComponents();
  vtkIdType nn = values->GetNumberOfTuples();
  if (nn <= 0)
  {
    return true;
  }

  std::vector<std::array<double, 2>> ranges;
  ranges.resize(attribute->GetNumberOfComponents() + 2,
    { vtkMath::Inf(), -vtkMath::Inf() });
  std::vector<double> tuple;
  tuple.resize(mm);
  // vtkSMPTools::For(0, nn, [&](vtkIdType beg, vtkIdType end)
    {
      // Loop over requested cells:
      // for (vtkIdType ii = beg; ii < end; ++ii)
      for (vtkIdType ii = 0; ii < nn; ++ii)
      {
        // Loop over all collocation points in cell ii:
        values->GetTuple(ii, tuple.data());
        for (int jj = 0; jj < mm; jj += cc)
        {
          // Loop over all components of collocation point jj:
          double magnitudeSquared = 0.;
          bool finiteTuple = true;
          for (int kk = 0; kk < cc; ++kk)
          {
            double val = tuple[jj + kk];
            if (FiniteRange && (vtkMath::IsNan(val) || vtkMath::IsInf(val)))
            {
              finiteTuple = false;
              if (vtkMath::IsNan(val))
              {
                magnitudeSquared = val;
              }
              break;
            }
            magnitudeSquared += val * val;
            if (ranges[kk][0] > val) { ranges[kk][0] = val; }
            if (ranges[kk][1] < val) { ranges[kk][1] = val; }
          }
          if (finiteTuple)
          {
            double magnitude = std::sqrt(magnitudeSquared);
            if (ranges[cc][0] > magnitude) { ranges[cc][0] = magnitude; }
            if (ranges[cc][1] < magnitude) { ranges[cc][1] = magnitude; }
          }
          /*
          else if (!FiniteRange && vtkMath::IsNan(magnitudeSquared))
          {
            // If we find a NaN, the entire range is undefined. Return immediately
            range[0] = magnitudeSquared;
            range[1] = magnitudeSquared;
            request->AddRange(range);
            return true;
          }
          */
        }
      }
    }
  // Compute L₁ norm from components and cache computed ranges for each component.
  std::array<double, 2> l1Norm(ranges[0]);
  for (int kk = 0; kk < cc; ++kk)
  {
    request->AddRange(kk, ranges[kk]);
    if (l1Norm[0] > ranges[kk][0]) { l1Norm[0] = ranges[kk][0]; }
    if (l1Norm[1] < ranges[kk][1]) { l1Norm[1] = ranges[kk][1]; }
  }
  request->AddRange(-1, l1Norm);
  request->AddRange(-2, ranges[cc]);
  return true;
}

template <bool FiniteRange>
bool vtkDGRangeResponder::DiscontinuousLagrange(
  vtkCellAttribute* attribute, vtkDataArray* values, vtkCellGridRangeQuery* request)
{
  vtkIdType numTuples = values->GetNumberOfTuples();
  if (values->IsIntegral())
  {
    // Attributes with integer coefficients should not result in NaN or Inf values,
    // so we can omit tests for exceptional values (the second template parameter
    // to CoefficientRangeWorker).
    CoefficientRangeWorker<false, false> computeRange(attribute, values, nullptr);
    vtkSMPTools::For(0, numTuples, computeRange);
    computeRange.CacheRanges(request);
  }
  else
  {
    CoefficientRangeWorker<false, true> computeRange(attribute, values, nullptr);
    vtkSMPTools::For(0, numTuples, computeRange);
    computeRange.CacheRanges(request);
  }
  return true;
}

template <bool FiniteRange>
bool vtkDGRangeResponder::ContinuousLagrange(vtkCellAttribute* attribute, vtkDataArray* conn,
  vtkDataArray* values, vtkCellGridRangeQuery* request)
{
  vtkIdType numTuples = values->GetNumberOfTuples();
  if (values->IsIntegral())
  {
    // Attributes with integer coefficients should not result in NaN or Inf values,
    // so we can omit tests for exceptional values (the second template parameter
    // to CoefficientRangeWorker).
    CoefficientRangeWorker<true, false> computeRange(attribute, values, conn);
    vtkSMPTools::For(0, numTuples, computeRange);
    computeRange.CacheRanges(request);
  }
  else
  {
    CoefficientRangeWorker<true, true> computeRange(attribute, values, conn);
    vtkSMPTools::For(0, numTuples, computeRange);
    computeRange.CacheRanges(request);
  }
  return true;
}
#endif

VTK_ABI_NAMESPACE_END
