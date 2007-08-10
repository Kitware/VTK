/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTable.cxx

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

#include "vtkThresholdTable.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkThresholdTable, "1.2");
vtkStandardNewMacro(vtkThresholdTable);

vtkThresholdTable::vtkThresholdTable() : MinValue(0), MaxValue(VTK_INT_MAX)
{
}

vtkThresholdTable::~vtkThresholdTable()
{
}

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
    case ACCEPT_GREATER_THAN:
      os << "Accept greater than";
    case ACCEPT_BETWEEN:
      os << "Accept between";
    case ACCEPT_OUTSIDE:
      os << "Accept outside";
    default:
      os << "Undefined";
    }
  os << endl;
}

bool vtkVariantLessThan(vtkVariant a, vtkVariant b)
{
  return a.ToDouble() < b.ToDouble();
}

template <typename iterT>
void vtkThresholdTableThresholdRows(iterT* it, vtkTable* input, vtkTable* output, vtkVariant min, vtkVariant max, int mode)
{
  vtkIdType maxInd = it->GetNumberOfValues();
  for (vtkIdType i = 0; i < maxInd; i++)
    {
    bool accept = false;
    vtkVariant v(it->GetValue(i));
    if (mode == vtkThresholdTable::ACCEPT_LESS_THAN)
      {
      accept = vtkVariantLessThan(v, max);
      }
    else if (mode == vtkThresholdTable::ACCEPT_GREATER_THAN)
      {
      accept = vtkVariantLessThan(min, v);
      }
    else if (mode == vtkThresholdTable::ACCEPT_BETWEEN)
      {
      accept = (vtkVariantLessThan(min, v) && vtkVariantLessThan(v, max));
      }
    else if (mode == vtkThresholdTable::ACCEPT_OUTSIDE)
      {
      accept = (vtkVariantLessThan(v, min) || vtkVariantLessThan(max, v));
      }
    if (accept)
      {
      vtkVariantArray* row = input->GetRow(i);
      output->InsertNextRow(row);
      row->Delete();
      }
    }
}

int vtkThresholdTable::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, inputVector);
  if (arr == NULL)
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

  vtkArrayIterator* iter = arr->NewIterator();
  switch (arr->GetDataType())
    {
    vtkArrayIteratorTemplateMacro(
      vtkThresholdTableThresholdRows(static_cast<VTK_TT*>(iter), input, output, 
        this->MinValue, this->MaxValue, this->Mode));
    }
  iter->Delete();

  return 1;
}


