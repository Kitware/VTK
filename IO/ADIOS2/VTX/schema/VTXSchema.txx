// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * VTXSchema.txx
 *
 *  Created on: May 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_SCHEMA_VTXSchema_tcc
#define VTK_IO_ADIOS2_SCHEMA_VTXSchema_tcc

#include "VTXSchema.h"

#include <stdexcept>

#include "VTX/common/VTXHelper.h"

namespace vtx
{
VTK_ABI_NAMESPACE_BEGIN

template <class T>
void VTXSchema::GetDataArrayCommon(
  adios2::Variable<T> variable, types::DataArray& dataArray, size_t step)
{
  dataArray.IsUpdated = true;

  if (variable.Steps() < 2)
  {
    const auto blocksInfo = this->Engine.BlocksInfo(variable, step);
    if (blocksInfo.empty())
    {
      dataArray.IsUpdated = false;
      return;
    }
  }
  else
  {
    variable.SetStepSelection({ step, 1 });
  }

  if (variable.ShapeID() == adios2::ShapeID::GlobalArray)
  {
    // determine the selection per rank
    GetDataArrayGlobal(variable, dataArray, step);
  }
  else if (variable.ShapeID() == adios2::ShapeID::LocalArray)
  {
    // determine the blocks per rank
    GetDataArrayLocal(variable, dataArray, step);
  }
  else if (variable.ShapeID() == adios2::ShapeID::GlobalValue)
  {
    GetDataValueGlobal(variable, dataArray, step);
  }
}

template <class T>
void VTXSchema::GetDataArrayGlobal(
  adios2::Variable<T> variable, types::DataArray& dataArray, size_t step)
{
  SetDimensions(variable, dataArray, step);
  const size_t elements = helper::TotalElements(dataArray.Count);
  // TODO: enable vectors
  InitDataArray<T>(variable.Name(), elements, 1, dataArray);
  T* ptr = reinterpret_cast<T*>(dataArray.Data->GetVoidPointer(0));
  this->Engine.Get(variable, ptr);
}

template <class T>
void VTXSchema::GetDataArrayLocal(
  adios2::Variable<T> variable, types::DataArray& dataArray, size_t step)
{
  // set partition: blocks per MPI visualization process
  SetBlocks(variable, dataArray, step);
  size_t elements = 0;
  for (const auto& blockPair : dataArray.BlockCounts)
  {
    elements += helper::TotalElements(blockPair.second);
  }

  // set number of components
  size_t components = 1;
  if (dataArray.HasTuples)
  {
    // last one?
    // TODO: check ordering here
    components = variable.Count().back();
  }
  else
  {
    // linearized vector
    if (variable.Count().size() == 2)
    {
      components = variable.Count().back();
    }
  }

  InitDataArray<T>(variable.Name(), elements, components, dataArray);

  // read blocks in current rank
  size_t offset = 0;
  for (const auto& blockPair : dataArray.BlockCounts)
  {
    variable.SetBlockSelection(blockPair.first);
    T* ptr = reinterpret_cast<T*>(dataArray.Data->GetVoidPointer(offset));
    this->Engine.Get(variable, ptr);
    offset += helper::TotalElements(blockPair.second);
  }
}

template <class T>
void VTXSchema::GetDataValueGlobal(
  adios2::Variable<T> variable, types::DataArray& dataArray, size_t /*step*/)
{
  InitDataArray<T>(variable.Name(), 1, 1, dataArray);
  T* ptr = reinterpret_cast<T*>(dataArray.Data->GetVoidPointer(0));
  this->Engine.Get(variable, ptr);
}

template <class T>
void VTXSchema::InitDataArray(
  const std::string& name, size_t elements, size_t components, types::DataArray& dataArray)
{
  if (dataArray.IsIdType)
  {
    dataArray.Data = helper::NewDataArrayIdType();
  }
  else
  {
    dataArray.Data = helper::NewDataArray<T>();
  }

  dataArray.Data->Allocate(elements);
  dataArray.Data->SetNumberOfComponents(static_cast<int>(components));
  dataArray.Data->SetNumberOfTuples(elements / components);
  dataArray.Data->SetName(name.c_str());
}

template <class T>
void VTXSchema::GetTimesCommon(const std::string& variableName)
{
  adios2::Variable<T> varTime = this->IO.InquireVariable<T>(variableName);
  varTime.SetStepSelection({ 0, varTime.Steps() });
  std::vector<T> timeValues;
  this->Engine.Get(varTime, timeValues, adios2::Mode::Sync);

  size_t currentStep = 0;
  for (const T timeValue : timeValues)
  {
    const double timeDbl = static_cast<double>(timeValue);
    this->Times[timeDbl] = currentStep;
    ++currentStep;
  }
}

VTK_ABI_NAMESPACE_END
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_SCHEMA_VTXSchema_tcc */
