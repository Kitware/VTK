/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

struct GetSizeImpl
{
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& cells) const
  {
    return (cells.GetOffsets()->GetSize() + cells.GetConnectivity()->GetSize());
  }
};

// Given a legacy Location, find the corresponding cellId. The location
// *must* refer to a [numPts] entry in the old connectivity array, or the
// returned CellId will be -1.
struct LocationToCellIdFunctor
{
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& cells, vtkIdType location) const
  {
    using ValueType = typename CellStateT::ValueType;

    const auto offsets = vtk::DataArrayValueRange<1>(cells.GetOffsets());

    // Use a binary-search to find the location:
    auto it = this->BinarySearchOffset(
      offsets.begin(), offsets.end() - 1, static_cast<ValueType>(location));

    const vtkIdType cellId = std::distance(offsets.begin(), it);

    if (it == offsets.end() - 1 /* no match found */ ||
      (*it + cellId) != location /* `location` not at cell head */)
    { // Location invalid.
      return -1;
    }

    // return the cell id:
    return cellId;
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

struct CellIdToLocationFunctor
{
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& cells, vtkIdType cellId) const
  {
    // Adding the cellId to the offset of that cell id gives us the cell
    // location in the old-style vtkCellArray connectivity array.
    return static_cast<vtkIdType>(cells.GetOffsets()->GetValue(cellId)) + cellId;
  }
};

struct GetInsertLocationImpl
{
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& cells) const
  {
    // The insert location used to just be the tail of the connectivity array.
    // Compute the equivalent value:
    return (
      (cells.GetOffsets()->GetNumberOfValues() - 1) + cells.GetConnectivity()->GetNumberOfValues());
  }
};

} // end namespace deprec

struct PrintDebugImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& state, std::ostream& os)
  {
    using ValueType = typename CellStateT::ValueType;

    const vtkIdType numCells = state.GetNumberOfCells();
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      os << "cell " << cellId << ": ";

      const auto cellRange = state.GetCellRange(cellId);
      for (ValueType ptId : cellRange)
      {
        os << ptId << " ";
      }

      os << "\n";
    }
  }
};

struct InitializeImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& cells) const
  {
    cells.GetConnectivity()->Initialize();
    cells.GetOffsets()->Initialize();
    cells.GetOffsets()->InsertNextValue(0);
  }
};

struct SqueezeImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& cells) const
  {
    cells.GetConnectivity()->Squeeze();
    cells.GetOffsets()->Squeeze();
  }
};

struct IsValidImpl
{
  template <typename CellStateT>
  bool operator()(CellStateT& state) const
  {
    using ValueType = typename CellStateT::ValueType;
    auto* offsetArray = state.GetOffsets();
    auto* connArray = state.GetConnectivity();

    // Both arrays must be single component
    if (offsetArray->GetNumberOfComponents() != 1 || connArray->GetNumberOfComponents() != 1)
    {
      return false;
    }

    auto offsets = vtk::DataArrayValueRange<1>(offsetArray);

    // Offsets must have at least one value, and the first value must be zero
    if (offsets.size() == 0 || *offsets.cbegin() != 0)
    {
      return false;
    }

    // Values in offsets must not decrease
    auto it = std::adjacent_find(offsets.cbegin(), offsets.cend(),
      [](const ValueType a, const ValueType b) -> bool { return a > b; });
    if (it != offsets.cend())
    {
      return false;
    }

    // The last value in offsets must be the size of the connectivity array.
    if (connArray->GetNumberOfValues() != *(offsets.cend() - 1))
    {
      return false;
    }

    return true;
  }
};

template <typename T>
struct CanConvert
{
  template <typename CellStateT>
  bool operator()(CellStateT& state) const
  {
    using ArrayType = typename CellStateT::ArrayType;
    using ValueType = typename CellStateT::ValueType;

    // offsets are sorted, so just check the last value, but we have to compute
    // the full range of the connectivity array.
    auto* off = state.GetOffsets();
    if (off->GetNumberOfValues() > 0 && !this->CheckValue(off->GetValue(off->GetMaxId())))
    {
      return false;
    }

    std::array<ValueType, 2> connRange;
    auto* mutConn = const_cast<ArrayType*>(state.GetConnectivity());
    if (mutConn->GetNumberOfValues() > 0)
    {
      mutConn->GetValueRange(connRange.data(), 0);
      if (!this->CheckValue(connRange[0]) || !this->CheckValue(connRange[1]))
      {
        return false;
      }
    }

    return true;
  }

