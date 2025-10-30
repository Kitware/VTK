// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellArrayIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <array>
#include <iterator>

namespace
{

// These implementations are for methods that will be deprecated in the future:
namespace deprec
{

// Given a legacy Location, find the corresponding cellId. The location
// *must* refer to a [numPts] entry in the old connectivity array, or the
// returned CellId will be -1.
struct LocationToCellIdFunctor : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(
    OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), vtkIdType location, vtkIdType& cellId) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    const auto offsetsRange = GetRange(offsets);

    // Use a binary-search to find the location:
    auto it = this->BinarySearchOffset(
      offsetsRange.begin(), offsetsRange.end() - 1, static_cast<ValueType>(location));

    cellId = std::distance(offsetsRange.begin(), it);

    if (it == offsetsRange.end() - 1 /* no match found */ ||
      static_cast<vtkIdType>(*it + cellId) != location /* `location` not at cell head */)
    { // Location invalid.
      cellId = -1;
      return;
    }
  }

  template <typename IterT>
  IterT BinarySearchOffset(const IterT& beginIter, const IterT& endIter,
    const typename std::iterator_traits<IterT>::value_type& targetLocation) const
  {
    using ValueType = typename std::iterator_traits<IterT>::value_type;
    using DifferenceType = typename std::iterator_traits<IterT>::difference_type;

    DifferenceType roiSize = std::distance(beginIter, endIter);

    IterT roiBegin = beginIter;
    while (roiSize > 0)
    {
      IterT it = roiBegin;
      const DifferenceType step = roiSize / 2;
      std::advance(it, step);
      // This differs from a generic binary search in the following line:
      // Adding the distance from the start of the array to the current
      // iterator will account for the cellSize entries in the old cell array
      // format, such that curLocation would be the offset in the old style
      // connectivity array.
      const ValueType curLocation = *it + std::distance(beginIter, it);
      if (curLocation < targetLocation)
      {
        roiBegin = ++it;
        roiSize -= step + 1;
      }
      else
      {
        roiSize = step;
      }
    }

    return roiBegin;
  }
};

struct CellIdToLocationFunctor : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(
    OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), vtkIdType cellId, vtkIdType& loc) const
  {
    // Adding the cellId to the offset of that cell id gives us the cell
    // location in the old-style vtkCellArray connectivity array.
    loc = static_cast<vtkIdType>(GetRange(offsets)[cellId]) + cellId;
  }
};

struct GetInsertLocationImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType& insertLoc) const
  {
    // The insert location used to just be the tail of the connectivity array.
    // Compute the equivalent value:
    insertLoc = offsets->GetNumberOfValues() - 1 + conn->GetNumberOfValues();
  }
};

} // end namespace deprec

struct PrintDebugImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, std::ostream& os)
  {
    using ValueType = GetAPIType<OffsetsT>;

    const vtkIdType numCells = offsets->GetNumberOfValues() - 1;
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      os << "cell " << cellId << ": ";

      const auto cellRange = GetCellRange(offsets, conn, cellId);
      for (ValueType ptId : cellRange)
      {
        os << ptId << " ";
      }

      os << "\n";
    }
  }
};

struct InitializeImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn) const
  {
    using ValueType = GetAPIType<OffsetsT>;
    using AccessorType = vtkDataArrayAccessor<OffsetsT>;
    conn->Initialize();
    if (offsets->GetArrayType() == vtkArrayTypes::VTK_AFFINE_ARRAY)
    {
      // AffineArray's Initialize destroys the backend of the offsets array,
      // so we only set the number of values to 0 to preserve the backend.
      offsets->SetNumberOfValues(0);
    }
    else
    {
      offsets->Initialize();
    }
    AccessorType accessor(offsets);
    ValueType firstOffset = 0;
    accessor.InsertNext(firstOffset);
  }
};

struct SqueezeImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn) const
  {
    offsets->Squeeze();
    conn->Squeeze();
  }
};

struct IsValidImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, bool& isValid) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    // Both arrays must be single component
    if (offsets->GetNumberOfComponents() != 1 || conn->GetNumberOfComponents() != 1)
    {
      isValid = false;
      return;
    }

    auto offsetsRange = GetRange(offsets);

    // Offsets must have at least one value, and the first value must be zero
    if (offsetsRange.size() == 0 || *offsetsRange.cbegin() != 0)
    {
      isValid = false;
      return;
    }

    // Values in offsets must not decrease
    auto it = std::adjacent_find(offsetsRange.cbegin(), offsetsRange.cend(),
      [](const ValueType a, const ValueType b) -> bool { return a > b; });
    if (it != offsetsRange.cend())
    {
      isValid = false;
      return;
    }

    // The last value in offsets must be the size of the connectivity array.
    if (conn->GetNumberOfValues() != static_cast<vtkIdType>(*(offsetsRange.cend() - 1)))
    {
      isValid = false;
      return;
    }

    isValid = true;
  }
};

template <typename T>
struct CanConvert : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, bool& canConvert) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    // offsets are sorted, so just check the last value, but we have to compute
    // the full range of the connectivity array.
    auto offsetsRange = GetRange(offsets);
    if (offsetsRange.size() > 0 && !this->CheckValue(ValueType(*(offsetsRange.end() - 1))))
    {
      canConvert = false;
      return;
    }

    auto connRange = GetRange(conn);
    if (connRange.size() > 0)
    {
      auto [minIt, maxIt] = std::minmax_element(connRange.begin(), connRange.end());
      std::array<ValueType, 2> connValueRange = { *minIt, *maxIt };
      if (!this->CheckValue(ValueType(connValueRange[0])) ||
        !this->CheckValue(ValueType(connValueRange[1])))
      {
        canConvert = false;
        return;
      }
    }

    canConvert = true;
  }

  template <typename U>
  bool CheckValue(const U& val) const
  {
    return val == static_cast<U>(static_cast<T>(val));
  }
};

