/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractExodusGlobalTemporalVariables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractExodusGlobalTemporalVariables.h"

#include "vtkAbstractArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKey.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

class vtkExtractExodusGlobalTemporalVariables::vtkInternals
{
  std::map<std::string, vtkSmartPointer<vtkAbstractArray> > Arrays;

public:
  bool InContinueExecuting = false;
  size_t Offset = 0;
  std::vector<double> TimeSteps;

  static vtkFieldData* Validate(vtkFieldData* fd)
  {
    vtkNew<vtkInformationIterator> iter;
    for (int cc = 0, max = (fd ? fd->GetNumberOfArrays() : 0); cc < max; ++cc)
    {
      auto arr = fd->GetAbstractArray(cc);
      iter->SetInformationWeak(arr->GetInformation());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        auto key = iter->GetCurrentKey();
        // ref: vtkExodusIIReader::GLOBAL_TEMPORAL_VARIABLE
        if (key && key->GetName() && strcmp(key->GetName(), "GLOBAL_TEMPORAL_VARIABLE") == 0)
        {
          return fd;
        }
      }
      iter->SetInformationWeak(nullptr);
    }
    return nullptr;
  }

  bool ContinueExecuting() const { return (this->Offset < this->TimeSteps.size()); }

  void ResetAccumulatedData()
  {
    this->Arrays.clear();
    this->Offset = 0;
  }

  bool Accumulate(vtkFieldData* fd)
  {
    std::map<std::string, vtkSmartPointer<vtkAbstractArray> > arrays;

    for (int cc = 0, max = (fd ? fd->GetNumberOfArrays() : 0); cc < max; ++cc)
    {
      auto arr = fd->GetAbstractArray(cc);
      vtkNew<vtkInformationIterator> iter;
      iter->SetInformationWeak(arr->GetInformation());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        auto key = iter->GetCurrentKey();
        // ref: vtkExodusIIReader::GLOBAL_TEMPORAL_VARIABLE
        if (key && key->GetName() && strcmp(key->GetName(), "GLOBAL_TEMPORAL_VARIABLE") == 0 &&
          arr->GetName())
        {
          if (arrays.size() == 0 ||
            arrays.begin()->second->GetNumberOfTuples() == arr->GetNumberOfTuples())
          {
            arrays[arr->GetName()] = arr;
          }
        }
      }
    }

    if (arrays.size() == 0)
    {
      return false;
    }

    const auto total_number_of_tuples =
      this->Offset + static_cast<size_t>(arrays.begin()->second->GetNumberOfTuples());
    if (this->Offset == 0)
    {
      // we do shallow copy, if we don't need to loop over timesteps otherwise
      // we deep copy the array to avoid manipulating input values.
      if (total_number_of_tuples == this->TimeSteps.size())
      {
        this->Arrays = std::move(arrays);
      }
      else
      {
        this->Arrays.clear();
        for (const auto& pair : arrays)
        {
          auto array = pair.second->NewInstance();
          array->DeepCopy(pair.second);
          this->Arrays[pair.first].TakeReference(array);
        }
      }
    }
    else
    {
      //  merge arrays.
      std::vector<std::string> to_drop;
      for (auto& dpair : this->Arrays)
      {
        auto iter = arrays.find(dpair.first);
        if (iter == arrays.end())
        {
          to_drop.push_back(dpair.first);
        }
        else
        {
          auto& darray = dpair.second;
          auto& sarray = iter->second;
          darray->InsertTuples(
            static_cast<vtkIdType>(this->Offset), sarray->GetNumberOfTuples(), 0, sarray);
        }
      }
      // remove arrays that were not available in the current set
      // -- this should not happen, but better to handle it.
      for (const auto& aname : to_drop)
      {
        this->Arrays.erase(aname);
      }
    }
    this->Offset = total_number_of_tuples;
    return true;
  }

  void GetResult(vtkTable* table)
  {
    table->Initialize();
    auto rowData = table->GetRowData();
    for (const auto& pair : this->Arrays)
    {
      rowData->AddArray(pair.second);
    }

    // add "Time" array".
    vtkNew<vtkDoubleArray> timeArray;
    timeArray->SetNumberOfComponents(1);
    timeArray->SetNumberOfTuples(static_cast<vtkIdType>(this->TimeSteps.size()));
    timeArray->SetName("Time");
    std::copy(this->TimeSteps.begin(), this->TimeSteps.end(), timeArray->GetPointer(0));
    rowData->AddArray(timeArray);
  }
};