  template <typename U>
  bool CheckValue(const U& val) const
  {
    return val == static_cast<U>(static_cast<T>(val));
  }
};

struct ExtractAndInitialize
{
  template <typename CellStateT, typename TargetArrayT>
  bool operator()(CellStateT& state, TargetArrayT* offsets, TargetArrayT* conn) const
  {
    return (
      this->Process(state.GetOffsets(), offsets) && this->Process(state.GetConnectivity(), conn));
  }

  template <typename SourceArrayT, typename TargetArrayT>
  bool Process(SourceArrayT* src, TargetArrayT* dst) const
  {
    // Check that allocation suceeds:
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

struct IsHomogeneousImpl
{
  template <typename CellArraysT>
  vtkIdType operator()(CellArraysT& state) const
  {
    using ValueType = typename CellArraysT::ValueType;
    auto* offsets = state.GetOffsets();

    const vtkIdType numCells = state.GetNumberOfCells();
    if (numCells == 0)
    {
      return 0;
    }

    // Initialize using the first cell:
    const vtkIdType firstCellSize = state.GetCellSize(0);

    // Verify the rest:
    auto offsetRange = vtk::DataArrayValueRange<1>(offsets);
    auto it = std::adjacent_find(offsetRange.begin() + 1, offsetRange.end(),
      [&](const ValueType a, const ValueType b) -> bool { return (b - a != firstCellSize); });

    if (it != offsetRange.end())
    { // Found a cell that doesn't match the size of the first cell:
      return -1;
    }

    return firstCellSize;
  }
};

struct AllocateExactImpl
{
  template <typename CellStateT>
  bool operator()(CellStateT& cells, vtkIdType numCells, vtkIdType connectivitySize) const
  {
    const bool result = (cells.GetOffsets()->Allocate(numCells + 1) &&
      cells.GetConnectivity()->Allocate(connectivitySize));
    if (result)
    {
      cells.GetOffsets()->InsertNextValue(0);
    }

    return result;
  }
};

struct ResizeExactImpl
{
  template <typename CellStateT>
  bool operator()(CellStateT& cells, vtkIdType numCells, vtkIdType connectivitySize) const
  {
    return (cells.GetOffsets()->SetNumberOfValues(numCells + 1) &&
      cells.GetConnectivity()->SetNumberOfValues(connectivitySize));
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

  struct Impl
  {
    template <typename CellStateT>
    vtkIdType operator()(CellStateT& cells, vtkIdType cellId, const vtkIdType endCellId) const
    {
      vtkIdType result = 0;
      for (; cellId < endCellId; ++cellId)
      {
        result = std::max(result, cells.GetCellSize(cellId));
      }
      return result;
    }
  };

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkIdType& lval = this->LocalResult.Local();
    lval = std::max(lval, this->CellArray->Visit(Impl{}, cellId, endCellId));
  }

  void Reduce()
  {
    for (const vtkIdType lResult : this->LocalResult)
    {
      this->Result = std::max(this->Result, lResult);
    }
  }
};

struct GetActualMemorySizeImpl
{
  template <typename CellStateT>
  unsigned long operator()(CellStateT& cells) const
  {
    return (
      cells.GetOffsets()->GetActualMemorySize() + cells.GetConnectivity()->GetActualMemorySize());
  }
};

struct PrintSelfImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& cells, ostream& os, vtkIndent indent) const
  {
    os << indent << "Offsets:\n";
    cells.GetOffsets()->PrintSelf(os, indent.GetNextIndent());
    os << indent << "Connectivity:\n";
    cells.GetConnectivity()->PrintSelf(os, indent.GetNextIndent());
  }
};

struct GetLegacyDataSizeImpl
{
  template <typename CellStateT>
  vtkIdType operator()(CellStateT& cells) const
  {
    return (
      (cells.GetOffsets()->GetNumberOfValues() - 1) + cells.GetConnectivity()->GetNumberOfValues());
  }
};

