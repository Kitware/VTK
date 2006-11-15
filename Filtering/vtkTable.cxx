/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayIteratorIncludes.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include <vtkFieldData.h>
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>
#include <vtkDataArray.h>
#include <vtkAbstractArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

//
// Standard functions
//

vtkCxxRevisionMacro(vtkTable, "1.3");
vtkStandardNewMacro(vtkTable);

//----------------------------------------------------------------------------

vtkTable::vtkTable()
{
  this->Rows = 0;
}

//----------------------------------------------------------------------------

void vtkTable::PrintSelf(ostream &os, vtkIndent indent)
{
  vtkDataObject::PrintSelf(os, indent);
  os << indent << "Number Of Rows: " << this->Rows << endl;
}

void vtkTable::Initialize()
{
  this->Superclass::Initialize();
  this->Rows = 0;
}

//
// Row functions
//

//----------------------------------------------------------------------------

vtkIdType vtkTable::GetNumberOfRows()
{
  return this->Rows;
}

//----------------------------------------------------------------------------

vtkVariantArray* vtkTable::GetRow(vtkIdType row)
{
  vtkVariantArray* varr = vtkVariantArray::New();
  for (int i = 0; i < this->FieldData->GetNumberOfArrays(); i++)
    {
    varr->InsertNextValue(this->GetValue(row, i));
    }
  return varr;
}

//----------------------------------------------------------------------------

vtkIdType vtkTable::InsertNextBlankRow()
{
  int n = this->FieldData->GetNumberOfArrays();
  for (int i = 0; i < n; i++)
    {
    vtkAbstractArray* arr = this->FieldData->GetAbstractArray(i);
    int comps = arr->GetNumberOfComponents();
    if (arr->IsA("vtkDataArray"))
      {
      vtkDataArray* data = vtkDataArray::SafeDownCast(arr);
      double* tuple = new double[comps];
      for (int j = 0; j < comps; j++)
        {
        tuple[j] = 0;
        }
      data->InsertNextTuple(tuple);
      delete[] tuple;
      }
    else if (arr->IsA("vtkStringArray"))
      {
      vtkStringArray* data = vtkStringArray::SafeDownCast(arr);
      for (int j = 0; j < comps; j++)
        {
        data->InsertNextValue(vtkStdString(""));
        }
      }
    else if (arr->IsA("vtkVariantArray"))
      {
      vtkVariantArray* data = vtkVariantArray::SafeDownCast(arr);
      for (int j = 0; j < comps; j++)
        {
        data->InsertNextValue(vtkVariant());
        }
      }
    }
  this->Rows++;
  return this->Rows - 1;
}

//----------------------------------------------------------------------------

vtkIdType vtkTable::InsertNextRow(vtkVariantArray* values)
{
  vtkIdType row = this->InsertNextBlankRow();
  for (int i = 0; i < this->GetNumberOfColumns(); i++)
    {
    this->SetValue(row, i, values->GetValue(i));
    }
  return row;
}

//----------------------------------------------------------------------------

void vtkTable::RemoveRow(vtkIdType row)
{
  int n = this->FieldData->GetNumberOfArrays();
  for (int i = 0; i < n; i++)
    {
    vtkAbstractArray* arr = this->FieldData->GetAbstractArray(i);
    int comps = arr->GetNumberOfComponents();
    if (arr->IsA("vtkDataArray"))
      {
      vtkDataArray* data = vtkDataArray::SafeDownCast(arr);
      data->RemoveTuple(row);
      }
    else if (arr->IsA("vtkStringArray"))
      {
      // Manually move all elements past the index back one place.
      vtkStringArray* data = vtkStringArray::SafeDownCast(arr);
      for (int j = comps*row; j < comps*data->GetNumberOfTuples() - 1; j++)
        {
        data->SetValue(j, data->GetValue(j+1));
        }
      data->Resize(data->GetNumberOfTuples() - 1);
      }
    else if (arr->IsA("vtkVariantArray"))
      {
      // Manually move all elements past the index back one place.
      vtkVariantArray* data = vtkVariantArray::SafeDownCast(arr);
      for (int j = comps*row; j < comps*data->GetNumberOfTuples() - 1; j++)
        {
        data->SetValue(j, data->GetValue(j+1));
        }
      data->Resize(data->GetNumberOfTuples() - 1);
      }
    }
  this->Rows--;
}

//
// Column functions
//

//----------------------------------------------------------------------------

vtkIdType vtkTable::GetNumberOfColumns()
{
  return this->FieldData->GetNumberOfArrays();
}

//----------------------------------------------------------------------------

void vtkTable::AddColumn(vtkAbstractArray* arr)
{
  if (this->FieldData->GetNumberOfArrays() == 0)
    {
    this->FieldData->AddArray(arr);
    this->Rows = arr->GetNumberOfTuples();
    }
  else
    {
    if (arr->GetNumberOfTuples() != this->Rows)
      {
      vtkErrorMacro(<< "Column \"" << arr->GetName() << "\" must have " 
        << this->Rows << " rows, but has " << arr->GetNumberOfTuples() << ".");
      return;
      }
    this->FieldData->AddArray(arr);
    }
}

//----------------------------------------------------------------------------

void vtkTable::RemoveColumnByName(const char* name)
{
  this->FieldData->RemoveArray(name);
}

//----------------------------------------------------------------------------

void vtkTable::RemoveColumn(vtkIdType col)
{
  int column = static_cast<int>(col);
  this->FieldData->RemoveArray(this->FieldData->GetArrayName(column));
}

//----------------------------------------------------------------------------

