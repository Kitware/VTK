// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2003 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkSortDataArray.h"

#include "vtkAbstractArray.h"
#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <functional> //std::greater
#include <numeric>

//------------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSortDataArray);

//------------------------------------------------------------------------------
using Arrays = vtkTypeList::Append<vtkArrayDispatch::Arrays, vtkStringArray, vtkVariantArray,
  vtkBitArray>::Result;

//------------------------------------------------------------------------------
vtkSortDataArray::vtkSortDataArray() = default;

//------------------------------------------------------------------------------
vtkSortDataArray::~vtkSortDataArray() = default;

//------------------------------------------------------------------------------
void vtkSortDataArray::Sort(vtkIdList* keys, int dir)
{
  if (keys == nullptr)
  {
    return;
  }
  vtkIdType* data = keys->GetPointer(0);
  vtkIdType numKeys = keys->GetNumberOfIds();
  if (dir == 0)
  {
    vtkSMPTools::Sort(data, data + numKeys);
  }
  else
  {
    vtkSMPTools::Sort(data, data + numKeys, std::greater<>());
  }
}

struct SortAscending
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* array)
  {
    auto data = vtk::DataArrayValueRange<1, T>(array);
    vtkSMPTools::Sort(data.begin(), data.end(), std::less<T>());
  }
};

struct SortDescending
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* array)
  {
    auto data = vtk::DataArrayValueRange<1, vtk::GetAPIType<TArray>>(array);
    vtkSMPTools::Sort(data.begin(), data.end(), std::greater<T>());
  }
};

//------------------------------------------------------------------------------
void vtkSortDataArray::Sort(vtkAbstractArray* keys, int dir)
{
  if (keys == nullptr)
  {
    return;
  }

  if (keys->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Can only sort keys that are 1-tuples.");
    return;
  }

  if (dir == 0)
  {
    SortAscending worker;
    if (!vtkArrayDispatch::DispatchByArray<Arrays>::Execute(keys, worker))
    {
      if (auto da = vtkDataArray::SafeDownCast(keys))
      {
        switch (da->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(da)));
        }
      }
    }
  }
  else
  {
    SortDescending worker;
    if (!vtkArrayDispatch::DispatchByArray<Arrays>::Execute(keys, worker))
    {
      if (auto da = vtkDataArray::SafeDownCast(keys))
      {
        switch (da->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(da)));
        }
      }
    }
  }
}
VTK_ABI_NAMESPACE_END