struct ReverseCellAtIdImpl
{
  template <typename CellStateT>
  void operator()(CellStateT& cells, vtkIdType cellId) const
  {
    auto cellRange = cells.GetCellRange(cellId);
    std::reverse(cellRange.begin(), cellRange.end());
  }
};

struct ReplaceCellAtIdImpl
{
  template <typename CellStateT>
  void operator()(
    CellStateT& cells, vtkIdType cellId, vtkIdType cellSize, const vtkIdType* cellPoints) const
  {
    using ValueType = typename CellStateT::ValueType;

    auto cellRange = cells.GetCellRange(cellId);

    assert(cellRange.size() == cellSize);
    for (vtkIdType i = 0; i < cellSize; ++i)
    {
      cellRange[i] = static_cast<ValueType>(cellPoints[i]);
    }
  }
};

struct AppendLegacyFormatImpl
{
  template <typename CellStateT>
  void operator()(
    CellStateT& cells, const vtkIdType* data, const vtkIdType len, const vtkIdType ptOffset) const
  {
    using ValueType = typename CellStateT::ValueType;

    ValueType offset = static_cast<ValueType>(cells.GetConnectivity()->GetNumberOfValues());

    const vtkIdType* const dataEnd = data + len;
    while (data < dataEnd)
    {
      vtkIdType numPts = *data++;
      offset += static_cast<ValueType>(numPts);
      cells.GetOffsets()->InsertNextValue(offset);
      while (numPts-- > 0)
      {
        cells.GetConnectivity()->InsertNextValue(static_cast<ValueType>(*data++ + ptOffset));
      }
    }
  }
};

struct AppendImpl
{
  // Call this signature:
  template <typename DstCellStateT>
  void operator()(DstCellStateT& dstcells, vtkCellArray* src, vtkIdType pointOffset) const
  { // dispatch on src:
    src->Visit(*this, dstcells, pointOffset);
  }

  // Above signature calls this operator in Visit:
  template <typename SrcCellStateT, typename DstCellStateT>
  void operator()(SrcCellStateT& src, DstCellStateT& dst, vtkIdType pointOffsets) const
  {
    this->AppendArrayWithOffset(
      src.GetOffsets(), dst.GetOffsets(), dst.GetConnectivity()->GetNumberOfValues(), true);
    this->AppendArrayWithOffset(src.GetConnectivity(), dst.GetConnectivity(), pointOffsets, false);
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
    dstArray->InsertValue(dstEnd - 1, 0);

    const auto srcRange = vtk::DataArrayValueRange<1>(srcArray, skipFirst ? 1 : 0);
    auto dstRange = vtk::DataArrayValueRange<1>(dstArray, dstBegin, dstEnd);
    assert(srcRange.size() == dstRange.size());

    const DstValueType dOffset = static_cast<DstValueType>(offset);

    std::transform(srcRange.cbegin(), srcRange.cend(), dstRange.begin(),
      [&](SrcValueType x) -> DstValueType { return static_cast<DstValueType>(x) + dOffset; });
  }
};

} // end anon namespace

vtkCellArray::vtkCellArray() = default;
vtkCellArray::~vtkCellArray() = default;
vtkStandardNewMacro(vtkCellArray);

//=================== Begin Legacy Methods ===================================
// These should be deprecated at some point as they are confusing or very slow

//----------------------------------------------------------------------------
vtkIdType vtkCellArray::GetSize()
{
  // We can still compute roughly the same result, so go ahead and do that.
  return this->Visit(deprec::GetSizeImpl{});
}

//----------------------------------------------------------------------------
vtkIdType vtkCellArray::GetNumberOfConnectivityEntries()
{
  // We can still compute roughly the same result, so go ahead and do that.
  return this->Visit(GetLegacyDataSizeImpl{});
}

//----------------------------------------------------------------------------
void vtkCellArray::GetCell(vtkIdType loc, vtkIdType& npts, const vtkIdType*& pts)
{
  const vtkIdType cellId = this->Visit(deprec::LocationToCellIdFunctor{}, loc);
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

//----------------------------------------------------------------------------
void vtkCellArray::GetCell(vtkIdType loc, vtkIdList* pts)
{
  const vtkIdType cellId = this->Visit(deprec::LocationToCellIdFunctor{}, loc);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location.");
    pts->Reset();
    return;
  }

  this->GetCellAtId(cellId, pts);
}

