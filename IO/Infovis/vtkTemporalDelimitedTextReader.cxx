/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalDelimitedTextReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalDelimitedTextReader.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include <string>
#include <vector>

vtkStandardNewMacro(vtkTemporalDelimitedTextReader);

//----------------------------------------------------------------------------
vtkTemporalDelimitedTextReader::vtkTemporalDelimitedTextReader()
{
  this->DetectNumericColumnsOn();
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::SetTimeColumnName(const std::string name)
{
  if (this->TimeColumnName.compare(name) != 0)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting TimeColumnName to "
                  << name);
    this->TimeColumnName = name;
    this->InternalModified();
  }
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::SetTimeColumnId(const int idx)
{
  if (idx != this->TimeColumnId)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting TimeColumnId to" << idx);
    this->TimeColumnId = idx;
    this->InternalModified();
  }
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::SetRemoveTimeStepColumn(bool rts)
{
  if (rts != this->RemoveTimeStepColumn)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting RemoveTimeStepColumn to "
                  << rts);
    this->RemoveTimeStepColumn = rts;
    this->InternalModified();
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkTemporalDelimitedTextReader::GetMTime()
{
  return std::max(this->MTime, this->InternalMTime);
}

//----------------------------------------------------------------------------
int vtkTemporalDelimitedTextReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (strlen(this->FieldDelimiterCharacters) == 0)
  {
    // This reader does not give any output as long as the
    // FieldDelimiterCharacters is not set by the user as we need to parse the
    // input file in the RequestInformation to set the time range (which
    // requires the FieldDelimiterCharacters).
    return 1;
  }

  if (this->MTime > this->LastReadTime)
  {
    // fill the ReadTable with the actual input
    // only if modified has been called
    this->ReadTable->Initialize();
    this->ReadData(this->ReadTable);
    this->LastReadTime = this->GetMTime();
  }

  if (!this->EnforceColumnName())
  {
    // Bad user input
    return 0;
  }

  if (this->InternalColumnName.empty())
  {
    // Output the whole input data, not temporal
    return this->Superclass::RequestInformation(request, inputVector, outputVector);
  }

  // Store each line id in the TimeMap, at the given time step
  vtkDataArray* inputColumn =
    vtkDataArray::SafeDownCast(this->ReadTable->GetColumnByName(this->InternalColumnName.c_str()));
  const vtkIdType nbRows = inputColumn->GetNumberOfTuples();
  this->TimeMap.clear();
  for (vtkIdType r = 0; r < nbRows; r++)
  {
    double v = inputColumn->GetTuple1(r);
    if (vtkMath::IsNan(v))
    {
      vtkWarningMacro("The time step indicator column has a nan value at line: " << r);
    }
    else
    {
      this->TimeMap[v].emplace_back(r);
    }
  }

  // Get the time range (first and last key of the TimeMap)
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  double timeRange[2] = { this->TimeMap.cbegin()->first, this->TimeMap.crbegin()->first };
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  // Get the discrete time steps from the TimeMap keys
  std::vector<double> timeStepsArray;
  timeStepsArray.reserve(this->TimeMap.size());
  for (auto mapEl : this->TimeMap)
  {
    timeStepsArray.emplace_back(mapEl.first);
  }
  const int nbTimeSteps = static_cast<int>(timeStepsArray.size());
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeStepsArray.data(), nbTimeSteps);

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkTemporalDelimitedTextReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (strlen(this->FieldDelimiterCharacters) == 0)
  {
    vtkErrorMacro(
      "You need to set the FieldDelimiterCharacters before requesting data with this reader");
    return 0;
  }

  if (!this->EnforceColumnName())
  {
    vtkErrorMacro("Invalid user input for the Time step indicator.");
    return 0;
  }

  if (this->InternalColumnName.empty())
  {
    // Shallow copy the internal reader's output as the time column
    // has not been set
    vtkTable* outputTable = vtkTable::GetData(outputVector, 0);
    outputTable->ShallowCopy(this->ReadTable);
    this->UpdateProgress(1);
    return 1;
  }

  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): process column "
                << this->InternalColumnName);

  // Retrieve the current time step
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  double updateTimeStep = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    updateTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  this->UpdateProgress(0.5);

  if (this->TimeMap.size())
  {
    // Generate an empty output with the same structure
    vtkTable* outputTable = vtkTable::GetData(outputVector, 0);
    vtkDataSetAttributes* outAttributes = outputTable->GetRowData();
    auto timeStepDataIt = this->TimeMap.lower_bound(updateTimeStep);
    if (timeStepDataIt == this->TimeMap.end())
    {
      // update time is too high, take last element.
      --timeStepDataIt;
    }
    vtkIdType nbRow = static_cast<vtkIdType>(timeStepDataIt->second.size());
    outAttributes->CopyAllocate(this->ReadTable->GetRowData(), nbRow);
    for (auto r : timeStepDataIt->second)
    {
      outputTable->InsertNextRow(this->ReadTable->GetRow(r));
    }

    // Get rid of the time column in the result
    if (this->RemoveTimeStepColumn)
    {
      outputTable->RemoveColumnByName(this->InternalColumnName.c_str());
    }
  }

  this->UpdateProgress(1);

  return 1;
}

//----------------------------------------------------------------------------
bool vtkTemporalDelimitedTextReader::EnforceColumnName()
{
  this->InternalColumnName = "";

  if (this->TimeColumnName.empty() && this->TimeColumnId == -1)
  {
    // No user specified input, the reader simply output the whole content of
    // the input file.
    return 1;
  }

  // Set TimeColumnName from user input
  if (this->TimeColumnId != -1)
  {
    // use id to retrieve column
    if (this->TimeColumnId >= 0 && this->TimeColumnId < this->ReadTable->GetNumberOfColumns())
    {
      this->InternalColumnName = this->ReadTable->GetColumnName(this->TimeColumnId);
    }
    else
    {
      vtkErrorMacro("Invalid column id: " << this->TimeColumnId);
      return 0;
    }
  }
  else if (!this->TimeColumnName.empty())
  {
    // use name to retrieve column
    vtkAbstractArray* arr = this->ReadTable->GetColumnByName(this->TimeColumnName.c_str());
    if (arr == nullptr)
    {
      vtkErrorMacro("Invalid column name: " << this->TimeColumnName);
      return 0;
    }
    else
    {
      // check valid array
      vtkDataArray* numArr = vtkDataArray::SafeDownCast(arr);
      if (numArr == nullptr)
      {
        vtkErrorMacro("Not a numerical column: " << this->TimeColumnName);
        return 0;
      }
      else if (numArr->GetNumberOfComponents() != 1)
      {
        vtkErrorMacro("The time column must have only one component: " << this->TimeColumnName);
        return 0;
      }
    }
    this->InternalColumnName = this->TimeColumnName;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::InternalModified()
{
  this->InternalMTime.Modified();
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "TimeColumnName: " << this->TimeColumnName << endl;
  os << "TimeColumnId: " << this->TimeColumnId << endl;
  os << "RemoveTimeStepColumn: " << this->RemoveTimeStepColumn << endl;
}