struct ExtractAndInitialize : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT, typename TargetConnectivityT>
  void operator()(OffsetsT* vtkNotUsed(inOffsets), ConnectivityT* inConn, TargetConnectivityT* conn,
    bool& result) const
  {
    result = this->Process(inConn, conn);
  }
  template <class OffsetsT, class ConnectivityT, class TargetOffsetsT, class TargetConnectivityT>
  void operator()(OffsetsT* inOffsets, ConnectivityT* inConn, TargetOffsetsT* outOffsets,
    TargetConnectivityT* outConn, bool& result) const
  {
    result = this->Process(inOffsets, outOffsets) && this->Process(inConn, outConn);
  }

  template <typename SourceArrayT, typename TargetArrayT>
  bool Process(SourceArrayT* src, TargetArrayT* dst) const
  {
    // check if we can shallow copy it first
    if (src->IsA(dst->GetClassName()))
    {
      dst->ShallowCopy(src);
      return true;
    }
    // Check that allocation succeeds:
    if (!dst->Resize(src->GetNumberOfTuples()))
    {
      return false;
    }

    // Copy data:
    dst->DeepCopy(src);

    // Free old memory:
    src->Resize(0);

    return true;
  }
};

struct IsHomogeneousImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(
    OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), vtkIdType& isHomogeneous) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    const vtkIdType numCells = offsets->GetNumberOfValues() - 1;
    if (numCells == 0)
    {
      isHomogeneous = 0;
      return;
    }

    // Initialize using the first cell:
    const vtkIdType firstCellSize = GetCellSize(offsets, 0);

    // Verify the rest:
    auto offsetsRange = GetRange(offsets);
    auto it = std::adjacent_find(offsetsRange.begin() + 1, offsetsRange.end(),
      [&](const ValueType a, const ValueType b) -> bool
      { return static_cast<vtkIdType>(b - a) != firstCellSize; });

    if (it != offsetsRange.end())
    { // Found a cell that doesn't match the size of the first cell:
      isHomogeneous = -1;
      return;
    }

    isHomogeneous = firstCellSize;
  }
};

struct AllocateExactImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType numCells,
    vtkIdType connectivitySize, bool& result) const
  {
    using ValueType = GetAPIType<OffsetsT>;
    using AccessorType = vtkDataArrayAccessor<OffsetsT>;
    result = (offsets->Allocate(numCells + 1) && conn->Allocate(connectivitySize));
    if (result)
    {
      AccessorType accessor(offsets);
      ValueType firstOffset = 0;
      accessor.InsertNext(firstOffset);
    }
  }
};

struct ResizeExactImpl
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType numCells,
    vtkIdType connectivitySize, bool& result) const
  {
    result = offsets->SetNumberOfValues(numCells + 1) && conn->SetNumberOfValues(connectivitySize);
  }
};

struct FindMaxCell // SMP functor
{
  vtkCellArray* CellArray;
  vtkIdType Result{ 0 };
  vtkSMPThreadLocal<vtkIdType> LocalResult;

  FindMaxCell(vtkCellArray* array)
    : CellArray{ array }
  {
  }

  void Initialize() { this->LocalResult.Local() = 0; }

  struct Impl : public vtkCellArray::DispatchUtilities
  {
    template <class OffsetsT, class ConnectivityT>
    void operator()(OffsetsT* offsets, ConnectivityT* vtkNotUsed(conn), vtkIdType cellId,
      const vtkIdType endCellId, vtkIdType& maxCellSize) const
    {
      maxCellSize = 0;
      auto offsetsRange = GetRange(offsets);
      for (; cellId < endCellId; ++cellId)
      {
        maxCellSize =
          std::max<vtkIdType>(maxCellSize, offsetsRange[cellId + 1] - offsetsRange[cellId]);
      }
    }
  };

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkIdType& lval = this->LocalResult.Local();
    vtkIdType maxCellSize;
    this->CellArray->Dispatch(Impl{}, cellId, endCellId, maxCellSize);
    lval = std::max(lval, maxCellSize);
  }

  void Reduce()
  {
    for (const vtkIdType lResult : this->LocalResult)
    {
      this->Result = std::max(this->Result, lResult);
    }
  }
};

struct PrintSelfImpl
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, ostream& os, vtkIndent indent) const
  {
    os << indent << "Offsets:\n";
    offsets->PrintSelf(os, indent.GetNextIndent());
    os << indent << "Connectivity:\n";
    conn->PrintSelf(os, indent.GetNextIndent());
  }
};

struct GetLegacyDataSizeImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType& size) const
  {
    size = offsets->GetNumberOfValues() - 1 + conn->GetNumberOfValues();
  }
};

struct ReverseCellAtIdImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType cellId) const
  {
    auto cellRange = GetCellRange(offsets, conn, cellId);
    std::reverse(cellRange.begin(), cellRange.end());
  }
};

struct ReplaceCellPointAtIdImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType cellId,
    vtkIdType cellPointIndex, vtkIdType newPointId) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    GetRange(conn)[GetBeginOffset(offsets, cellId) + cellPointIndex] =
      static_cast<ValueType>(newPointId);
  }
};

struct ReplaceCellAtIdImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType cellId, vtkIdType cellSize,
    const vtkIdType* cellPoints) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    auto cellRange = GetCellRange(offsets, conn, cellId);

    assert(cellRange.size() == cellSize);
    for (vtkIdType i = 0; i < cellSize; ++i)
    {
      cellRange[i] = static_cast<ValueType>(cellPoints[i]);
    }
  }
};