//----------------------------------------------------------------------------
vtkIdType vtkCellArray::GetInsertLocation(int npts)
{
  // It looks like the original implementation of this actually returned the
  // location of the last cell (of size npts), not the current insert location.
  return this->Visit(deprec::GetInsertLocationImpl{}) - npts - 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkCellArray::GetTraversalLocation()
{
  return this->Visit(deprec::CellIdToLocationFunctor{}, this->GetTraversalCellId());
}

//----------------------------------------------------------------------------
vtkIdType vtkCellArray::GetTraversalLocation(vtkIdType npts)
{
  return this->Visit(deprec::CellIdToLocationFunctor{}, this->GetTraversalCellId()) - npts - 1;
}

//----------------------------------------------------------------------------
void vtkCellArray::SetTraversalLocation(vtkIdType loc)
{
  const vtkIdType cellId = this->Visit(deprec::LocationToCellIdFunctor{}, loc);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location, ignoring.");
    return;
  }

  this->SetTraversalCellId(cellId);
}

//----------------------------------------------------------------------------
vtkIdType vtkCellArray::EstimateSize(vtkIdType numCells, int maxPtsPerCell)
{
  return numCells * (1 + maxPtsPerCell);
}

//----------------------------------------------------------------------------
void vtkCellArray::SetNumberOfCells(vtkIdType)
{
  // no-op
}

//----------------------------------------------------------------------------
void vtkCellArray::ReverseCell(vtkIdType loc)
{
  const vtkIdType cellId = this->Visit(deprec::LocationToCellIdFunctor{}, loc);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location, ignoring.");
    return;
  }

  this->ReverseCellAtId(cellId);
}

//----------------------------------------------------------------------------
void vtkCellArray::ReplaceCell(vtkIdType loc, int npts, const vtkIdType pts[])
{
  const vtkIdType cellId = this->Visit(deprec::LocationToCellIdFunctor{}, loc);
  if (cellId < 0)
  {
    vtkErrorMacro("Invalid location, ignoring.");
    return;
  }

  this->ReplaceCellAtId(cellId, static_cast<vtkIdType>(npts), pts);
}

//----------------------------------------------------------------------------
vtkIdTypeArray* vtkCellArray::GetData()
{
  this->ExportLegacyFormat(this->LegacyData);

  return this->LegacyData;
}

//----------------------------------------------------------------------------
// Specify a group of cells.
void vtkCellArray::SetCells(vtkIdType ncells, vtkIdTypeArray* cells)
{
  this->AllocateExact(ncells, cells->GetNumberOfValues() - ncells);
  this->ImportLegacyFormat(cells);
}

//=================== End Legacy Methods =====================================

//----------------------------------------------------------------------------
void vtkCellArray::DeepCopy(vtkCellArray* ca)
{
  if (ca == this)
  {
    return;
  }

  if (ca->Storage.Is64Bit())
  {
    this->Storage.Use64BitStorage();
    auto& srcStorage = ca->Storage.GetArrays64();
    auto& dstStorage = this->Storage.GetArrays64();
    dstStorage.Offsets->DeepCopy(srcStorage.Offsets);
    dstStorage.Connectivity->DeepCopy(srcStorage.Connectivity);
    this->Modified();
  }
  else
  {
    this->Storage.Use32BitStorage();
    auto& srcStorage = ca->Storage.GetArrays32();
    auto& dstStorage = this->Storage.GetArrays32();
    dstStorage.Offsets->DeepCopy(srcStorage.Offsets);
    dstStorage.Connectivity->DeepCopy(srcStorage.Connectivity);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCellArray::ShallowCopy(vtkCellArray* ca)
{
  if (ca == this)
  {
    return;
  }

  if (ca->Storage.Is64Bit())
  {
    auto& srcStorage = ca->Storage.GetArrays64();
    this->SetData(srcStorage.GetOffsets(), srcStorage.GetConnectivity());
  }
  else
  {
    auto& srcStorage = ca->Storage.GetArrays32();
    this->SetData(srcStorage.GetOffsets(), srcStorage.GetConnectivity());
  }
}

//----------------------------------------------------------------------------
void vtkCellArray::Append(vtkCellArray* src, vtkIdType pointOffset)
{
  if (src->GetNumberOfCells() > 0)
  {
    this->Visit(AppendImpl{}, src, pointOffset);
  }
}

//----------------------------------------------------------------------------
void vtkCellArray::Initialize()
{
  this->Visit(InitializeImpl{});

  this->LegacyData->Initialize();
}

//----------------------------------------------------------------------------
vtkCellArrayIterator* vtkCellArray::NewIterator()
{
  vtkCellArrayIterator* iter = vtkCellArrayIterator::New();
  iter->SetCellArray(this);
  iter->GoToFirstCell();
  return iter;
}

//----------------------------------------------------------------------------
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
  storage.Offsets = vtkArrayDownCast<ArrayType32>(offsets);
  storage.Connectivity = vtkArrayDownCast<ArrayType32>(connectivity);
  this->Modified();
}

//----------------------------------------------------------------------------
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
  storage.Offsets = vtkArrayDownCast<ArrayType64>(offsets);
  storage.Connectivity = vtkArrayDownCast<ArrayType64>(connectivity);
  this->Modified();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

} // end anon namespace

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkCellArray::Use32BitStorage()
{
  if (!this->Storage.Is64Bit())
  {
    this->Initialize();
    return;
  }
  this->Storage.Use32BitStorage();
}

