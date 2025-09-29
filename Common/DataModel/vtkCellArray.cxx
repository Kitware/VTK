// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArrayIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"

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
      (*it + cellId) != location /* `location` not at cell head */)
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
    offsets->Initialize();
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
  template <class OffsetsT, class ConnectivityT, class TargetArrayT>
  void operator()(OffsetsT* inOffsets, ConnectivityT* inConn, TargetArrayT* outOffsets,
    TargetArrayT* outConn, bool& result) const
  {
    result = this->Process(inOffsets, outOffsets) && this->Process(inConn, outConn);
  }

  template <typename SourceArrayT, typename TargetArrayT>
  bool Process(SourceArrayT* src, TargetArrayT* dst) const
  {
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
      [&](const ValueType a, const ValueType b) -> bool { return (b - a != firstCellSize); });

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
    using AccessorType = vtkDataArrayAccessor<OffsetsT>;
    AccessorType offsetsAccessor(offsets);
    AccessorType connAccessor(conn);

    ValueType offset = static_cast<ValueType>(conn->GetNumberOfValues());

    const vtkIdType* const dataEnd = data + len;
    while (data < dataEnd)
    {
      vtkIdType numPts = *data++;
      offset += static_cast<ValueType>(numPts);
      offsetsAccessor.InsertNext(static_cast<ValueType>(offset));
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

    const auto srcRange = vtk::DataArrayValueRange<1>(srcArray, skipFirst ? 1 : 0);
    auto dstRange = vtk::DataArrayValueRange<1>(dstArray, dstBegin, dstEnd);
    assert(srcRange.size() == dstRange.size());

    const DstValueType dOffset = static_cast<DstValueType>(offset);

    std::transform(srcRange.cbegin(), srcRange.cend(), dstRange.begin(),
      [&](SrcValueType x) -> DstValueType { return static_cast<DstValueType>(x) + dOffset; });
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
vtkCellArray::vtkCellArray() = default;
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

  if (other->Storage.Is64Bit())
  {
    this->Storage.Use64BitStorage();
    auto& srcStorage = other->Storage.GetArrays64();
    auto& dstStorage = this->Storage.GetArrays64();
    dstStorage.Offsets->DeepCopy(srcStorage.Offsets);
    dstStorage.Connectivity->DeepCopy(srcStorage.Connectivity);
    this->Modified();
  }
  else
  {
    this->Storage.Use32BitStorage();
    auto& srcStorage = other->Storage.GetArrays32();
    auto& dstStorage = this->Storage.GetArrays32();
    dstStorage.Offsets->DeepCopy(srcStorage.Offsets);
    dstStorage.Connectivity->DeepCopy(srcStorage.Connectivity);
    this->Modified();
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

  if (other->Storage.Is64Bit())
  {
    auto& srcStorage = other->Storage.GetArrays64();
    this->SetData(srcStorage.GetOffsets(), srcStorage.GetConnectivity());
  }
  else
  {
    auto& srcStorage = other->Storage.GetArrays32();
    this->SetData(srcStorage.GetOffsets(), srcStorage.GetConnectivity());
  }
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

  this->Storage.Use32BitStorage();
  auto& storage = this->Storage.GetArrays32();
  // vtkArrayDownCast to ensure this works when ArrayType32 is vtkIdTypeArray.
  if (storage.Offsets != offsets)
  {
    storage.Offsets = vtkArrayDownCast<ArrayType32>(offsets);
    this->Modified();
  }
  if (storage.Connectivity != connectivity)
  {
    storage.Connectivity = vtkArrayDownCast<ArrayType32>(connectivity);
    this->Modified();
  }
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

  this->Storage.Use64BitStorage();
  auto& storage = this->Storage.GetArrays64();
  // vtkArrayDownCast to ensure this works when ArrayType64 is vtkIdTypeArray.
  if (storage.Offsets != offsets)
  {
    storage.Offsets = vtkArrayDownCast<ArrayType64>(offsets);
    this->Modified();
  }
  if (storage.Connectivity != connectivity)
  {
    storage.Connectivity = vtkArrayDownCast<ArrayType64>(connectivity);
    this->Modified();
  }
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
#elif VTK_SIZEOF_INT == 8
  vtkNew<vtkTypeInt64Array> o;
  vtkNew<vtkTypeInt64Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else
  vtkErrorMacro("`int` type is neither 32 nor 64 bits.");
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
#if VTK_SIZEOF_LONG_LONG == 4
  vtkNew<vtkTypeInt32Array> o;
  vtkNew<vtkTypeInt32Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#elif VTK_SIZEOF_LONG_LONG == 8
  vtkNew<vtkTypeInt64Array> o;
  vtkNew<vtkTypeInt64Array> c;
  o->ShallowCopy(offsets);
  c->ShallowCopy(connectivity);
  this->SetData(o, c);
#else
  vtkErrorMacro("`long long` type is neither 32 nor 64 bits.");
#endif
}

VTK_ABI_NAMESPACE_END

namespace
{

struct SetDataGenericImpl
{
  vtkCellArray* CellArray;
  vtkDataArray* ConnDA;
  bool ArraysMatch;

  template <typename ArrayT>
  void operator()(ArrayT* offsets)
  {
    ArrayT* conn = vtkArrayDownCast<ArrayT>(this->ConnDA);
    if (!conn)
    {
      this->ArraysMatch = false;
      return;
    }
    this->ArraysMatch = true;

    this->CellArray->SetData(offsets, conn);
  }
};

struct GenerateOffsetsImpl
{
  vtkIdType CellSize;
  vtkIdType ConnectivityArraySize;

  template <typename ArrayT>
  void operator()(ArrayT* offsets)
  {
    for (vtkIdType cc = 0, max = (offsets->GetNumberOfTuples() - 1); cc < max; ++cc)
    {
      offsets->SetTypedComponent(cc, 0, cc * this->CellSize);
    }
    offsets->SetTypedComponent(offsets->GetNumberOfTuples() - 1, 0, this->ConnectivityArraySize);
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
bool vtkCellArray::SetData(vtkDataArray* offsets, vtkDataArray* connectivity)
{
  SetDataGenericImpl worker{ this, connectivity, false };
  using SupportedArrays = vtkCellArray::InputArrayList;
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  if (!Dispatch::Execute(offsets, worker))
  {
    vtkErrorMacro("Invalid array types passed to SetData: "
      << "offsets=" << offsets->GetClassName() << ", "
      << "connectivity=" << connectivity->GetClassName());
    return false;
  }

  if (!worker.ArraysMatch)
  {
    vtkErrorMacro("Offsets and Connectivity arrays must have the same type.");
    return false;
  }

  return true;
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

  vtkSmartPointer<vtkDataArray> offsets;
  offsets.TakeReference(connectivity->NewInstance());
  offsets->SetNumberOfTuples(1 + connectivity->GetNumberOfTuples() / cellSize);

  GenerateOffsetsImpl worker{ cellSize, connectivity->GetNumberOfTuples() };
  using SupportedArrays = vtkCellArray::InputArrayList;
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  if (!Dispatch::Execute(offsets, worker))
  {
    vtkErrorMacro("Invalid array types passed to SetData: "
      << "connectivity=" << connectivity->GetClassName());
    return false;
  }

  return this->SetData(offsets, connectivity);
}

//------------------------------------------------------------------------------
void vtkCellArray::Use32BitStorage()
{
  if (!this->Storage.Is64Bit())
  {
    this->Initialize();
    return;
  }
  this->Storage.Use32BitStorage();
}

//------------------------------------------------------------------------------
void vtkCellArray::Use64BitStorage()
{
  if (this->Storage.Is64Bit())
  {
    this->Initialize();
    return;
  }
  this->Storage.Use64BitStorage();
}

//------------------------------------------------------------------------------
void vtkCellArray::UseDefaultStorage()
{
#ifdef VTK_USE_64BIT_IDS
  this->Use64BitStorage();
#else  // VTK_USE_64BIT_IDS
  this->Use32BitStorage();
#endif // VTK_USE_64BIT_IDS
}

//------------------------------------------------------------------------------
bool vtkCellArray::CanConvertTo32BitStorage() const
{
  if (!this->Storage.Is64Bit())
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
bool vtkCellArray::CanConvertToDefaultStorage() const
{
#ifdef VTK_USE_64BIT_IDS
  return this->CanConvertTo64BitStorage();
#else  // VTK_USE_64BIT_IDS
  return this->CanConvertTo32BitStorage();
#endif // VTK_USE_64BIT_IDS
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertTo32BitStorage()
{
  if (!this->IsStorage64Bit())
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
bool vtkCellArray::ConvertToDefaultStorage()
{
#ifdef VTK_USE_64BIT_IDS
  return this->ConvertTo64BitStorage();
#else  // VTK_USE_64BIT_IDS
  return this->ConvertTo32BitStorage();
#endif // VTK_USE_64BIT_IDS
}

//------------------------------------------------------------------------------
bool vtkCellArray::ConvertToSmallestStorage()
{
  if (this->IsStorage64Bit() && this->CanConvertTo32BitStorage())
  {
    return this->ConvertTo32BitStorage();
  }
  // Already at the smallest possible.
  return true;
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

  os << indent << "StorageIs64Bit: " << this->Storage.Is64Bit() << "\n";

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
vtkIdType vtkCellArray::IsHomogeneous()
{
  vtkIdType isHomogeneous;
  this->Dispatch(IsHomogeneousImpl{}, isHomogeneous);
  return isHomogeneous;
}
VTK_ABI_NAMESPACE_END