struct AppendLegacyFormatImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, const vtkIdType* data,
    const vtkIdType len, const vtkIdType ptOffset) const
  {
    using ValueType = GetAPIType<OffsetsT>;
    using OffsetsAccessorType = vtkDataArrayAccessor<OffsetsT>;
    using ConnectivityAccessorType = vtkDataArrayAccessor<ConnectivityT>;
    OffsetsAccessorType offsetsAccessor(offsets);
    ConnectivityAccessorType connAccessor(conn);

    ValueType offset = static_cast<ValueType>(conn->GetNumberOfValues());

    const vtkIdType* const dataEnd = data + len;
    while (data < dataEnd)
    {
      vtkIdType numPts = *data++;
      offset += static_cast<ValueType>(numPts);
      offsetsAccessor.InsertNext(offset);
      while (numPts-- > 0)
      {
        connAccessor.InsertNext(static_cast<ValueType>(*data++ + ptOffset));
      }
    }
  }
};

struct AppendImpl
{
  // Call this signature:
  template <class DstOffsetsT, class DstConnectivityT>
  void operator()(DstOffsetsT* dstOffsets, DstConnectivityT* dstConn, vtkCellArray* src,
    vtkIdType pointOffset) const
  { // dispatch on src:
    src->Dispatch(*this, dstOffsets, dstConn, pointOffset);
  }

  // Above signature calls this operator in Visit:
  template <class SrcOffsetsT, class SrcConnectivityT, class DstOffsetsT, class DstConnectivityT>
  void operator()(SrcOffsetsT* srcOffsets, SrcConnectivityT* srcConn, DstOffsetsT* dstOffsets,
    DstConnectivityT* dstConn, vtkIdType pointOffsets) const
  {
    this->AppendArrayWithOffset(srcOffsets, dstOffsets, dstConn->GetNumberOfValues(), true);
    this->AppendArrayWithOffset(srcConn, dstConn, pointOffsets, false);
  }

  // Assumes both arrays are 1 component. src's data is appended to dst with
  // offset added to each value.
  template <typename SrcArrayT, typename DstArrayT>
  void AppendArrayWithOffset(
    SrcArrayT* srcArray, DstArrayT* dstArray, vtkIdType offset, bool skipFirst) const
  {
    VTK_ASSUME(srcArray->GetNumberOfComponents() == 1);
    VTK_ASSUME(dstArray->GetNumberOfComponents() == 1);

    using SrcValueType = vtk::GetAPIType<SrcArrayT>;
    using DstValueType = vtk::GetAPIType<DstArrayT>;

    const vtkIdType srcSize =
      skipFirst ? srcArray->GetNumberOfValues() - 1 : srcArray->GetNumberOfValues();
    const vtkIdType dstBegin = dstArray->GetNumberOfValues();
    const vtkIdType dstEnd = dstBegin + srcSize;

    // This extends the allocation of dst to ensure we have enough space
    // allocated:
    {
      vtkDataArrayAccessor<DstArrayT> dstAccessor(dstArray);
      dstAccessor.Insert(dstEnd - 1, static_cast<DstValueType>(0));
    }

    const auto srcRange = vtk::DataArrayValueRange<1, vtkIdType>(srcArray, skipFirst ? 1 : 0);
    auto dstRange = vtk::DataArrayValueRange<1, vtkIdType>(dstArray, dstBegin, dstEnd);
    assert(srcRange.size() == dstRange.size());

    const DstValueType dOffset = static_cast<DstValueType>(offset);

    std::transform(srcRange.cbegin(), srcRange.cend(), dstRange.begin(),
      [&](SrcValueType x) -> DstValueType { return static_cast<DstValueType>(x) + dOffset; });
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
vtkCellArray::vtkCellArray()
{
  // Default can be changed, to save memory
  if (vtkCellArray::GetDefaultStorageIs64Bit())
  {
    this->Offsets = vtkSmartPointer<ArrayType64>::New();
    this->Connectivity = vtkSmartPointer<ArrayType64>::New();
    this->GetOffsetsArray64()->InsertNextValue(0);
    this->StorageType = StorageTypes::Int64;
  }
  else
  {
    this->Offsets = vtkSmartPointer<ArrayType32>::New();
    this->Connectivity = vtkSmartPointer<ArrayType32>::New();
    this->GetOffsetsArray32()->InsertNextValue(0);
    this->StorageType = StorageTypes::Int32;
  }
}
vtkCellArray::~vtkCellArray() = default;
vtkStandardNewMacro(vtkCellArray);

#ifdef VTK_USE_64BIT_IDS
bool vtkCellArray::DefaultStorageIs64Bit = true;
#else
bool vtkCellArray::DefaultStorageIs64Bit = false;
#endif

//=================== Begin Legacy Methods ===================================
// These should be deprecated at some point as they are confusing or very slow

//------------------------------------------------------------------------------
vtkIdType vtkCellArray::GetSize()
{
  // We can still compute roughly the same result, so go ahead and do that.
  return this->GetOffsetsArray()->GetSize() + this->GetConnectivityArray()->GetSize();
}

//------------------------------------------------------------------------------
vtkIdType vtkCellArray::GetNumberOfConnectivityEntries()
{
  // We can still compute roughly the same result, so go ahead and do that.
  vtkIdType size;
  this->Dispatch(GetLegacyDataSizeImpl{}, size);
  return size;
}

//------------------------------------------------------------------------------
void vtkCellArray::GetCell(vtkIdType loc, vtkIdType& npts, const vtkIdType*& pts)
{
  vtkIdType cellId;
  this->Dispatch(deprec::LocationToCellIdFunctor{}, loc, cellId);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location.");
    npts = 0;
    pts = nullptr;
    return;
  }

  this->GetCellAtId(cellId, this->TempCell);
  npts = this->TempCell->GetNumberOfIds();
  pts = this->TempCell->GetPointer(0);
}

//------------------------------------------------------------------------------
void vtkCellArray::GetCell(vtkIdType loc, vtkIdList* pts)
{
  vtkIdType cellId;
  this->Dispatch(deprec::LocationToCellIdFunctor{}, loc, cellId);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location.");
    pts->Reset();
    return;
  }

