/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeColumns.cxx

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

#include "vtkMergeColumns.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkUnicodeStringArray.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkMergeColumns);

vtkMergeColumns::vtkMergeColumns()
{
  this->MergedColumnName = 0;
}

vtkMergeColumns::~vtkMergeColumns()
{
  this->SetMergedColumnName(0);
}

template <typename T>
void vtkMergeColumnsCombine(T* col1, T* col2, T* merged, vtkIdType size)
{
  for (vtkIdType i = 0; i < size; i++)
    {
    merged[i] = col1[i] + col2[i];
    }
}

int vtkMergeColumns::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get input tables
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkTable* input = vtkTable::SafeDownCast(
    inputInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get output table
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(
    outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  vtkAbstractArray* col1 = this->GetInputAbstractArrayToProcess(0, 0, inputVector);
  vtkAbstractArray* col2 = this->GetInputAbstractArrayToProcess(1, 0, inputVector);
  if (!col1)
    {
    vtkErrorMacro("Could not find first column to process.");
    return 0;
    }
  if (!col2)
    {
    vtkErrorMacro("Could not find second column to process.");
    return 0;
    }
  if (col1->GetDataType() != col2->GetDataType())
    {
    vtkErrorMacro("The columns must be of the same type.");
    return 0;
    }

  output->RemoveColumnByName(col1->GetName());
  output->RemoveColumnByName(col2->GetName());

  vtkAbstractArray* merged = vtkAbstractArray::CreateArray(col1->GetDataType());
  merged->SetName(this->MergedColumnName);
  merged->SetNumberOfTuples(col1->GetNumberOfTuples());

  switch (merged->GetDataType())
    {
    case VTK_STRING:
      {
      vtkStringArray* col1Str = vtkStringArray::SafeDownCast(col1);
      vtkStringArray* col2Str = vtkStringArray::SafeDownCast(col2);
      vtkStringArray* mergedStr = vtkStringArray::SafeDownCast(merged);
      for (vtkIdType i = 0; i < merged->GetNumberOfTuples(); i++)
        {
        vtkStdString combined = col1Str->GetValue(i);
        if (col1Str->GetValue(i).length() > 0 &&
            col2Str->GetValue(i).length() > 0)
          {
          combined += " ";
          }
        combined += col2Str->GetValue(i);
        mergedStr->SetValue(i, combined);
        }
      break;
      }
    case VTK_UNICODE_STRING:
      {
      vtkUnicodeStringArray* col1Str = vtkUnicodeStringArray::SafeDownCast(col1);
      vtkUnicodeStringArray* col2Str = vtkUnicodeStringArray::SafeDownCast(col2);
      vtkUnicodeStringArray* mergedStr = vtkUnicodeStringArray::SafeDownCast(merged);
      for (vtkIdType i = 0; i < merged->GetNumberOfTuples(); i++)
        {
        vtkUnicodeString combined = col1Str->GetValue(i);
        if (!col1Str->GetValue(i).empty() &&
            !col2Str->GetValue(i).empty())
          {
          combined += vtkUnicodeString::from_utf8(" ");
          }
        combined += col2Str->GetValue(i);
        mergedStr->SetValue(i, combined);
        }
      break;
      }
    vtkTemplateMacro(vtkMergeColumnsCombine(
      static_cast<VTK_TT*>(col1->GetVoidPointer(0)),
      static_cast<VTK_TT*>(col2->GetVoidPointer(0)),
      static_cast<VTK_TT*>(merged->GetVoidPointer(0)),
      merged->GetNumberOfTuples()));
    }

  output->AddColumn(merged);
  merged->Delete();

  return 1;
}

void vtkMergeColumns::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MergedColumnName: "
     << (this->MergedColumnName ? this->MergedColumnName : "(null)") << endl;
}
