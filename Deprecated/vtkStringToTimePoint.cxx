/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringToTimePoint.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkStringToTimePoint.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimePointUtility.h"
#include "vtkTypeUInt64Array.h"

vtkStandardNewMacro(vtkStringToTimePoint);

vtkStringToTimePoint::vtkStringToTimePoint()
{
  this->OutputArrayName = 0;
}

vtkStringToTimePoint::~vtkStringToTimePoint()
{
  this->SetOutputArrayName(0);
}

void vtkStringToTimePoint::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputArrayName: "
    << (this->OutputArrayName ? this->OutputArrayName : "(none)") << endl;
}

int vtkStringToTimePoint::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and output objects
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);

  if (this->OutputArrayName == NULL)
  {
    vtkErrorMacro(<< "The output array name must be specified.");
    return 0;
  }

  // Get the input array
  vtkAbstractArray* inputArray = this->GetInputAbstractArrayToProcess(0, inputVector);
  vtkStringArray* stringArray = vtkArrayDownCast<vtkStringArray>(inputArray);
  if (stringArray == NULL)
  {
    vtkErrorMacro(<< "The input array must be a string array.");
    return 0;
  }

  vtkIdType numTuples = stringArray->GetNumberOfTuples();
  vtkIdType numComps = stringArray->GetNumberOfComponents();

  // Set up the output array
  vtkTypeUInt64Array* outputArray = vtkTypeUInt64Array::New();
  outputArray->SetNumberOfValues(numComps*numTuples);
  outputArray->SetNumberOfComponents(numComps);
  outputArray->SetName(this->OutputArrayName);

  // Convert the strings to time point values
  for (vtkIdType i = 0; i < numTuples*numComps; i++)
  {
    vtkStdString str = stringArray->GetValue(i);
    bool ok;
    vtkTypeUInt64 value = vtkTimePointUtility::ISO8601ToTimePoint(str, &ok);
    if (!ok)
    {
      vtkWarningMacro(<< "Invalid format at index " << i);
    }
    outputArray->SetValue(i, value);
  }

  // Add the array to the approprate field data.
  bool addedArray = false;
  for (vtkIdType i = 0; i < output->GetFieldData()->GetNumberOfArrays(); i++)
  {
    if (stringArray == output->GetFieldData()->GetAbstractArray(i))
    {
      output->GetFieldData()->AddArray(outputArray);
      addedArray = true;
    }
  }
  vtkDataSet* outputDataSet;
  if (!addedArray && (outputDataSet = vtkDataSet::SafeDownCast(output)))
  {
    for (vtkIdType i = 0; i < outputDataSet->GetPointData()->GetNumberOfArrays(); i++)
    {
      if (stringArray == outputDataSet->GetPointData()->GetAbstractArray(i))
      {
        outputDataSet->GetPointData()->AddArray(outputArray);
        addedArray = true;
      }
    }
    for (vtkIdType i = 0; i < outputDataSet->GetCellData()->GetNumberOfArrays(); i++)
    {
      if (stringArray == outputDataSet->GetCellData()->GetAbstractArray(i))
      {
        outputDataSet->GetCellData()->AddArray(outputArray);
        addedArray = true;
      }
    }
  }
  vtkGraph* outputGraph;
  if (!addedArray && (outputGraph = vtkGraph::SafeDownCast(output)))
  {
    for (vtkIdType i = 0; i < outputGraph->GetVertexData()->GetNumberOfArrays(); i++)
    {
      if (stringArray == outputGraph->GetVertexData()->GetAbstractArray(i))
      {
        outputGraph->GetVertexData()->AddArray(outputArray);
        addedArray = true;
      }
    }
    for (vtkIdType i = 0; i < outputGraph->GetEdgeData()->GetNumberOfArrays(); i++)
    {
      if (stringArray == outputGraph->GetEdgeData()->GetAbstractArray(i))
      {
        outputGraph->GetEdgeData()->AddArray(outputArray);
        addedArray = true;
      }
    }
  }
  vtkTable* outputTable;
  if (!addedArray && (outputTable = vtkTable::SafeDownCast(output)))
  {
    for (vtkIdType i = 0; i < outputTable->GetRowData()->GetNumberOfArrays(); i++)
    {
      if (stringArray == outputTable->GetRowData()->GetAbstractArray(i))
      {
        outputTable->GetRowData()->AddArray(outputArray);
        addedArray = true;
      }
    }
  }
  if (!addedArray)
  {
    vtkErrorMacro(<< "The input array was not found in the field, point, or cell data.");
    outputArray->Delete();
    return 0;
  }

  // Clean up
  outputArray->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkStringToTimePoint::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkStringToTimePoint::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}