//------------------------------------------------------------------------------
// Hide some stuff; mostly things plugged into templated functions
namespace
{

//------------------------------------------------------------------------------
// We sort the indices based on a key value in another array. Produces sort
// in ascending direction. Note that sort comparison operator is for single
// component arrays.
template <typename TArray, typename T>
struct KeyComp
{
  vtk::detail::ValueRange<TArray, 1, T> Array;
  KeyComp(TArray* array)
    : Array(vtk::DataArrayValueRange<1, T>(array))
  {
  }
  bool operator()(vtkIdType idx0, vtkIdType idx1) const
  {
    return static_cast<T>(Array[idx0]) < static_cast<T>(Array[idx1]);
  }
};

//------------------------------------------------------------------------------
// Special comparison functor using tuple component as a key. Note that this
// comparison function is for general arrays of n components.
template <typename TArray, typename T>
struct TupleComp
{
  vtk::detail::ValueRange<TArray, vtk::detail::DynamicTupleSize, T> Array;
  int NumComp;
  int K;
  TupleComp(TArray* array, int k)
    : Array(vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(array))
    , NumComp(array->GetNumberOfComponents())
    , K(k)
  {
  }
  bool operator()(vtkIdType idx0, vtkIdType idx1) const
  {
    return static_cast<T>(Array[idx0 * NumComp + K]) < static_cast<T>(Array[idx1 * NumComp + K]);
  }
};

//------------------------------------------------------------------------------
// Given a set of indices (after sorting), copy the data from a pre-sorted
// array to a final, post-sorted array, Implementation note: the direction of
// sort (dir) is treated here rather than in the std::sort() function to
// reduce object file .obj size; e.g., running std::sort with a different
// comparator function causes inline expansion to produce very large object
// files.
struct Shuffle1Tuples
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* arrayIn, vtkIdType* idx, int dir)
  {
    vtkIdType size = arrayIn->GetNumberOfTuples();
    if (arrayIn->HasStandardMemoryLayout())
    {
      auto in = vtk::DataArrayValueRange<1, T>(arrayIn);
      T* out = new T[size];

      if (dir == 0) // ascending
      {
        for (vtkIdType i = 0; i < size; ++i)
        {
          out[i] = in[idx[i]];
        }
      }
      else // descending
      {
        vtkIdType end = size - 1;
        for (vtkIdType i = 0; i < size; ++i)
        {
          out[i] = in[idx[end - i]];
        }
      }
      arrayIn->SetVoidArray(out, size, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
    }
    else
    {
      auto arrayInCopy = vtk::TakeSmartPointer(
        TArray::FastDownCast(vtkAbstractArray::CreateArray(arrayIn->GetDataType())));
      arrayInCopy->DeepCopy(arrayIn); // make a contiguous copy
      auto in = vtk::DataArrayValueRange<1, T>(arrayInCopy);
      auto out = vtk::DataArrayValueRange<1, T>(arrayIn);
      if (dir == 0) // ascending
      {
        for (vtkIdType i = 0; i < size; ++i)
        {
          out[i] = in[idx[i]];
        }
      }
      else // descending
      {
        vtkIdType end = size - 1;
        for (vtkIdType i = 0; i < size; ++i)
        {
          out[i] = in[idx[end - i]];
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
// Given a set of indices (after sorting), copy the data from a pre-sorted
// data array to a final, post-sorted array. Note that the data array is
// assumed to have arbitrary sized components.
struct ShuffleTuples
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* arrayIn, vtkIdType* idx, int dir)
  {
    vtkIdType size = arrayIn->GetNumberOfTuples();
    if (arrayIn->HasStandardMemoryLayout())
    {
      auto arrayOut = vtk::TakeSmartPointer(
        TArray::FastDownCast(vtkAbstractArray::CreateArray(arrayIn->GetDataType())));
      arrayOut->SetNumberOfComponents(arrayIn->GetNumberOfComponents());
      arrayOut->SetNumberOfTuples(size);
      auto in = vtk::DataArrayTupleRange(arrayIn);
      auto out = vtk::DataArrayTupleRange(arrayOut);

      if (dir == 0) // ascending
      {
        for (vtkIdType i = 0; i < size; ++i)
        {
          out[i] = in[idx[i]];
        }
      }
      else // descending
      {
        vtkIdType end = size - 1;
        for (vtkIdType i = 0; i < size; ++i)
        {
          out[i] = in[idx[end - i]];
        }
      }
      arrayIn->ShallowCopy(arrayOut); // replace contents without
    }
    else
    {
      auto arrayInCopy = vtk::TakeSmartPointer(
        TArray::FastDownCast(vtkAbstractArray::CreateArray(arrayIn->GetDataType())));
      arrayInCopy->DeepCopy(arrayIn); // make a contiguous copy
      auto in = vtk::DataArrayTupleRange(arrayInCopy);
      auto out = vtk::DataArrayTupleRange(arrayIn);
      if (dir == 0) // ascending
      {
        for (vtkIdType i = 0; i < size; ++i)
        {
          out[i] = in[idx[i]];
        }
      }
      else // descending
      {
        vtkIdType end = size - 1;
        for (vtkIdType i = 0; i < size; ++i)
        {
          out[i] = in[idx[end - i]];
        }
      }
    }
  }
};

} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// Allocate and initialize sort indices
vtkIdType* vtkSortDataArray::InitializeSortIndices(vtkIdType num)
{
  vtkIdType* idx = new vtkIdType[num];
  std::iota(idx, idx + num, static_cast<vtkIdType>(0));
  return idx;
}

struct Sort1Array
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* arrayIn, vtkIdType* idx)
  {
    vtkIdType size = arrayIn->GetNumberOfTuples();
    vtkSMPTools::Sort(idx, idx + size, KeyComp<TArray, T>(arrayIn));
  }
};