//----------------------------------------------------------------------------
void vtkCellArray::Use64BitStorage()
{
  if (this->Storage.Is64Bit())
  {
    this->Initialize();
    return;
  }
  this->Storage.Use64BitStorage();
}

//----------------------------------------------------------------------------
void vtkCellArray::UseDefaultStorage()
{
#ifdef VTK_USE_64BIT_IDS
  this->Use64BitStorage();
#else  // VTK_USE_64BIT_IDS
  this->Use32BitStorage();
#endif // VTK_USE_64BIT_IDS
}

//----------------------------------------------------------------------------
bool vtkCellArray::CanConvertTo32BitStorage() const
{
  if (!this->Storage.Is64Bit())
  {
    return true;
  }
  return this->Visit(CanConvert<ArrayType32::ValueType>{});
}

//----------------------------------------------------------------------------
bool vtkCellArray::CanConvertTo64BitStorage() const
{
  return true;
}

//----------------------------------------------------------------------------
bool vtkCellArray::CanConvertToDefaultStorage() const
{
#ifdef VTK_USE_64BIT_IDS
  return this->CanConvertTo64BitStorage();
#else  // VTK_USE_64BIT_IDS
  return this->CanConvertTo32BitStorage();
#endif // VTK_USE_64BIT_IDS
}

//----------------------------------------------------------------------------
bool vtkCellArray::ConvertTo32BitStorage()
{
  if (!this->IsStorage64Bit())
  {
    return true;
  }
  vtkNew<ArrayType32> offsets;
  vtkNew<ArrayType32> conn;
  if (!this->Visit(ExtractAndInitialize{}, offsets.Get(), conn.Get()))
  {
    return false;
  }

  this->SetData(offsets, conn);
  return true;
}

//----------------------------------------------------------------------------
bool vtkCellArray::ConvertTo64BitStorage()
{
  if (this->IsStorage64Bit())
  {
    return true;
  }
  vtkNew<ArrayType64> offsets;
  vtkNew<ArrayType64> conn;
  if (!this->Visit(ExtractAndInitialize{}, offsets.Get(), conn.Get()))
  {
    return false;
  }

  this->SetData(offsets, conn);
  return true;
}

//----------------------------------------------------------------------------
bool vtkCellArray::ConvertToDefaultStorage()
{
#ifdef VTK_USE_64BIT_IDS
  return this->ConvertTo64BitStorage();
#else  // VTK_USE_64BIT_IDS
  return this->ConvertTo32BitStorage();
#endif // VTK_USE_64BIT_IDS
}

//----------------------------------------------------------------------------
bool vtkCellArray::ConvertToSmallestStorage()
{
  if (this->IsStorage64Bit() && this->CanConvertTo32BitStorage())
  {
    return this->ConvertTo32BitStorage();
  }
  // Already at the smallest possible.
  return true;
}

