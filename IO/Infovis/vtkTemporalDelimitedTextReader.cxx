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
#include "vtkDelimitedTextReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <algorithm>

vtkStandardNewMacro(vtkTemporalDelimitedTextReader);

//----------------------------------------------------------------------------
vtkTemporalDelimitedTextReader::vtkTemporalDelimitedTextReader()
{
  this->SetNumberOfInputPorts(0);
  this->ColumnNames->SetNumberOfComponents(1);
  this->InternalReader->DetectNumericColumnsOn();
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::SetTimeColumnName(const std::string name)
{
  if (this->TimeColumnName.compare(name) != 0)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting TimeColumnName to " << name
                  << " and TimeColumnId to -1");
    this->TimeColumnName = name;
    this->TimeColumnId = -1;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::SetTimeColumnId(const int idx)
{
  if (idx != this->TimeColumnId)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this
                  << "): setting TimeColumnName to '' and TimeColumnId to" << idx);
    this->TimeColumnName = "";
    this->TimeColumnId = idx;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkTemporalDelimitedTextReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (this->FieldDelimiterCharacters.empty())
  {
    // This reader does not give any output as long as the
    // FieldDelimiterCharacters is not set by the user as we need to parse the
    // input file in the RequestInformation to set the time range (which
    // requires the FieldDelimiterCharacters).
    return 1;
  }

  this->ReadInputFile();

  vtkTable* inputTable = vtkTable::SafeDownCast(this->InternalReader->GetOutputDataObject(0));
  if (!this->EnforceColumnName(inputTable))
  {
    // The filter simply read the CSV, the output will not be temporal in this case
    return 1;
  }

  // Store each line id in the TimeMap, at the given time step
  vtkAbstractArray* inputColumnRead = inputTable->GetColumnByName(this->TimeColumnName.c_str());
  vtkDataArray* inputColumn = vtkDataArray::SafeDownCast(inputColumnRead);
  const vtkIdType nbRows = inputColumn->GetNumberOfTuples();
  this->TimeMap.clear();
  for (vtkIdType r = 0; r < nbRows; r++)
  {
    double v = inputColumn->GetTuple1(r);
    if (std::isnan(v))
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
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeStepsArray.data(), timeStepsArray.size());
  return 1;
}

//----------------------------------------------------------------------------
int vtkTemporalDelimitedTextReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (this->FieldDelimiterCharacters.empty())
  {
    vtkErrorMacro(
      "You need to set the FieldDelimiterCharacters before requesting data with this reader");
    return 0;
  }

  vtkTable* inputTable = vtkTable::SafeDownCast(this->InternalReader->GetOutputDataObject(0));
  if (inputTable == nullptr)
  {
    vtkErrorMacro("Unable to parse the input file.");
    return 0;
  }

  if (!this->EnforceColumnName(inputTable))
  {
    // Shallow copy the internal reader's output as the time column
    // is either not set or invalid.
    vtkTable* outputTable = vtkTable::GetData(outputVector, 0);
    outputTable->ShallowCopy(this->InternalReader->GetOutputDataObject(0));
    this->UpdateProgress(1);
    return 1;
  }

  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): process column "
                << this->TimeColumnName);

  // Retreive the current time step
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  double updateTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  this->UpdateProgress(0.5);

  // Generate an empty output with the same structure
  vtkTable* outputTable = vtkTable::GetData(outputVector, 0);
  vtkDataSetAttributes* outAttributes = outputTable->GetRowData();
  outAttributes->CopyAllocate(inputTable->GetRowData(), this->TimeMap[updateTimeStep].size());
  for (auto r : this->TimeMap[updateTimeStep])
  {
    outputTable->InsertNextRow(inputTable->GetRow(r));
  }

  // Get rid of the time column in the result
  if (this->RemoveTimeStepColumn)
  {
    outputTable->RemoveColumnByName(this->TimeColumnName.c_str());
  }

  this->UpdateProgress(1);

  return 1;
}

//----------------------------------------------------------------------------
bool vtkTemporalDelimitedTextReader::EnforceColumnName(vtkTable* inputTable)
{
  if (this->TimeColumnName.empty() && this->TimeColumnId == -1)
  {
    // No user specified input, the reader simply output the CSV.
    return 0;
  }

  // Set TimeColumnName from user input
  if (this->TimeColumnId != -1)
  {
    // use id to retrieve column
    if (this->TimeColumnId >= 0 && this->TimeColumnId < inputTable->GetNumberOfColumns())
    {
      this->TimeColumnName = inputTable->GetColumnName(this->TimeColumnId);
    }
    else
    {
      vtkErrorMacro("Invalid column id: " << this->TimeColumnId);
    }
  }
  if (!this->TimeColumnName.empty())
  {
    // use name to retrieve column
    vtkAbstractArray* arr = inputTable->GetColumnByName(this->TimeColumnName.c_str());
    if (arr == nullptr)
    {
      vtkWarningMacro("Invalid column name: " << this->TimeColumnName);
      this->TimeColumnName = "";
    }
    else
    {
      // check valid array
      vtkDataArray* numArr = vtkDataArray::SafeDownCast(arr);
      if (numArr == nullptr)
      {
        vtkWarningMacro("Not a numerical column: " << this->TimeColumnName);
        this->TimeColumnName = "";
      }
      else if (numArr->GetNumberOfComponents() != 1)
      {
        vtkErrorMacro("The time column must have only one component: " << this->TimeColumnName);
        this->TimeColumnName = "";
      }
    }
  }

  return !this->TimeColumnName.empty();
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::ReadInputFile()
{
  // We need the input data set to be available here
  // as we will read the column
  this->InternalReader->SetFileName(this->FileName.c_str());
  this->InternalReader->SetFieldDelimiterCharacters(this->FieldDelimiterCharacters.c_str());
  this->InternalReader->SetHaveHeaders(this->HaveHeaders);
  this->InternalReader->SetMergeConsecutiveDelimiters(this->MergeConsecutiveDelimiters);
  this->InternalReader->SetAddTabFieldDelimiter(this->AddTabFieldDelimiter);
  this->InternalReader->Update();
}

//----------------------------------------------------------------------------
void vtkTemporalDelimitedTextReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Internal vtkDelimitedTextReader: ";
  this->InternalReader->PrintSelf(os, indent.GetNextIndent());
  os << "TimeColumnName: " << this->TimeColumnName << endl;
  os << "TimeColumnId: " << this->TimeColumnId << endl;
  os << "RemoveTimeStepColumn: " << this->RemoveTimeStepColumn << endl;
}