  this->GetCellAtId(cellId, pts);
}

//------------------------------------------------------------------------------
vtkIdType vtkCellArray::GetInsertLocation(int npts)
{
  // It looks like the original implementation of this actually returned the
  // location of the last cell (of size npts), not the current insert location.
  vtkIdType insertLoc;
  this->Dispatch(deprec::GetInsertLocationImpl{}, insertLoc);
  return insertLoc - npts - 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkCellArray::GetTraversalLocation()
{
  vtkIdType loc;
  this->Dispatch(deprec::CellIdToLocationFunctor{}, this->GetTraversalCellId(), loc);
  return loc;
}

//------------------------------------------------------------------------------
vtkIdType vtkCellArray::GetTraversalLocation(vtkIdType npts)
{
  vtkIdType loc;
  this->Dispatch(deprec::CellIdToLocationFunctor{}, this->GetTraversalCellId(), loc);
  return loc - npts - 1;
}

//------------------------------------------------------------------------------
void vtkCellArray::SetTraversalLocation(vtkIdType loc)
{
  vtkIdType cellId;
  this->Dispatch(deprec::LocationToCellIdFunctor{}, loc, cellId);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location, ignoring.");
    return;
  }

  this->SetTraversalCellId(cellId);
}

//------------------------------------------------------------------------------
vtkIdType vtkCellArray::EstimateSize(vtkIdType numCells, int maxPtsPerCell)
{
  return numCells * (1 + maxPtsPerCell);
}

//------------------------------------------------------------------------------
void vtkCellArray::SetNumberOfCells(vtkIdType)
{
  // no-op
}

//------------------------------------------------------------------------------
void vtkCellArray::ReverseCell(vtkIdType loc)
{
  vtkIdType cellId;
  this->Dispatch(deprec::LocationToCellIdFunctor{}, loc, cellId);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location, ignoring.");
    return;
  }

  this->ReverseCellAtId(cellId);
}

//------------------------------------------------------------------------------
void vtkCellArray::ReplaceCell(vtkIdType loc, int npts, const vtkIdType pts[])
{
  vtkIdType cellId;
  this->Dispatch(deprec::LocationToCellIdFunctor{}, loc, cellId);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location, ignoring.");
    return;
  }

  this->ReplaceCellAtId(cellId, static_cast<vtkIdType>(npts), pts);
}

//------------------------------------------------------------------------------
vtkIdTypeArray* vtkCellArray::GetData()
{
  this->ExportLegacyFormat(this->LegacyData);

  return this->LegacyData;
}

//------------------------------------------------------------------------------
// Specify a group of cells.
void vtkCellArray::SetCells(vtkIdType ncells, vtkIdTypeArray* cells)
{
  this->AllocateExact(ncells, cells->GetNumberOfValues() - ncells);
  this->ImportLegacyFormat(cells);
}

//=================== End Legacy Methods =====================================

