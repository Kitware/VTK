// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAbstractArray.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"

#include <cassert>

// Silence warnings that these functions are unused.
// Because the functions are in an anonymous namespace, unused-function
// function warnings are generated. But not every file that includes
// this header needs to use every function.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-template"
#endif

VTK_ABI_NAMESPACE_BEGIN
namespace
{
template <typename T>
bool Synchronize(vtkMultiProcessController* controller, T& data, T& result)
{
  if (controller == nullptr || controller->GetNumberOfProcesses() <= 1)
  {
    return true;
  }

  vtkMultiProcessStream stream;
  stream << data;

  std::vector<vtkMultiProcessStream> all_streams;
  if (controller->AllGather(stream, all_streams))
  {
    for (auto& s : all_streams)
    {
      s >> result;
    }
    return true;
  }

  return false;
}

template <typename T>
bool Broadcast(vtkMultiProcessController* controller, T& data, int root)
{
  if (controller == nullptr || controller->GetNumberOfProcesses() <= 1)
  {
    return true;
  }
  if (controller->GetLocalProcessId() == root)
  {
    vtkMultiProcessStream stream;
    stream << data;
    return controller->Broadcast(stream, root) != 0;
  }
  else
  {
    data = T();
    vtkMultiProcessStream stream;
    if (controller->Broadcast(stream, root))
    {
      stream >> data;
      return true;
    }
    return false;
  }
}

vtkSmartPointer<vtkAbstractArray> JoinArrays(
  const std::vector<vtkSmartPointer<vtkAbstractArray>>& arrays)
{
  if (arrays.empty())
  {
    return nullptr;
  }
  else if (arrays.size() == 1)
  {
    return arrays[0];
  }

  vtkIdType numTuples = 0;
  for (auto& array : arrays)
  {
    numTuples += array->GetNumberOfTuples();
  }

  vtkSmartPointer<vtkAbstractArray> result;
  result.TakeReference(arrays[0]->NewInstance());
  result->CopyInformation(arrays[0]->GetInformation());
  result->SetName(arrays[0]->GetName());
  result->SetNumberOfComponents(arrays[0]->GetNumberOfComponents());
  result->SetNumberOfTuples(numTuples);
  vtkIdType offset = 0;
  for (auto& array : arrays)
  {
    const auto count = array->GetNumberOfTuples();
    result->InsertTuples(offset, count, 0, array);
    offset += count;
  }
  result->Modified();
  assert(offset == numTuples);
  return result;
}

} // end of namespace {}
VTK_ABI_NAMESPACE_END

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif
