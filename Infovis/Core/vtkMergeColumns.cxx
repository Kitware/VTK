// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkMergeColumns.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMergeColumns);

vtkMergeColumns::vtkMergeColumns()
{
  this->MergedColumnName = nullptr;
}

vtkMergeColumns::~vtkMergeColumns()
{
  this->SetMergedColumnName(nullptr);
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
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get input tables
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkTable* input = vtkTable::SafeDownCast(inputInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get output table
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(outputInfo->Get(vtkDataObject::DATA_OBJECT()));

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
      vtkStringArray* col1Str = vtkArrayDownCast<vtkStringArray>(col1);
      vtkStringArray* col2Str = vtkArrayDownCast<vtkStringArray>(col2);
      vtkStringArray* mergedStr = vtkArrayDownCast<vtkStringArray>(merged);
      for (vtkIdType i = 0; i < merged->GetNumberOfTuples(); i++)
      {
        std::string combined = col1Str->GetValue(i);
        if (col1Str->GetValue(i).length() > 0 && col2Str->GetValue(i).length() > 0)
        {
          combined += " ";
        }
        combined += col2Str->GetValue(i);
        mergedStr->SetValue(i, combined);
      }
      break;
    }

      vtkTemplateMacro(vtkMergeColumnsCombine(static_cast<VTK_TT*>(col1->GetVoidPointer(0)),
        static_cast<VTK_TT*>(col2->GetVoidPointer(0)),
        static_cast<VTK_TT*>(merged->GetVoidPointer(0)), merged->GetNumberOfTuples()));
  }

  output->AddColumn(merged);
  merged->Delete();

  return 1;
}

void vtkMergeColumns::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "MergedColumnName: " << (this->MergedColumnName ? this->MergedColumnName : "(null)")
     << endl;
}
VTK_ABI_NAMESPACE_END