const char* vtkTable::GetColumnName(vtkIdType col)
{
  int column = static_cast<int>(col);
  return this->FieldData->GetArrayName(column);
}

//----------------------------------------------------------------------------

vtkAbstractArray* vtkTable::GetColumnByName(const char* name)
{
  return this->FieldData->GetAbstractArray(name);
}

//----------------------------------------------------------------------------

vtkAbstractArray* vtkTable::GetColumn(vtkIdType col)
{
  int column = static_cast<int>(col);
  return this->FieldData->GetAbstractArray(column);
}

//
// Table single entry functions
//

//----------------------------------------------------------------------------

void vtkTable::SetValue(vtkIdType row, vtkIdType col, vtkVariant value)
{
  int column = static_cast<int>(col);
  this->SetValueByName(row, this->GetColumnName(column), value);
}

//----------------------------------------------------------------------------

void vtkTable::SetValueByName(vtkIdType row, const char* col, vtkVariant value)
{
  vtkAbstractArray* arr = this->GetColumnByName(col);
  int comps = arr->GetNumberOfComponents();
  if (arr->IsA("vtkDataArray"))
    {
    vtkDataArray* data = vtkDataArray::SafeDownCast(arr);
    if (comps == 1)
      {
      data->SetTuple1(row, value.ToDouble());
      }
    else
      {
      if (value.IsArray() && value.ToArray()->IsA("vtkDataArray") && value.ToArray()->GetNumberOfComponents() == comps)
        {
        data->SetTuple(row, vtkDataArray::SafeDownCast(value.ToArray())->GetTuple(0));
        }
      else
        {
        vtkWarningMacro("Cannot assign this variant type to multi-component data array.");
        return;
        }
      }
    }
  else if (arr->IsA("vtkStringArray"))
    {
    vtkStringArray* data = vtkStringArray::SafeDownCast(arr);
    if (comps == 1)
      {
      data->SetValue(row, value.ToString());
      }
    else
      {
      if (value.IsArray() && value.ToArray()->IsA("vtkStringArray") && value.ToArray()->GetNumberOfComponents() == comps)
        {
        data->SetTuple(row, 0, vtkStringArray::SafeDownCast(value.ToArray()));
        }
      else
        {
        vtkWarningMacro("Cannot assign this variant type to multi-component string array.");
        return;
        }
      }
    }
  else if (arr->IsA("vtkVariantArray"))
    {
    vtkVariantArray* data = vtkVariantArray::SafeDownCast(arr);
    if (comps == 1)
      {
      data->SetValue(row, value);
      }
    else
      {
      if (value.IsArray() && value.ToArray()->GetNumberOfComponents() == comps)
        {
        data->SetTuple(row, 0, value.ToArray());
        }
      else
        {
        vtkWarningMacro("Cannot assign this variant type to multi-component string array.");
        return;
        }
      }
    }
}

//----------------------------------------------------------------------------

vtkVariant vtkTable::GetValue(vtkIdType row, vtkIdType col)
{
  int column = static_cast<int>(col);
  return this->GetValueByName(row, this->GetColumnName(column));
}


//----------------------------------------------------------------------------

template <typename iterT>
vtkVariant vtkTableGetVariantValue(iterT* it, vtkIdType row)
{
  return vtkVariant(it->GetValue(row));
}

//----------------------------------------------------------------------------

vtkVariant vtkTable::GetValueByName(vtkIdType row, const char* col)
{
  vtkAbstractArray* arr = this->GetColumnByName(col);
  int comps = arr->GetNumberOfComponents();
  if (arr->IsA("vtkDataArray"))
    {
    if (comps == 1)
      {
      vtkArrayIterator* iter = arr->NewIterator();
      vtkVariant v;
      switch(arr->GetDataType())
        {
        vtkArrayIteratorTemplateMacro(
          v = vtkTableGetVariantValue(static_cast<VTK_TT*>(iter), row));
        }
      iter->Delete();
      return v;
      }
    else
      {
      // Create a variant holding an array of the appropriate type
      // with one tuple.
      vtkDataArray* da = vtkDataArray::CreateDataArray(arr->GetDataType());
      da->SetNumberOfComponents(comps);
      da->InsertNextTuple(row, arr);
      vtkVariant v(da);
      da->Delete();
      return v;
      }
    }
  else if (arr->IsA("vtkStringArray"))
    {
    vtkStringArray* data = vtkStringArray::SafeDownCast(arr);
    if (comps == 1)
      {
      return vtkVariant(data->GetValue(row));
      }
    else
      {
      // Create a variant holding a vtkStringArray with one tuple.
      vtkStringArray* sa = vtkStringArray::New();
      sa->SetNumberOfComponents(comps);
      sa->InsertNextTuple(row, data);
      vtkVariant v(sa);
      sa->Delete();
      return v;
      }
    }
  else if (arr->IsA("vtkVariantArray"))
    {
    vtkVariantArray* data = vtkVariantArray::SafeDownCast(arr);
    if (comps == 1)
      {
      return data->GetValue(row);
      }
    else
      {
      // Create a variant holding a vtkVariantArray with one tuple.
      vtkVariantArray* va = vtkVariantArray::New();
      va->SetNumberOfComponents(comps);
      va->InsertNextTuple(row, data);
      vtkVariant v(va);
      va->Delete();
      return v;
      }
    }
  return vtkVariant();
}

//----------------------------------------------------------------------------
vtkTable* vtkTable::GetData(vtkInformation* info)
{
  return info? vtkTable::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkTable* vtkTable::GetData(vtkInformationVector* v, int i)
{
  return vtkTable::GetData(v->GetInformationObject(i));
}
