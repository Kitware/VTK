// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkXMLUnstructuredPolyReader_txx
#define vtkXMLUnstructuredPolyReader_txx

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <array>
#include <cassert> // for assert()
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace vtkXMLUnstructuredDataReaderPrivate
{
VTK_ABI_NAMESPACE_BEGIN

template <typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class FaceIdMinAndMax
{
private:
  ArrayT* Array;
  APIType ReducedRange[2];
  vtkSMPThreadLocal<std::array<APIType, 2>> TLRange;

public:
  FaceIdMinAndMax(ArrayT* array)
    : Array(array)
  {
    this->ReducedRange[0] = vtkTypeTraits<APIType>::Max();
    this->ReducedRange[1] = vtkTypeTraits<APIType>::Min();
  }
  // Help vtkSMPTools find Initialize() and Reduce()
  void Initialize()
  {
    auto& range = this->TLRange.Local();
    range[0] = vtkTypeTraits<APIType>::Max();
    range[1] = vtkTypeTraits<APIType>::Min();
  }
  void Reduce()
  {
    for (auto itr = this->TLRange.begin(); itr != this->TLRange.end(); ++itr)
    {
      auto& range = *itr;
      this->ReducedRange[0] = vtkMath::Min(this->ReducedRange[0], range[0]);
      this->ReducedRange[1] = vtkMath::Max(this->ReducedRange[1], range[1]);
    }
  }

  template <typename T>
  void CopyRanges(T* ranges)
  {
    ranges[0] = static_cast<T>(this->ReducedRange[0]);
    ranges[1] = static_cast<T>(this->ReducedRange[1]);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    const auto tuples = vtk::DataArrayValueRange<1>(this->Array, begin, end);
    auto& range = FaceIdMinAndMax::TLRange.Local();
    for (const APIType value : tuples)
    {
      if (value < 0)
      {
        // ignored for now
        // but can be needed for compact storage in the future.
        continue;
      }
      else
      {
        vtkMathUtilities::UpdateRange(range[0], range[1], value);
      }
    }
  }
};

struct FaceIdRangeDispatchWrapper
{
  bool Successful;
  vtkIdType* Range;

  FaceIdRangeDispatchWrapper(vtkIdType* range)
    : Successful(false)
    , Range(range)
  {
  }

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    using T = vtk::GetAPIType<ArrayT>;
    T range[2];
    const vtkIdType numTuples = array->GetNumberOfTuples();
    range[0] = vtkTypeTraits<T>::Max();
    range[1] = vtkTypeTraits<T>::Min();

    // do this after we make sure range is max to min
    if (numTuples == 0)
    {
      this->Successful = false;
      this->Range[0] = static_cast<vtkIdType>(range[0]);
      this->Range[1] = static_cast<vtkIdType>(range[1]);
      return;
    }

    FaceIdMinAndMax<ArrayT> MinAndMax(array);
    vtkSMPTools::For(0, numTuples, MinAndMax);
    MinAndMax.CopyRanges(range);
    this->Range[0] = static_cast<vtkIdType>(range[0]);
    this->Range[1] = static_cast<vtkIdType>(range[1]);

    this->Successful = true;
  }
};

bool FindPolyFaceRange(vtkIdType* ranges, vtkDataArray* polyhedron_faces)
{
  using Dispatcher = vtkArrayDispatch::DispatchByArray<vtkCellArray::StorageArrayList>;

  FaceIdRangeDispatchWrapper worker(ranges);

  if (!Dispatcher::Execute(polyhedron_faces, worker))
  {
    worker(polyhedron_faces);
  }
  return worker.Successful;
}

template <typename ArrayT, typename APIType = typename vtk::GetAPIType<ArrayT>>
class Offsetter
{
private:
  ArrayT* Array;
  APIType Offset;
  vtkSMPThreadLocal<std::array<APIType, 2>> TLRange;

public:
  Offsetter(ArrayT* array, APIType value)
    : Array(array)
    , Offset(value)
  {
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto range = vtk::DataArrayValueRange<1>(this->Array, begin, end);
    using RefType = typename decltype(range)::ReferenceType;
    APIType val = this->Offset;
    for (RefType value : range)
    {
      value -= val;
    }
  }
};

struct OffsettingWrapper
{
  bool Successful;
  bool UseStartValue;
  vtkIdType Value;

  OffsettingWrapper(vtkIdType val)
    : Successful(false)
    , UseStartValue(false)
    , Value(val)
  {
  }

  template <typename ArrayT>
  vtk::GetAPIType<ArrayT> InitOffsetWithFirstValue(ArrayT* array)
  {
    return array->GetValue(0);
  }

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    using T = vtk::GetAPIType<ArrayT>;
    T offsetValue;
    const vtkIdType numTuples = array->GetNumberOfTuples();

    if (numTuples == 0)
    {
      this->Successful = true;
      return;
    }

    if (this->UseStartValue)
    {
      offsetValue = static_cast<T>(this->InitOffsetWithFirstValue(array));
      this->Value = static_cast<vtkIdType>(offsetValue);
    }
    else
    {
      offsetValue = static_cast<T>(this->Value);
    }
    if (offsetValue == 0)
    {
      this->Successful = true;
      return;
    }

    Offsetter<ArrayT> translator(array, offsetValue);
    vtkSMPTools::For(0, numTuples, translator);

    this->Successful = true;
  }
};

template <>
vtk::GetAPIType<vtkDataArray> OffsettingWrapper::InitOffsetWithFirstValue(vtkDataArray* array)
{
  using T = vtk::GetAPIType<vtkDataArray>;
  T startValue;
  startValue = static_cast<T>(array->GetTuple1(0));
  return startValue;
}

bool RebaseOffset(vtkDataArray* arr)
{
  using Dispatcher = vtkArrayDispatch::DispatchByArray<vtkCellArray::StorageArrayList>;

  OffsettingWrapper worker(0);
  worker.UseStartValue = true;

  if (!Dispatcher::Execute(arr, worker))
  {
    worker(arr);
  }

  if (!Dispatcher::Execute(arr, worker))
  {
    worker(arr);
  }
  return worker.Successful;
}

bool RebasePolyFaces(vtkDataArray* arr, vtkIdType offset)
{
  using Dispatcher = vtkArrayDispatch::DispatchByArray<vtkCellArray::StorageArrayList>;

  OffsettingWrapper worker(offset);
  worker.UseStartValue = false;

  if (!Dispatcher::Execute(arr, worker))
  {
    worker(arr);
  }
  return worker.Successful;
}

VTK_ABI_NAMESPACE_END
} // end namespace vtkXMLUnstructuredDataReaderPrivate
#endif
// VTK-HeaderTest-Exclude: vtkXMLUnstructuredPolyReader.txx