vtkStandardNewMacro(vtkExtractExodusGlobalTemporalVariables);
//----------------------------------------------------------------------------
vtkExtractExodusGlobalTemporalVariables::vtkExtractExodusGlobalTemporalVariables()
  : Internals(new vtkExtractExodusGlobalTemporalVariables::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkExtractExodusGlobalTemporalVariables::~vtkExtractExodusGlobalTemporalVariables() {}

//----------------------------------------------------------------------------
void vtkExtractExodusGlobalTemporalVariables::GetContinuationState(
  bool& continue_executing_flag, size_t& offset) const
{
  auto& internals = (*this->Internals);
  continue_executing_flag = internals.InContinueExecuting;
  offset = internals.Offset;
}

//----------------------------------------------------------------------------
void vtkExtractExodusGlobalTemporalVariables::SetContinuationState(
  bool continue_executing_flag, size_t offset)
{
  auto& internals = (*this->Internals);
  internals.InContinueExecuting = continue_executing_flag;
  internals.Offset = offset;
}

//----------------------------------------------------------------------------
int vtkExtractExodusGlobalTemporalVariables::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractExodusGlobalTemporalVariables::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inInfo = inputVector[0]->GetInformationObject(0);
  auto& internals = (*this->Internals);

  const int size = inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS())
    ? inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS())
    : 0;
  internals.TimeSteps.resize(size);
  internals.Offset = 0;
  internals.InContinueExecuting = false;
  if (size > 0)
  {
    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &internals.TimeSteps[0]);
  }
  vtkLogF(TRACE, "info: num-of-timesteps: %d", size);

  // The output of this filter does not contain a specific time, rather
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractExodusGlobalTemporalVariables::RequestUpdateExtent(vtkInformation*,
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  auto& internals = (*this->Internals);

  // we don't make explicit time-request unless we're looping i.e.
  // `internals.InContinueExecuting == true`. This help us avoid forcing the reader to
  // always read timestep 0 as it is only necessary when we're dealing with
  // restarts. In case of restarts, have to start from the first timestep since
  // it's unclear how to know which set of timesteps are provided by the current
  // dataset.
  if (internals.InContinueExecuting && internals.TimeSteps.size() > 0 &&
    internals.Offset < internals.TimeSteps.size())
  {
    const double timeReq = internals.TimeSteps[internals.Offset];
    auto inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
    vtkLogF(TRACE, "req: timestep %f", timeReq);
  }
  else
  {
    vtkLogF(TRACE, "req: timestep <nothing specific>");
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractExodusGlobalTemporalVariables::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());

  auto& internals = (*this->Internals);
  internals.InContinueExecuting = false;
  if (internals.TimeSteps.size() == 0)
  {
    // nothing to do when data is not temporal.
    vtkLogF(TRACE, "rd: no ts, nothing to do");
    return 1;
  }

  vtkTable* output = vtkTable::GetData(outputVector, 0);

  vtkFieldData* fd = nullptr;
  if (auto cd = vtkCompositeDataSet::GetData(inputVector[0], 0))
  {
    fd = vtkInternals::Validate(cd->GetFieldData());
    auto iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal() && fd == nullptr; iter->GoToNextItem())
    {
      if (auto ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
      {
        fd = vtkInternals::Validate(ds->GetFieldData());
      }
    }
    iter->Delete();
  }
  else if (auto ds = vtkDataSet::GetData(inputVector[0], 0))
  {
    fd = vtkInternals::Validate(ds->GetFieldData());
  }

  if (fd == nullptr)
  {
    // nothing to do.
    vtkLogF(TRACE, "rd: no fd, nothing to do");
    return 1;
  }

  bool isFirst = (internals.Offset == 0);
  internals.Accumulate(fd);
  if (internals.ContinueExecuting())
  {
    // if this is the first time we're executing and we didn't get all timesteps
    // for the global variable, we must discard current values and start from
    // 0 since it's unclear which set of values we processed.
    auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
    if (isFirst && inputDO->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()) &&
      inputDO->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP()) != internals.TimeSteps[0])
    {
      // loop from the beginning.
      internals.ResetAccumulatedData();
      vtkLogF(TRACE, "rd: reset accumulated data to restart from ts 0");
    }
    vtkLogF(TRACE, "rd: collected %d / %d", (int)internals.Offset,
      static_cast<int>(internals.TimeSteps.size()));
    internals.InContinueExecuting = true;
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    return 1;
  }
  else
  {
    // produce output only for piece 0.
    vtkLogF(TRACE, "rd: collected %d / %d", (int)internals.Offset,
      static_cast<int>(internals.TimeSteps.size()));
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    if (!outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) ||
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) == 0)
    {
      vtkLogF(TRACE, "rd: populate result");
      internals.GetResult(output);
    }
    else
    {
      vtkLogF(TRACE, "rd: empty result");
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
void vtkExtractExodusGlobalTemporalVariables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
