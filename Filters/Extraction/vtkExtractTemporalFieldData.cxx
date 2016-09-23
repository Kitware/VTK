/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractTemporalFieldData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractTemporalFieldData.h"

#include "vtkCompositeDataIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <vector>

class vtkExtractTemporalFieldData::vtkInternals
{
public:
  std::vector<double> TimeSteps;
};

vtkObjectFactoryNewMacro(vtkExtractTemporalFieldData);
//----------------------------------------------------------------------------
vtkExtractTemporalFieldData::vtkExtractTemporalFieldData()
{
  this->Internals = new vtkExtractTemporalFieldData::vtkInternals();
  this->HandleCompositeDataBlocksIndividually = true;
}

//----------------------------------------------------------------------------
vtkExtractTemporalFieldData::~vtkExtractTemporalFieldData()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
int vtkExtractTemporalFieldData::GetNumberOfTimeSteps()
{
  return static_cast<int>(this->Internals->TimeSteps.size());
}

//----------------------------------------------------------------------------
void vtkExtractTemporalFieldData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "HandleCompositeDataBlocksIndividually: "
     << this->HandleCompositeDataBlocksIndividually << endl;
}

//----------------------------------------------------------------------------
int vtkExtractTemporalFieldData::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractTemporalFieldData::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);

  if (vtkCompositeDataSet::SafeDownCast(input) &&
    this->HandleCompositeDataBlocksIndividually)
  {
    if (vtkMultiBlockDataSet::GetData(outputVector, 0) == NULL)
    {
      vtkNew<vtkMultiBlockDataSet> mb;
      outputVector->GetInformationObject(0)->Set(
        vtkDataObject::DATA_OBJECT(), mb.Get());
    }
  }
  else if (vtkTable::GetData(outputVector, 0) == NULL)
  {
    vtkNew<vtkTable> table;
    outputVector->GetInformationObject(0)->Set(
      vtkDataObject::DATA_OBJECT(), table.Get());
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractTemporalFieldData::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    int size = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->Internals->TimeSteps.resize(size);
    if (size > 0)
    {
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->Internals->TimeSteps[0]);
    }
  }
  else
  {
    this->Internals->TimeSteps.clear();
  }

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
int vtkExtractTemporalFieldData::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (this->GetNumberOfTimeSteps() == 0)
  {
    vtkErrorMacro("No time steps in input data!");
    return 0;
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  if (vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    if (this->HandleCompositeDataBlocksIndividually)
    {
      vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
      assert(output);
      output->CopyStructure(cd);
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        if (vtkDataSet* inputDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
        {
          vtkNew<vtkTable> outputBlock;
          this->CopyDataToOutput(inputDS, outputBlock.Get());
          output->SetDataSet(iter, outputBlock.Get());
        }
      }
    }
    else
    {
      vtkTable *output = vtkTable::GetData(outputVector, 0);
      assert(output);
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        if (vtkDataSet* inputDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
        {
          if (this->CopyDataToOutput(inputDS, output))
          {
            break;
          }
        }
      }
    }
  }
  else if (vtkDataSet* input = vtkDataSet::SafeDownCast(inputDO))
  {
    vtkTable *output = vtkTable::GetData(outputVector, 0);
    this->CopyDataToOutput(input, output);
  }
  else
  {
    vtkErrorMacro("Incorrect input type.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkExtractTemporalFieldData::CopyDataToOutput(vtkDataSet *input, vtkTable *output)
{
  vtkDataSetAttributes *outRowData = output->GetRowData();
  vtkFieldData *ifd = input->GetFieldData();
  if (!ifd || !outRowData)
  {
    return false;
  }

  int numTimeSteps = this->GetNumberOfTimeSteps();
  assert(numTimeSteps > 0);
  for (vtkIdType j=0; j<ifd->GetNumberOfArrays(); j++)
  {
    vtkDataArray* inFieldArray = ifd->GetArray(j);
    if (inFieldArray &&
        inFieldArray->GetName() &&
        inFieldArray->GetNumberOfTuples() == numTimeSteps)
    {
      vtkDataArray *outArray = inFieldArray->NewInstance();
      outArray->ShallowCopy(inFieldArray);
      outRowData->AddArray(outArray);
      outArray->Delete();
    }
  }

  if (outRowData->GetNumberOfArrays() == 0)
  {
    return false;
  }

  // Add an array to hold the time at each step
  vtkNew<vtkDoubleArray> timeArray;
  timeArray->SetNumberOfComponents(1);
  timeArray->SetNumberOfTuples(numTimeSteps);
  if (ifd->GetArray("Time"))
  {
    timeArray->SetName("TimeData");
  }
  else
  {
    timeArray->SetName("Time");
  }
  std::copy(this->Internals->TimeSteps.begin(), this->Internals->TimeSteps.end(),
    timeArray->GetPointer(0));
  outRowData->AddArray(timeArray.Get());
  return true;
}
