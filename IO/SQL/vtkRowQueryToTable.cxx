/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRowQueryToTable.cxx

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
#include "vtkRowQueryToTable.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRowQuery.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTypeUInt64Array.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkRowQueryToTable);

vtkRowQueryToTable::vtkRowQueryToTable()
{
  this->SetNumberOfInputPorts(0);
  this->Query = NULL;
}

vtkRowQueryToTable::~vtkRowQueryToTable()
{
  if (this->Query)
    {
    this->Query->Delete();
    this->Query = NULL;
    }
}

void vtkRowQueryToTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Query: " << (this->Query ? "" : "NULL") << endl;
  if (this->Query)
    {
    this->Query->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkCxxSetObjectMacro(vtkRowQueryToTable, Query, vtkRowQuery);

unsigned long vtkRowQueryToTable::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  if (this->Query != NULL)
    {
    unsigned long time = this->Query->GetMTime();
    mTime = (time > mTime ? time : mTime);
    }
  return mTime;
}

int vtkRowQueryToTable::RequestData(
  vtkInformation*,
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  if (this->Query == NULL)
    {
    vtkErrorMacro("Query undefined.");
    return 0;
    }

  vtkTable* output = vtkTable::GetData(outputVector);

  // Set up the columns
  this->Query->Execute();

  // Check for query error
  if (this->Query->HasError())
    {
    vtkErrorMacro("Query Error: " << this->Query->GetLastErrorText());
    return 0;
    }
  int cols = this->Query->GetNumberOfFields();
  for (int c = 0; c < cols; c++)
    {
    vtkAbstractArray* arr;
    int type = this->Query->GetFieldType(c);

    // Take care of the special case of uint64
    // to ensure timepoints have specific array type
    if (type == VTK_TYPE_UINT64)
      {
      arr = vtkTypeUInt64Array::New();
      }
    else if (type == 0)
      {
      arr = vtkAbstractArray::CreateArray(VTK_DOUBLE);
      }
    else
      {
      arr = vtkAbstractArray::CreateArray(type);
      }

    // Make sure name doesn't clash with existing name.
    const char* name = this->Query->GetFieldName(c);
    if (output->GetColumnByName(name))
      {
      int i = 1;
      vtksys_ios::ostringstream oss;
      vtkStdString newName;
      do
        {
        oss.str("");
        oss << name << "_" << i;
        newName = oss.str();
        ++i;
        } while (output->GetColumnByName(newName));
      arr->SetName(newName);
      }
    else
      {
      arr->SetName(name);
      }

    // Add the column to the table.
    output->AddColumn(arr);
    arr->Delete();
    }

  // Fill the table
  int numRows = 0;
  float progressGuess = 0;
  vtkVariantArray* rowArray = vtkVariantArray::New();
  while (this->Query->NextRow(rowArray))
    {
    output->InsertNextRow(rowArray);

    // Update progress every 100 rows
    numRows++;
    if ((numRows%100)==0)
      {
      // 1% for every 100 rows, and then 'spin around'
      progressGuess = ((numRows/100)%100)*.01;
      this->UpdateProgress(progressGuess);
      }
    }
  rowArray->Delete();

  return 1;
}