//------------------------------------------------------------------------------
void vtkCellArray::DeepCopy(vtkAbstractCellArray* ca)
{
  auto other = vtkCellArray::SafeDownCast(ca);
  if (!other)
  {
    vtkErrorMacro("Cannot copy from non-vtkCellArray.");
    return;
  }
  if (other == this)
  {
    return;
  }

  switch (other->GetStorageType())
  {
    case StorageTypes::FixedSizeInt32:
    {
      this->UseFixedSize32BitStorage(1 /*dummy, ImplicitDeepCopy will fix it*/);
      this->GetOffsetsAffineArray32()->ImplicitDeepCopy(
        AffineArrayType32::FastDownCast(other->Offsets));
      this->Connectivity->DeepCopy(other->Connectivity);
      this->Modified();
      break;
    }
    case StorageTypes::FixedSizeInt64:
    {
      this->UseFixedSize64BitStorage(1 /*dummy, ImplicitDeepCopy will fix it*/);
      this->GetOffsetsAffineArray64()->ImplicitDeepCopy(
        AffineArrayType64::FastDownCast(other->Offsets));
      this->Connectivity->DeepCopy(other->Connectivity);
      this->Modified();
      break;
    }
    case StorageTypes::Int32:
    case StorageTypes::Int64:
    case StorageTypes::Generic:
    default:
    {
      this->Offsets = vtk::TakeSmartPointer(other->Offsets->NewInstance());
      this->Offsets->DeepCopy(other->Offsets);
      this->Connectivity = vtk::TakeSmartPointer(other->Connectivity->NewInstance());
      this->Connectivity->DeepCopy(other->Connectivity);
      this->StorageType = other->StorageType;
      this->Modified();
      break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkCellArray::ShallowCopy(vtkAbstractCellArray* ca)
{
  auto other = vtkCellArray::SafeDownCast(ca);
  if (!other)
  {
    vtkErrorMacro("Cannot shallow copy from a non-vtkCellArray.");
    return;
  }
  if (other == this)
  {
    return;
  }

  if (this->Offsets != other->Offsets)
  {
    this->Offsets = other->Offsets;
    this->Modified();
  }
  if (this->Connectivity != other->Connectivity)
  {
    this->Connectivity = other->Connectivity;
    this->Modified();
  }
  this->StorageType = other->StorageType;
}

//------------------------------------------------------------------------------
void vtkCellArray::Append(vtkCellArray* src, vtkIdType pointOffset)
{
  if (src->GetNumberOfCells() > 0)
  {
    this->Dispatch(AppendImpl{}, src, pointOffset);
  }
}

//------------------------------------------------------------------------------
void vtkCellArray::Initialize()
{
  this->Dispatch(InitializeImpl{});

  this->LegacyData->Initialize();
}

//------------------------------------------------------------------------------
vtkCellArrayIterator* vtkCellArray::NewIterator()
{
  vtkCellArrayIterator* iter = vtkCellArrayIterator::New();
  iter->SetCellArray(this);
  iter->GoToFirstCell();
  return iter;
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(vtkTypeInt32Array* offsets, vtkTypeInt32Array* connectivity)
{
  if (offsets->GetNumberOfComponents() != 1 || connectivity->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Only single component arrays may be used for vtkCellArray "
                  "storage.");
    return;
  }

  if (this->Offsets.Get() != offsets)
  {
    this->Offsets = offsets;
    this->Modified();
  }
  if (this->Connectivity.Get() != connectivity)
  {
    this->Connectivity = connectivity;
    this->Modified();
  }
  this->StorageType = StorageTypes::Int32;
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(vtkTypeInt64Array* offsets, vtkTypeInt64Array* connectivity)
{
  if (offsets->GetNumberOfComponents() != 1 || connectivity->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Only single component arrays may be used for vtkCellArray "
                  "storage.");
    return;
  }

  if (this->Offsets.Get() != offsets)
  {
    this->Offsets = offsets;
    this->Modified();
  }
  if (this->Connectivity.Get() != connectivity)
  {
    this->Connectivity = connectivity;
    this->Modified();
  }
  this->StorageType = StorageTypes::Int64;
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(vtkIdTypeArray* offsets, vtkIdTypeArray* connectivity)
{
#ifdef VTK_USE_64BIT_IDS
  vtkNew<vtkTypeInt64Array> o;
  vtkNew<vtkTypeInt64Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else  // VTK_USE_64BIT_IDS
  vtkNew<vtkTypeInt32Array> o;
  vtkNew<vtkTypeInt32Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#endif // VTK_USE_64BIT_IDS
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(
  vtkAOSDataArrayTemplate<int>* offsets, vtkAOSDataArrayTemplate<int>* connectivity)
{
#if VTK_SIZEOF_INT == 4
  vtkNew<vtkTypeInt32Array> o;
  vtkNew<vtkTypeInt32Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else
  vtkErrorMacro("`int` type is not 32 bits.");
#endif
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(
  vtkAOSDataArrayTemplate<long>* offsets, vtkAOSDataArrayTemplate<long>* connectivity)
{
#if VTK_SIZEOF_LONG == 4
  vtkNew<vtkTypeInt32Array> o;
  vtkNew<vtkTypeInt32Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#elif VTK_SIZEOF_LONG == 8
  vtkNew<vtkTypeInt64Array> o;
  vtkNew<vtkTypeInt64Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else
  vtkErrorMacro("`long` type is neither 32 nor 64 bits.");
#endif
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(
  vtkAOSDataArrayTemplate<long long>* offsets, vtkAOSDataArrayTemplate<long long>* connectivity)
{
#if VTK_SIZEOF_LONG_LONG == 8
  vtkNew<vtkTypeInt64Array> o;
  vtkNew<vtkTypeInt64Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else
  vtkErrorMacro("`long long` type is not 64 bits.");
#endif
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(vtkAffineTypeInt32Array* offsets, vtkTypeInt32Array* connectivity)
{
  if (offsets->GetNumberOfComponents() != 1 || connectivity->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Only single component arrays may be used for vtkCellArray "
                  "storage.");
    return;
  }

  if (this->Offsets.Get() != offsets)
  {
    this->Offsets = offsets;
    this->Modified();
  }
  if (this->Connectivity.Get() != connectivity)
  {
    this->Connectivity = connectivity;
    this->Modified();
  }
  this->StorageType = StorageTypes::FixedSizeInt32;
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(vtkAffineTypeInt64Array* offsets, vtkTypeInt64Array* connectivity)
{
  if (offsets->GetNumberOfComponents() != 1 || connectivity->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Only single component arrays may be used for vtkCellArray "
                  "storage.");
    return;
  }

  if (this->Offsets.Get() != offsets)
  {
    this->Offsets = offsets;
    this->Modified();
  }
  if (this->Connectivity.Get() != connectivity)
  {
    this->Connectivity = connectivity;
    this->Modified();
  }
  this->StorageType = StorageTypes::FixedSizeInt64;
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(vtkAffineArray<vtkIdType>* offsets, vtkIdTypeArray* connectivity)
{
#ifdef VTK_USE_64BIT_IDS
  vtkNew<vtkAffineTypeInt64Array> o;
  vtkNew<vtkTypeInt64Array> c;
  o->ConstructBackend(offsets->GetBackend()->Slope, offsets->GetBackend()->Intercept);
  o->SetNumberOfValues(offsets->GetNumberOfValues());
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else  // VTK_USE_64BIT_IDS
  vtkNew<vtkAffineTypeInt32Array> o;
  vtkNew<vtkTypeInt32Array> c;
  o->ConstructBackend(offsets->GetBackend()->Slope, offsets->GetBackend()->Intercept);
  o->SetNumberOfValues(offsets->GetNumberOfValues());
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#endif // VTK_USE_64BIT_IDS
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(vtkAffineArray<int>* offsets, vtkAOSDataArrayTemplate<int>* connectivity)
{
#if VTK_SIZEOF_INT == 4
  vtkNew<vtkAffineTypeInt32Array> o;
  vtkNew<vtkTypeInt32Array> c;
  o->ConstructBackend(offsets->GetBackend()->Slope, offsets->GetBackend()->Intercept);
  o->SetNumberOfValues(offsets->GetNumberOfValues());
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else
  vtkErrorMacro("`int` type is not 32 bits.");
#endif
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(
  vtkAffineArray<long>* offsets, vtkAOSDataArrayTemplate<long>* connectivity)
{
#if VTK_SIZEOF_LONG == 4
  vtkNew<vtkAffineTypeInt32Array> o;
  vtkNew<vtkTypeInt32Array> c;
  o->ConstructBackend(offsets->GetBackend()->Slope, offsets->GetBackend()->Intercept);
  o->SetNumberOfValues(offsets->GetNumberOfValues());
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#elif VTK_SIZEOF_LONG == 8
  vtkNew<vtkAffineTypeInt64Array> o;
  vtkNew<vtkTypeInt64Array> c;
  o->ConstructBackend(offsets->GetBackend()->Slope, offsets->GetBackend()->Intercept);
  o->SetNumberOfValues(offsets->GetNumberOfValues());
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else
  vtkErrorMacro("`long` type is neither 32 nor 64 bits.");
#endif
}

//------------------------------------------------------------------------------
void vtkCellArray::SetData(
  vtkAffineArray<long long>* offsets, vtkAOSDataArrayTemplate<long long>* connectivity)
{
#if VTK_SIZEOF_LONG_LONG == 8
  vtkNew<vtkAffineTypeInt64Array> o;
  vtkNew<vtkTypeInt64Array> c;
  o->ConstructBackend(offsets->GetBackend()->Slope, offsets->GetBackend()->Intercept);
  o->SetNumberOfValues(offsets->GetNumberOfValues());
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else
  vtkErrorMacro("`long long` type is not 64 bits.");
#endif
}

VTK_ABI_NAMESPACE_END

namespace
{
struct SetDataGenericImpl
{
  vtkCellArray* CellArray;
  template <typename OffsetsT, typename ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* connectivity)
  {
    this->CellArray->SetData(offsets, connectivity);
  }
};
} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
bool vtkCellArray::SetData(vtkDataArray* offsets, vtkDataArray* connectivity)
{
  if (!offsets || !connectivity)
  {
    vtkErrorMacro("Empty offsets or connectivity array.");
    return false;
  }
  if (offsets->GetNumberOfComponents() != 1 || connectivity->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Only single component arrays may be used for vtkCellArray "
                  "storage.");
    return false;
  }
  SetDataGenericImpl worker{ this };
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<vtkArrayDispatch::InputOffsetsArrays,
      vtkArrayDispatch::InputConnectivityArrays>;
  if (!Dispatcher::Execute(offsets, connectivity, worker))
  {
    if (this->Offsets.Get() != offsets)
    {
      this->Offsets = offsets;
      this->Modified();
    }
    if (this->Connectivity.Get() != connectivity)
    {
      this->Connectivity = connectivity;
      this->Modified();
    }
    this->StorageType = StorageTypes::Generic;
  }
  return true;
}

namespace
{
template <typename T>
void SetDataFixedCellSizeImpl(
  vtkCellArray* cellArray, vtkIdType cellSize, vtkDataArray* connectivity, bool& success)
{
  vtkNew<vtkAffineArray<T>> offsets;
  offsets->ConstructBackend(cellSize, 0);
  offsets->SetNumberOfTuples((connectivity->GetNumberOfValues() / cellSize) + 1);
  success = cellArray->SetData(offsets, connectivity);
}
}

//------------------------------------------------------------------------------
bool vtkCellArray::SetData(vtkIdType cellSize, vtkDataArray* connectivity)
{
  if (connectivity == nullptr || cellSize <= 0)
  {
    vtkErrorMacro("Invalid cellSize or connectivity array.");
    return false;
  }

  if ((connectivity->GetNumberOfTuples() % cellSize) != 0)
  {
    vtkErrorMacro("Connectivity array size is not suitable for chosen cellSize");
    return false;
  }

  bool success = false;
  switch (connectivity->GetDataType())
  {
    vtkTemplateMacro(SetDataFixedCellSizeImpl<VTK_TT>(this, cellSize, connectivity, success));
    default:
      vtkErrorMacro("Unsupported connectivity array type: " << connectivity->GetDataType());
      break;
  }
  return success;
}

//------------------------------------------------------------------------------
void vtkCellArray::Use32BitStorage()
{
  if (this->IsStorage32Bit())
  {
    this->Initialize();
    return;
  }
  this->Offsets = vtkSmartPointer<ArrayType32>::New();
  this->Connectivity = vtkSmartPointer<ArrayType32>::New();
  this->GetOffsetsArray32()->InsertNextValue(0);
  this->StorageType = StorageTypes::Int32;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCellArray::Use64BitStorage()
{
  if (this->IsStorage64Bit())
  {
    this->Initialize();
    return;
  }
  this->Offsets = vtkSmartPointer<ArrayType64>::New();
  this->Connectivity = vtkSmartPointer<ArrayType64>::New();
  this->GetOffsetsArray64()->InsertNextValue(0);
  this->StorageType = StorageTypes::Int64;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCellArray::UseDefaultStorage()
{
  if (this->GetDefaultStorageIs64Bit())
  {
    this->Use64BitStorage();
  }
  else
  {
    this->Use32BitStorage();
  }
}

//------------------------------------------------------------------------------
void vtkCellArray::UseFixedSize32BitStorage(vtkIdType cellSize)
{
  if (this->IsStorageFixedSize32Bit() &&
    this->GetOffsetsAffineArray32()->GetBackend()->Slope == cellSize)
  {
    this->Initialize();
    return;
  }
  this->Offsets = vtkSmartPointer<AffineArrayType32>::New();
  this->GetOffsetsAffineArray32()->ConstructBackend(cellSize, 0);
  this->Connectivity = vtkSmartPointer<ArrayType32>::New();
  this->GetOffsetsAffineArray32()->InsertNextValue(0);
  this->StorageType = StorageTypes::FixedSizeInt32;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCellArray::UseFixedSize64BitStorage(vtkIdType cellSize)
{
  if (this->IsStorageFixedSize64Bit() &&
    this->GetOffsetsAffineArray64()->GetBackend()->Slope == cellSize)
  {
    this->Initialize();
    return;
  }
  this->Offsets = vtkSmartPointer<AffineArrayType64>::New();
  this->GetOffsetsAffineArray64()->ConstructBackend(cellSize, 0);
  this->Connectivity = vtkSmartPointer<ArrayType64>::New();
  this->GetOffsetsAffineArray64()->InsertNextValue(0);
  this->StorageType = StorageTypes::FixedSizeInt64;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCellArray::UseFixedSizeDefaultStorage(vtkIdType cellSize)
{
  if (this->GetDefaultStorageIs64Bit())
  {
    this->UseFixedSize64BitStorage(cellSize);
  }
  else
  {
    this->UseFixedSize32BitStorage(cellSize);
  }
}

//------------------------------------------------------------------------------
bool vtkCellArray::CanConvertTo32BitStorage() const
{
  if (this->IsStorage32Bit())
  {
    return true;
  }
  bool canConvert;
  this->Dispatch(CanConvert<ArrayType32::ValueType>{}, canConvert);
  return canConvert;
}

//------------------------------------------------------------------------------
bool vtkCellArray::CanConvertTo64BitStorage() const
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkCellArray::CanConvertToFixedSize32BitStorage() const
{
  if (this->IsStorageFixedSize32Bit())
  {
    return true;
  }
  const vtkIdType homogeneousCellSize = this->IsHomogeneous();
  if (homogeneousCellSize >= 0)
  {
    bool result;
    this->Dispatch(CanConvert<ArrayType32::ValueType>{}, result);
    return result;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkCellArray::CanConvertToFixedSize64BitStorage() const
{
  if (this->IsStorageFixedSize64Bit())
  {
    return true;
  }
  return this->IsHomogeneous() >= 0;
}

//------------------------------------------------------------------------------
bool vtkCellArray::CanConvertToDefaultStorage() const
{
  if (this->GetDefaultStorageIs64Bit())
  {
    return this->CanConvertTo64BitStorage();
  }
  else
  {
    return this->CanConvertTo32BitStorage();
  }
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertTo32BitStorage()
{
  if (this->IsStorage32Bit())
  {
    return true;
  }
  vtkNew<ArrayType32> offsets;
  vtkNew<ArrayType32> conn;
  bool result;
  this->Dispatch(ExtractAndInitialize{}, offsets.Get(), conn.Get(), result);
  if (!result)
  {
    return false;
  }

  this->SetData(offsets, conn);
  return true;
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertTo64BitStorage()
{
  if (this->IsStorage64Bit())
  {
    return true;
  }
  vtkNew<ArrayType64> offsets;
  vtkNew<ArrayType64> conn;
  bool result;
  this->Dispatch(ExtractAndInitialize{}, offsets.Get(), conn.Get(), result);
  if (!result)
  {
    return false;
  }

  this->SetData(offsets, conn);
  return true;
}

//------------------------------------------------------------------------------
bool vtkCellArray::CanConvertToFixedSizeDefaultStorage() const
{
  if (this->GetDefaultStorageIs64Bit())
  {
    return this->CanConvertToFixedSize64BitStorage();
  }
  else
  {
    return this->CanConvertToFixedSize32BitStorage();
  }
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertToDefaultStorage()
{
  if (this->GetDefaultStorageIs64Bit())
  {
    return this->ConvertTo64BitStorage();
  }
  else
  {
    return this->ConvertTo32BitStorage();
  }
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertToFixedSize32BitStorage()
{
  if (this->IsStorageFixedSize32Bit())
  {
    return true;
  }
  vtkNew<AffineArrayType32> offsets;
  offsets->ConstructBackend(this->GetCellSize(0), 0);
  offsets->SetNumberOfValues(this->GetNumberOfCells() + 1);
  vtkNew<ArrayType32> conn;
  bool result;
  this->Dispatch(ExtractAndInitialize{}, conn.Get(), result);
  if (!result)
  {
    return false;
  }

  this->SetData(offsets, conn);
  return true;
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertToFixedSize64BitStorage()
{
  if (this->IsStorageFixedSize64Bit())
  {
    return true;
  }
  vtkNew<AffineArrayType64> offsets;
  offsets->ConstructBackend(this->GetCellSize(0), 0);
  offsets->SetNumberOfValues(this->GetNumberOfCells() + 1);
  vtkNew<ArrayType64> conn;
  bool result;
  this->Dispatch(ExtractAndInitialize{}, conn.Get(), result);
  if (!result)
  {
    return false;
  }

  this->SetData(offsets, conn);
  return true;
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertToFixedSizeDefaultStorage()
{
  if (this->GetDefaultStorageIs64Bit())
  {
    return this->ConvertToFixedSize64BitStorage();
  }
  else
  {
    return this->ConvertToFixedSize32BitStorage();
  }
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertToSmallestStorage()
{
  bool isHomogeneous = this->IsHomogeneous() >= 0;
  if (!isHomogeneous)
  {
    if (this->IsStorage64Bit() && this->CanConvertTo32BitStorage())
    {
      return this->ConvertTo32BitStorage();
    }
    // Already at the smallest possible.
    return true;
  }
  else
  {
    if (this->IsStorage64Bit() || this->IsStorageFixedSize64Bit())
    {
      if (this->CanConvertTo32BitStorage()) // Avoid checking for homogeneity again
      {
        return this->ConvertToFixedSize32BitStorage();
      }
      return this->ConvertToFixedSize64BitStorage();
    }
    else if (this->IsStorage32Bit())
    {
      return this->ConvertToFixedSize32BitStorage();
    }
    // Already at the smallest possible.
    return true;
  }
}

//------------------------------------------------------------------------------
bool vtkCellArray::AllocateExact(vtkIdType numCells, vtkIdType connectivitySize)
{
  bool result;
  this->Dispatch(AllocateExactImpl{}, numCells, connectivitySize, result);
  return result;
}

//------------------------------------------------------------------------------
bool vtkCellArray::ResizeExact(vtkIdType numCells, vtkIdType connectivitySize)
{
  bool result;
  this->Dispatch(ResizeExactImpl{}, numCells, connectivitySize, result);
  return result;
}

//------------------------------------------------------------------------------
// Returns the size of the largest cell. The size is the number of points
// defining the cell.
int vtkCellArray::GetMaxCellSize()
{
  const vtkIdType numCells = this->GetNumberOfCells();
  // We use THRESHOLD to test if the data size is small enough
  // to execute the functor serially. This is faster.
  // and also potentially avoids nested multithreading which creates race conditions.
  FindMaxCell finder{ this };
  vtkSMPTools::For(0, numCells, vtkSMPTools::THRESHOLD, finder);

  return static_cast<int>(finder.Result);
}

//------------------------------------------------------------------------------
unsigned long vtkCellArray::GetActualMemorySize() const
{
  return this->GetOffsetsArray()->GetActualMemorySize() +
    this->GetConnectivityArray()->GetActualMemorySize();
}

//------------------------------------------------------------------------------
void vtkCellArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StorageType: ";
  switch (this->GetStorageType())
  {
    case StorageTypes::Int32:
      os << "Int32\n";
      break;
    case StorageTypes::Int64:
      os << "Int64\n";
      break;
    case StorageTypes::FixedSizeInt32:
      os << "FixedSizeInt32\n";
      break;
    case StorageTypes::FixedSizeInt64:
      os << "FixedSizeInt64\n";
      break;
    case StorageTypes::Generic:
    default:
      os << "Generic\n";
      break;
  }

  PrintSelfImpl functor;
  this->Dispatch(functor, os, indent);
}

//------------------------------------------------------------------------------
void vtkCellArray::PrintDebug(std::ostream& os)
{
  this->Print(os);
  this->Dispatch(PrintDebugImpl{}, os);
}

//------------------------------------------------------------------------------
vtkIdType vtkCellArray::GetTraversalCellId()
{
  return this->TraversalCellId;
}

//------------------------------------------------------------------------------
void vtkCellArray::SetTraversalCellId(vtkIdType cellId)
{
  this->TraversalCellId = cellId;
}

//------------------------------------------------------------------------------
void vtkCellArray::ReverseCellAtId(vtkIdType cellId)
{
  this->Dispatch(ReverseCellAtIdImpl{}, cellId);
}

//------------------------------------------------------------------------------
void vtkCellArray::ReplaceCellAtId(vtkIdType cellId, vtkIdList* list)
{
  this->Dispatch(ReplaceCellAtIdImpl{}, cellId, list->GetNumberOfIds(), list->GetPointer(0));
}

//------------------------------------------------------------------------------
void vtkCellArray::ReplaceCellAtId(
  vtkIdType cellId, vtkIdType cellSize, const vtkIdType cellPoints[])
{
  this->Dispatch(ReplaceCellAtIdImpl{}, cellId, cellSize, cellPoints);
}

//------------------------------------------------------------------------------
void vtkCellArray::ReplaceCellPointAtId(
  vtkIdType cellId, vtkIdType cellPointIndex, vtkIdType newPointId)
{
  this->Dispatch(ReplaceCellPointAtIdImpl{}, cellId, cellPointIndex, newPointId);
}

//------------------------------------------------------------------------------
void vtkCellArray::ExportLegacyFormat(vtkIdTypeArray* data)
{
  vtkIdType size;
  this->Dispatch(GetLegacyDataSizeImpl{}, size);
  data->Allocate(size);

  auto it = vtk::TakeSmartPointer(this->NewIterator());

  vtkIdType cellSize;
  const vtkIdType* cellPoints;
  for (it->GoToFirstCell(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    it->GetCurrentCell(cellSize, cellPoints);
    data->InsertNextValue(cellSize);
    for (vtkIdType i = 0; i < cellSize; ++i)
    {
      data->InsertNextValue(cellPoints[i]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkCellArray::ImportLegacyFormat(vtkIdTypeArray* data)
{
  this->ImportLegacyFormat(data->GetPointer(0), data->GetNumberOfValues());
}

//------------------------------------------------------------------------------
void vtkCellArray::ImportLegacyFormat(const vtkIdType* data, vtkIdType len)
{
  this->Reset();
  this->AppendLegacyFormat(data, len, 0);
}

//------------------------------------------------------------------------------
void vtkCellArray::AppendLegacyFormat(vtkIdTypeArray* data, vtkIdType ptOffset)
{
  this->AppendLegacyFormat(data->GetPointer(0), data->GetNumberOfValues(), ptOffset);
}

//------------------------------------------------------------------------------
void vtkCellArray::AppendLegacyFormat(const vtkIdType* data, vtkIdType len, vtkIdType ptOffset)
{
  this->Dispatch(AppendLegacyFormatImpl{}, data, len, ptOffset);
}

//------------------------------------------------------------------------------
void vtkCellArray::Squeeze()
{
  this->Dispatch(SqueezeImpl{});

  // Just delete the legacy buffer.
  this->LegacyData->Initialize();
}

//------------------------------------------------------------------------------
bool vtkCellArray::IsValid()
{
  bool isValid;
  this->Dispatch(IsValidImpl{}, isValid);
  return isValid;
}

//------------------------------------------------------------------------------
vtkIdType vtkCellArray::IsHomogeneous() const
{
  if (this->IsStorageFixedSize32Bit() || this->IsStorageFixedSize64Bit())
  {
    return this->GetNumberOfCells() == 0 ? 0 : this->GetCellSize(0);
  }
  vtkIdType isHomogeneous;
  this->Dispatch(IsHomogeneousImpl{}, isHomogeneous);
  return isHomogeneous;
}
VTK_ABI_NAMESPACE_END
