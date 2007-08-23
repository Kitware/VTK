/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLTableReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkSQLTableReader.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSQLQuery.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTypeUInt64Array.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkSQLTableReader, "1.1");
vtkStandardNewMacro(vtkSQLTableReader);

vtkSQLTableReader::vtkSQLTableReader()
{
  this->SetNumberOfInputPorts(0);
  this->Query = NULL;
}

vtkSQLTableReader::~vtkSQLTableReader()
{
  if (this->Query)
    {
    this->Query->Delete();
    this->Query = NULL;
    }
}

void vtkSQLTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Query: " << (this->Query ? "" : "NULL") << endl;
  if (this->Query)
    {
    this->Query->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkCxxSetObjectMacro(vtkSQLTableReader, Query, vtkSQLQuery);

unsigned long vtkSQLTableReader::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  if (this->Query != NULL)
    {
    unsigned long time = this->Query->GetMTime();
    mTime = (time > mTime ? time : mTime);
    }
  return mTime;
}

int vtkSQLTableReader::RequestData(
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
    else
      {
      arr = vtkAbstractArray::CreateArray(type);
      }

    arr->SetName(this->Query->GetFieldName(c));
    output->AddColumn(arr);
    arr->Delete();
    }

  // Fill the table
  vtkVariantArray* rowArray = vtkVariantArray::New();
  while (this->Query->NextRow(rowArray))
    {
    output->InsertNextRow(rowArray);
    }
  rowArray->Delete();
 
  return 1;
}