//----------------------------------------------------------------------------
bool vtkCellArray::AllocateExact(vtkIdType numCells, vtkIdType connectivitySize)
{
  return this->Visit(AllocateExactImpl{}, numCells, connectivitySize);
}

//----------------------------------------------------------------------------
bool vtkCellArray::ResizeExact(vtkIdType numCells, vtkIdType connectivitySize)
{
  return this->Visit(ResizeExactImpl{}, numCells, connectivitySize);
}

//----------------------------------------------------------------------------
// Returns the size of the largest cell. The size is the number of points
// defining the cell.
int vtkCellArray::GetMaxCellSize()
{
  FindMaxCell finder{ this };

  // Grain size puts an even number of pages into each instance.
  vtkSMPTools::For(0, this->GetNumberOfCells(), finder);

  return static_cast<int>(finder.Result);
}

//----------------------------------------------------------------------------
unsigned long vtkCellArray::GetActualMemorySize() const
{
  return this->Visit(GetActualMemorySizeImpl{});
}

//----------------------------------------------------------------------------
void vtkCellArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StorageIs64Bit: " << this->Storage.Is64Bit() << "\n";

  PrintSelfImpl functor;
  this->Visit(functor, os, indent);
}

//----------------------------------------------------------------------------
void vtkCellArray::PrintDebug(std::ostream& os)
{
  this->Print(os);
  this->Visit(PrintDebugImpl{}, os);
}

//----------------------------------------------------------------------------
vtkIdType vtkCellArray::GetTraversalCellId()
{
  return this->TraversalCellId;
}

//----------------------------------------------------------------------------
void vtkCellArray::SetTraversalCellId(vtkIdType cellId)
{
  this->TraversalCellId = cellId;
}

//----------------------------------------------------------------------------
void vtkCellArray::ReverseCellAtId(vtkIdType cellId)
{
  this->Visit(ReverseCellAtIdImpl{}, cellId);
}

//----------------------------------------------------------------------------
void vtkCellArray::ReplaceCellAtId(vtkIdType cellId, vtkIdList* list)
{
  this->Visit(ReplaceCellAtIdImpl{}, cellId, list->GetNumberOfIds(), list->GetPointer(0));
}

//----------------------------------------------------------------------------
void vtkCellArray::ReplaceCellAtId(
  vtkIdType cellId, vtkIdType cellSize, const vtkIdType cellPoints[])
{
  this->Visit(ReplaceCellAtIdImpl{}, cellId, cellSize, cellPoints);
}

//----------------------------------------------------------------------------
void vtkCellArray::ExportLegacyFormat(vtkIdTypeArray* data)
{
  data->Allocate(this->Visit(GetLegacyDataSizeImpl{}));

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

//----------------------------------------------------------------------------
void vtkCellArray::ImportLegacyFormat(vtkIdTypeArray* data)
{
  this->ImportLegacyFormat(data->GetPointer(0), data->GetNumberOfValues());
}

//----------------------------------------------------------------------------
void vtkCellArray::ImportLegacyFormat(const vtkIdType* data, vtkIdType len)
{
  this->Reset();
  this->AppendLegacyFormat(data, len, 0);
}

//----------------------------------------------------------------------------
void vtkCellArray::AppendLegacyFormat(vtkIdTypeArray* data, vtkIdType ptOffset)
{
  this->AppendLegacyFormat(data->GetPointer(0), data->GetNumberOfValues(), ptOffset);
}

//----------------------------------------------------------------------------
void vtkCellArray::AppendLegacyFormat(const vtkIdType* data, vtkIdType len, vtkIdType ptOffset)
{
  this->Visit(AppendLegacyFormatImpl{}, data, len, ptOffset);
}

//----------------------------------------------------------------------------
void vtkCellArray::Squeeze()
{
  this->Visit(SqueezeImpl{});

  // Just delete the legacy buffer.
  this->LegacyData->Initialize();
}

//----------------------------------------------------------------------------
bool vtkCellArray::IsValid()
{
  return this->Visit(IsValidImpl{});
}

//----------------------------------------------------------------------------
vtkIdType vtkCellArray::IsHomogeneous()
{
  return this->Visit(IsHomogeneousImpl{});
}