struct SortArray
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* arrayIn, vtkIdType* idx, int k)
  {
    vtkIdType size = arrayIn->GetNumberOfTuples();
    vtkSMPTools::Sort(idx, idx + size, TupleComp<TArray, T>(arrayIn, k));
  }
};

//------------------------------------------------------------------------------
// Efficient function for generating sort ordering specialized to single
// component arrays.
void vtkSortDataArray::GenerateSort1Indices(vtkAbstractArray* arr, vtkIdType* idx)
{
  Sort1Array worker;
  if (!vtkArrayDispatch::DispatchByArray<Arrays>::Execute(arr, worker, idx))
  {
    if (auto da = vtkDataArray::SafeDownCast(arr))
    {
      switch (da->GetDataType())
      {
        vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(da, idx)));
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkSortDataArray::GenerateSort1Indices(
  int dataType, void* dataIn, vtkIdType numKeys, vtkIdType* idx)
{
  auto arr = vtk::TakeSmartPointer(vtkAbstractArray::CreateArray(dataType));
  arr->SetVoidArray(dataIn, numKeys, 1);
  vtkSortDataArray::GenerateSort1Indices(arr, idx);
}

//------------------------------------------------------------------------------
// Function for generating sort ordering for general arrays.
void vtkSortDataArray::GenerateSortIndices(vtkAbstractArray* arr, int k, vtkIdType* idx)
{
  // Specialized and faster for single component arrays
  if (arr->GetNumberOfComponents() == 1)
  {
    vtkSortDataArray::GenerateSort1Indices(arr, idx);
    return;
  }

  SortArray worker;
  if (!vtkArrayDispatch::DispatchByArray<Arrays>::Execute(arr, worker, idx, k))
  {
    if (auto da = vtkDataArray::SafeDownCast(arr))
    {
      switch (da->GetDataType())
      {
        vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(da, idx, k)));
      }
    }
  }
}

//------------------------------------------------------------------------------
// Function for generating sort ordering for general arrays.
void vtkSortDataArray::GenerateSortIndices(
  int dataType, void* dataIn, vtkIdType numKeys, int numComp, int k, vtkIdType* idx)
{
  auto arr = vtk::TakeSmartPointer(vtkAbstractArray::CreateArray(dataType));
  arr->SetNumberOfComponents(numComp);
  arr->SetVoidArray(dataIn, numKeys * numComp, 1);
  vtkSortDataArray::GenerateSortIndices(arr, k, idx);
}

//------------------------------------------------------------------------------
// Set up the actual templated shuffling operation. This method is for
// VTK arrays that are precsisely one component.
void vtkSortDataArray::Shuffle1Array(vtkAbstractArray* arr, vtkIdType* idx, int dir)
{
  Shuffle1Tuples worker;
  if (!vtkArrayDispatch::DispatchByArray<Arrays>::Execute(arr, worker, idx, dir))
  {
    if (auto da = vtkDataArray::SafeDownCast(arr))
    {
      switch (da->GetDataType())
      {
        vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(da, idx, dir)));
      }
    }
  }
}

//------------------------------------------------------------------------------
// Set up the actual templated shuffling operation
void vtkSortDataArray::ShuffleArray(vtkAbstractArray* arr, vtkIdType* idx, int dir)
{
  // Specialized for single component arrays
  if (arr->GetNumberOfComponents() == 1)
  {
    vtkSortDataArray::Shuffle1Array(arr, idx, dir);
  }
  else
  {
    ShuffleTuples worker;
    if (!vtkArrayDispatch::DispatchByArray<Arrays>::Execute(arr, worker, idx, dir))
    {
      if (auto da = vtkDataArray::SafeDownCast(arr))
      {
        worker(da, idx, dir);
      }
    }
  }
}

