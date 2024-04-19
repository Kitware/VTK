// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkThresholdTable.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkThresholdTable);

//------------------------------------------------------------------------------
vtkThresholdTable::vtkThresholdTable()
  : MinValue(0)
  , MaxValue(VTK_INT_MAX)
  , Mode(0)
{
}

//------------------------------------------------------------------------------
vtkThresholdTable::~vtkThresholdTable() = default;

//------------------------------------------------------------------------------
void vtkThresholdTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MinValue: " << this->MinValue.ToString() << endl;
  os << indent << "MaxValue: " << this->MaxValue.ToString() << endl;
  os << indent << "Mode: ";
  switch (this->Mode)
  {
    case ACCEPT_LESS_THAN:
      os << "Accept less than";
      break;
    case ACCEPT_GREATER_THAN:
      os << "Accept greater than";
      break;
    case ACCEPT_BETWEEN:
      os << "Accept between";
      break;
    case ACCEPT_OUTSIDE:
      os << "Accept outside";
      break;
    default:
      os << "Undefined";
      break;
  }
  os << endl;
}

//------------------------------------------------------------------------------
static bool vtkThresholdTableCompare(vtkVariant a, vtkVariant b)
{
  return a.ToDouble() <= b.ToDouble();
}

//------------------------------------------------------------------------------
bool vtkThresholdTable::IsValueAcceptable(vtkVariant value)
{
  bool accept = false;
  switch (this->Mode)
  {
    case vtkThresholdTable::ACCEPT_LESS_THAN:
    {
      accept = vtkThresholdTableCompare(value, this->MaxValue);
    }
    break;
    case vtkThresholdTable::ACCEPT_GREATER_THAN:
    {
      accept = vtkThresholdTableCompare(this->MinValue, value);
      break;
    }
    case vtkThresholdTable::ACCEPT_BETWEEN:
    {
      accept = (vtkThresholdTableCompare(this->MinValue, value) &&
        vtkThresholdTableCompare(value, this->MaxValue));
      break;
    }
    case vtkThresholdTable::ACCEPT_OUTSIDE:
    {
      accept = (vtkThresholdTableCompare(value, this->MinValue) ||
        vtkThresholdTableCompare(this->MaxValue, value));
      break;
    }
    default:
      break;
  }

  return accept;
}

//------------------------------------------------------------------------------
void vtkThresholdTable::ThresholdBetween(vtkVariant lower, vtkVariant upper)
{
  if (this->MinValue != lower || this->MaxValue != upper ||
    this->Mode != vtkThresholdTable::ACCEPT_BETWEEN)
  {
    this->MinValue = lower;
    this->MaxValue = upper;
    this->Mode = vtkThresholdTable::ACCEPT_BETWEEN;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkThresholdTable::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, inputVector);
  if (arr == nullptr)
  {
    vtkErrorMacro("An input array must be specified.");
    return 0;
  }

  vtkTable* input = vtkTable::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector);

  for (int n = 0; n < input->GetNumberOfColumns(); n++)
  {
    vtkAbstractArray* col = input->GetColumn(n);
    vtkAbstractArray* ncol = vtkAbstractArray::CreateArray(col->GetDataType());
    ncol->SetName(col->GetName());
    ncol->SetNumberOfComponents(col->GetNumberOfComponents());
    output->AddColumn(ncol);
    ncol->Delete();
  }

  vtkIdType columnIndex = input->GetColumnIndex(arr->GetName());
  for (vtkIdType rowIndex = 0; rowIndex < arr->GetNumberOfTuples(); rowIndex++)
  {
    auto value = input->GetValue(rowIndex, columnIndex);
    if (this->IsValueAcceptable(value))
    {
      vtkVariantArray* row = input->GetRow(rowIndex);
      output->InsertNextRow(row);
    }
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
