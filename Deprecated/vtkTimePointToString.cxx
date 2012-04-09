/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimePointToString.cxx

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

#include "vtkTimePointToString.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkUnsignedLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimePointUtility.h"
#include "vtkTypeUInt64Array.h"

vtkStandardNewMacro(vtkTimePointToString);

vtkTimePointToString::vtkTimePointToString()
{
  this->ISO8601Format = 0;
  this->OutputArrayName = 0;
}

vtkTimePointToString::~vtkTimePointToString()
{
  this->SetOutputArrayName(0);
}

void vtkTimePointToString::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ISO8601Format: " << this->ISO8601Format << endl;
  os << indent << "OutputArrayName: "
    << (this->OutputArrayName ? this->OutputArrayName : "(none)") << endl;
}

#define HOURS_IN_DAY 24
#define MINUTES_IN_HOUR 60
#define MINUTES_IN_DAY 1440
#define SECONDS_IN_MINUTE 60
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_DAY 86400
#define MSEC_IN_SECOND 1000
#define MSEC_IN_MINUTE 60000
#define MSEC_IN_HOUR 3600000
#define MSEC_IN_DAY 86400000

void GetTimeFromMilliseconds(int time, int *hour, int *minute, int *second, int *msec)
{
  *hour = time / MSEC_IN_HOUR;
  *minute = (time % MSEC_IN_HOUR) / MSEC_IN_MINUTE;
  *second = (time % MSEC_IN_MINUTE) / MSEC_IN_SECOND;
  *msec = time % MSEC_IN_SECOND;
}

void GetDateFromJulianDay(int julianDay, int *year, int *month, int *day)
{
    int y, m, d;

    if (julianDay >= 2299161) {
        // Gregorian calendar starting from October 15, 1582
        // This algorithm is from Henry F. Fliegel and Thomas C. Van Flandern
        vtkTypeUInt64 ell, n, i, j;
        ell = vtkTypeUInt64(julianDay) + 68569;
        n = (4 * ell) / 146097;
        ell = ell - (146097 * n + 3) / 4;
        i = (4000 * (ell + 1)) / 1461001;
        ell = ell - (1461 * i) / 4 + 31;
        j = (80 * ell) / 2447;
        d = ell - (2447 * j) / 80;
        ell = j / 11;
        m = j + 2 - (12 * ell);
        y = 100 * (n - 49) + i + ell;
    } else {
        // Julian calendar until October 4, 1582
        // Algorithm from Frequently Asked Questions about Calendars by Claus Toendering
        julianDay += 32082;
        int dd = (4 * julianDay + 3) / 1461;
        int ee = julianDay - (1461 * dd) / 4;
        int mm = ((5 * ee) + 2) / 153;
        d = ee - (153 * mm + 2) / 5 + 1;
        m = mm + 3 - 12 * (mm / 10);
        y = dd - 4800 + (mm / 10);
        if (y <= 0)
            --y;
    }
    *year = y;
    *month = m;
    *day = d;
}

int vtkTimePointToString::RequestData(
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

  // Get the input array
  vtkTypeUInt64Array* inputArray = vtkTypeUInt64Array::SafeDownCast(
    this->GetInputAbstractArrayToProcess(0, inputVector));

  if (inputArray == NULL)
    {
    vtkErrorMacro(<< "The input array must be of type vtkTypeUInt64Array.");
    return 0;
    }
  if (this->OutputArrayName == NULL)
    {
    vtkErrorMacro(<< "The output array name must be specified.");
    return 0;
    }

  // Initialize the string array
  vtkStringArray* stringArray = vtkStringArray::New();
  vtkIdType numTuples;
  vtkIdType numComps;
  vtkStdString oldName;
  numTuples = inputArray->GetNumberOfTuples();
  numComps = inputArray->GetNumberOfComponents();
  stringArray->SetNumberOfValues(numComps*numTuples);
  stringArray->SetNumberOfComponents(numComps);
  stringArray->SetName(this->OutputArrayName);

  // Convert the time points to strings
  for (vtkIdType i = 0; i < numTuples*numComps; i++)
    {
    vtkTypeUInt64 timePoint = inputArray->GetValue(i);
    const char* s = vtkTimePointUtility::TimePointToISO8601(timePoint, this->ISO8601Format);
    vtkStdString str = s;
    stringArray->SetValue(i, str);
    delete[] s;
    }

  // Add the array to the approprate field data (i.e. the same field data as the input).
  bool addedArray = false;
  for (vtkIdType i = 0; i < output->GetFieldData()->GetNumberOfArrays(); i++)
    {
    if (inputArray == output->GetFieldData()->GetAbstractArray(i))
      {
      output->GetFieldData()->AddArray(stringArray);
      addedArray = true;
      }
    }
  vtkDataSet* outputDataSet;
  if (!addedArray && (outputDataSet = vtkDataSet::SafeDownCast(output)))
    {
    for (vtkIdType i = 0; i < outputDataSet->GetPointData()->GetNumberOfArrays(); i++)
      {
      if (inputArray == outputDataSet->GetPointData()->GetAbstractArray(i))
        {
        outputDataSet->GetPointData()->AddArray(stringArray);
        addedArray = true;
        }
      }
    for (vtkIdType i = 0; i < outputDataSet->GetCellData()->GetNumberOfArrays(); i++)
      {
      if (inputArray == outputDataSet->GetCellData()->GetAbstractArray(i))
        {
        outputDataSet->GetCellData()->AddArray(stringArray);
        addedArray = true;
        }
      }
    }
  vtkGraph* outputGraph;
  if (!addedArray && (outputGraph = vtkGraph::SafeDownCast(output)))
    {
    for (vtkIdType i = 0; i < outputGraph->GetVertexData()->GetNumberOfArrays(); i++)
      {
      if (inputArray == outputGraph->GetVertexData()->GetAbstractArray(i))
        {
        outputGraph->GetVertexData()->AddArray(stringArray);
        addedArray = true;
        }
      }
    for (vtkIdType i = 0; i < outputGraph->GetEdgeData()->GetNumberOfArrays(); i++)
      {
      if (inputArray == outputGraph->GetEdgeData()->GetAbstractArray(i))
        {
        outputGraph->GetEdgeData()->AddArray(stringArray);
        addedArray = true;
        }
      }
    }
  vtkTable* outputTable;
  if (!addedArray && (outputTable = vtkTable::SafeDownCast(output)))
    {
    for (vtkIdType i = 0; i < outputTable->GetRowData()->GetNumberOfArrays(); i++)
      {
      if (inputArray == outputTable->GetRowData()->GetAbstractArray(i))
        {
        outputTable->GetRowData()->AddArray(stringArray);
        addedArray = true;
        }
      }
    }
  if (!addedArray)
    {
    vtkErrorMacro(<< "The input array was not found in the field, point, or cell data.");
    stringArray->Delete();
    return 0;
    }

  // Clean up
  stringArray->Delete();
  return 1;
}

//----------------------------------------------------------------------------
int vtkTimePointToString::ProcessRequest(
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
int vtkTimePointToString::RequestDataObject(
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