//------------------------------------------------------------------------------
// Given a set of indices (after sorting), copy the ids from a pre-sorted
// id array to a final, post-sorted array.
void vtkSortDataArray::ShuffleIdList(
  vtkIdType* idx, vtkIdType sze, vtkIdList* arrayIn, vtkIdType* preSort, int dir)
{
  vtkIdType* postSort = new vtkIdType[sze];

  if (dir == 0) // ascending
  {
    for (vtkIdType i = 0; i < sze; ++i)
    {
      postSort[i] = preSort[idx[i]];
    }
  }
  else
  {
    vtkIdType end = sze - 1;
    for (vtkIdType i = 0; i < sze; ++i)
    {
      postSort[i] = preSort[idx[end - i]];
    }
  }

  arrayIn->SetArray(postSort, sze);
}

//------------------------------------------------------------------------------
// Sort a position index based on the values in the abstract array. Once
// sorted, then shuffle the keys and values around into new arrays.
void vtkSortDataArray::Sort(vtkAbstractArray* keys, vtkAbstractArray* values, int dir)
{
  // Check input
  if (keys == nullptr || values == nullptr)
  {
    return;
  }
  if (keys->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Can only sort keys that are 1-tuples.");
    return;
  }
  vtkIdType numKeys = keys->GetNumberOfTuples();
  vtkIdType numValues = values->GetNumberOfTuples();
  if (numKeys != numValues)
  {
    vtkGenericWarningMacro("Could not sort arrays.  Key and value arrays have different sizes.");
    return;
  }

  // Sort the index array
  vtkIdType* idx = vtkSortDataArray::InitializeSortIndices(numKeys);

  // Generate the sorting index array
  vtkSortDataArray::GenerateSortIndices(keys, 0, idx);

  // Now shuffle data around based on sorted indices
  vtkSortDataArray::ShuffleArray(keys, idx, dir);

  vtkSortDataArray::ShuffleArray(values, idx, dir);

  // Clean up
  delete[] idx;
}

//------------------------------------------------------------------------------
void vtkSortDataArray::Sort(vtkAbstractArray* keys, vtkIdList* values, int dir)
{
  // Check input
  if (keys == nullptr || values == nullptr)
  {
    return;
  }
  if (keys->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Can only sort keys that are 1-tuples.");
    return;
  }
  vtkIdType numKeys = keys->GetNumberOfTuples();
  vtkIdType numIds = values->GetNumberOfIds();
  if (numKeys != numIds)
  {
    vtkGenericWarningMacro("Could not sort arrays.  Key and id arrays have different sizes.");
    return;
  }

  // Sort the index array
  vtkIdType* idx = vtkSortDataArray::InitializeSortIndices(numKeys);

  // Generate the sorting index array
  vtkSortDataArray::GenerateSortIndices(keys, 0, idx);

  // Shuffle the keys
  vtkSortDataArray::ShuffleArray(keys, idx, dir);

  // Now shuffle the ids to match the sort
  vtkIdType* ids = values->GetPointer(0);
  ShuffleIdList(idx, numKeys, values, ids, dir);

  // Clean up
  delete[] idx;
}

//------------------------------------------------------------------------------
void vtkSortDataArray::SortArrayByComponent(vtkAbstractArray* arr, int k, int dir)
{
  // Check input
  if (arr == nullptr)
  {
    return;
  }
  vtkIdType numKeys = arr->GetNumberOfTuples();
  int nc = arr->GetNumberOfComponents();

  if (k < 0 || k >= nc)
  {
    vtkGenericWarningMacro(
      "Cannot sort by column " << k << " since the array only has columns 0 through " << (nc - 1));
    return;
  }

  // Perform the sort
  vtkIdType* idx = vtkSortDataArray::InitializeSortIndices(numKeys);

  vtkSortDataArray::GenerateSortIndices(arr, k, idx);

  vtkSortDataArray::ShuffleArray(arr, idx, dir);

  // Clean up
  delete[] idx;
}

//------------------------------------------------------------------------------
void vtkSortDataArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// vtkSortDataArray methods -------------------------------------------------------
VTK_ABI_NAMESPACE_END
