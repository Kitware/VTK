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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkArrayIteratorIncludes.h"
#include "vtkTable.h"

#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkUnicodeStringArray.h"
#include "vtkVariantArray.h"

//
// Standard functions
//

vtkStandardNewMacro(vtkTable);
vtkCxxSetObjectMacro(vtkTable, RowData, vtkDataSetAttributes);

//----------------------------------------------------------------------------
vtkTable::vtkTable()
{
  this->RowArray = vtkVariantArray::New();
  this->RowData = vtkDataSetAttributes::New();

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
}

//----------------------------------------------------------------------------
vtkTable::~vtkTable()
{
  if (this->RowArray)
    {
    this->RowArray->Delete();
    }
  if (this->RowData)
    {
    this->RowData->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkTable::PrintSelf(ostream &os, vtkIndent indent)
{
  vtkDataObject::PrintSelf(os, indent);
  os << indent << "RowData: " << (this->RowData ? "" : "(none)") << endl;
  if (this->RowData)
    {
    this->RowData->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkTable::Dump( unsigned int colWidth, int rowLimit )

{
  if ( ! this->GetNumberOfColumns() )
    {
    cout << "++\n++\n";
    return;
    }

  vtkStdString lineStr;
  for ( int c = 0; c < this->GetNumberOfColumns(); ++ c )
    {
    lineStr += "+-";

    for ( unsigned int i = 0; i < colWidth; ++ i )
      {
      lineStr += "-";
      }
    }
  lineStr += "-+\n";

  cout << lineStr;

  for ( int c = 0; c < this->GetNumberOfColumns(); ++ c )
    {
    cout << "| ";
    const char* name = this->GetColumnName( c );
    vtkStdString str = name ? name : "";

    if ( colWidth < str.length() )
      {
      cout << str.substr( 0, colWidth );
      }
    else
      {
      cout << str;
      for ( unsigned int i = static_cast<unsigned int>(str.length()); i < colWidth; ++ i )
        {
        cout << " ";
        }
      }
    }

  cout << " |\n"
       << lineStr;

  if ( rowLimit != 0 )
    {
    for ( vtkIdType r = 0; r < this->GetNumberOfRows(); ++ r )
      {
      for ( int c = 0; c < this->GetNumberOfColumns(); ++ c )
        {
        cout << "| ";
        vtkStdString str = this->GetValue( r, c ).ToString();

        if ( colWidth < str.length() )
          {
          cout << str.substr( 0, colWidth );
          }
        else
          {
          cout << str;
          for ( unsigned int i = static_cast<unsigned int>(str.length()); i < colWidth; ++ i )
            {
            cout << " ";
            }
          }
        }
      cout << " |\n";
      if ( rowLimit != -1 && r >= rowLimit )
        break;
      }
    cout << lineStr;
    cout.flush();
    }
}

//----------------------------------------------------------------------------
void vtkTable::Initialize()
{
  this->Superclass::Initialize();
  if (this->RowData)
    {
    this->RowData->Initialize();
    }
}

//----------------------------------------------------------------------------
unsigned long vtkTable::GetActualMemorySize()
{
  return this->RowData->GetActualMemorySize() +
         this->Superclass::GetActualMemorySize();
}

//
// Row functions
//

//----------------------------------------------------------------------------
vtkIdType vtkTable::GetNumberOfRows()
{
  if (this->GetNumberOfColumns() > 0)
    {
    return this->GetColumn(0)->GetNumberOfTuples();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkTable::SetNumberOfRows( vtkIdType n )
{
  if( this->RowData )
    {
      this->RowData->SetNumberOfTuples( n );
    }
}

//----------------------------------------------------------------------------
vtkVariantArray* vtkTable::GetRow(vtkIdType row)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  this->RowArray->SetNumberOfTuples(ncol);
  for (vtkIdType i = 0; i < ncol; i++)
    {
    this->RowArray->SetValue(i, this->GetValue(row, i));
    }
  return this->RowArray;
}

//----------------------------------------------------------------------------
void vtkTable::GetRow(vtkIdType row, vtkVariantArray *values)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  values->SetNumberOfTuples(ncol);
  for (vtkIdType i = 0; i < ncol; i++)
    {
    values->SetValue(i, this->GetValue(row, i));
    }
}

//----------------------------------------------------------------------------
void vtkTable::SetRow(vtkIdType row, vtkVariantArray *values)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  if (values->GetNumberOfTuples() != ncol)
    {
    vtkErrorMacro(<< "Incorrect number of tuples in SetRow");
    }
  for (vtkIdType i = 0; i < ncol; i++)
    {
    this->SetValue(row, i, values->GetValue(i));
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkTable::InsertNextBlankRow(double default_num_val)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  for (vtkIdType i = 0; i < ncol; i++)
    {
    vtkAbstractArray* arr = this->GetColumn(i);
    int comps = arr->GetNumberOfComponents();
    if (vtkDataArray::SafeDownCast(arr))
      {
      vtkDataArray* data = vtkDataArray::SafeDownCast(arr);
      double* tuple = new double[comps];
      for (int j = 0; j < comps; j++)
        {
        tuple[j] = default_num_val;
        }
      data->InsertNextTuple(tuple);
      delete[] tuple;
      }
    else if (vtkStringArray::SafeDownCast(arr))
      {
      vtkStringArray* data = vtkStringArray::SafeDownCast(arr);
      for (int j = 0; j < comps; j++)
        {
        data->InsertNextValue(vtkStdString(""));
        }
      }
    else if (vtkVariantArray::SafeDownCast(arr))
      {
      vtkVariantArray* data = vtkVariantArray::SafeDownCast(arr);
      for (int j = 0; j < comps; j++)
        {
        data->InsertNextValue(vtkVariant());
        }
      }
    else if (vtkUnicodeStringArray::SafeDownCast(arr))
      {
      vtkUnicodeStringArray* data = vtkUnicodeStringArray::SafeDownCast(arr);
      for (int j = 0; j < comps; j++)
        {
        data->InsertNextValue(vtkUnicodeString::from_utf8(""));
        }
      }
    else
      {
      vtkErrorMacro(<< "Unsupported array type for InsertNextBlankRow");
      }
    }
  return this->GetNumberOfRows() - 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkTable::InsertNextRow(vtkVariantArray* values)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  if (values->GetNumberOfTuples() != ncol)
    {
    vtkErrorMacro(<< "Incorrect number of tuples in SetRow");
    }
  vtkIdType row = this->InsertNextBlankRow();
  for (vtkIdType i = 0; i < ncol; i++)
    {
    this->SetValue(row, i, values->GetValue(i));
    }
  return row;
}

//----------------------------------------------------------------------------
void vtkTable::RemoveRow(vtkIdType row)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  for (vtkIdType i = 0; i < ncol; i++)
    {
    vtkAbstractArray* arr = this->GetColumn(i);
    int comps = arr->GetNumberOfComponents();
    if (vtkDataArray::SafeDownCast(arr))
      {
      vtkDataArray* data = vtkDataArray::SafeDownCast(arr);
      data->RemoveTuple(row);
      }
    else if (vtkStringArray::SafeDownCast(arr))
      {
      // Manually move all elements past the index back one place.
      vtkStringArray* data = vtkStringArray::SafeDownCast(arr);
      for (int j = comps*row; j < comps*data->GetNumberOfTuples() - 1; j++)
        {
        data->SetValue(j, data->GetValue(j+1));
        }
      data->Resize(data->GetNumberOfTuples() - 1);
      }
    else if (vtkVariantArray::SafeDownCast(arr))
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
}

//
// Column functions
//

//----------------------------------------------------------------------------
vtkIdType vtkTable::GetNumberOfColumns()
{
  return this->RowData->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkTable::AddColumn(vtkAbstractArray* arr)
{
  if (this->GetNumberOfColumns() > 0 &&
      arr->GetNumberOfTuples() != this->GetNumberOfRows())
    {
    vtkErrorMacro(<< "Column \"" << arr->GetName() << "\" must have "
      << this->GetNumberOfRows() << " rows, but has "
      << arr->GetNumberOfTuples() << ".");
    return;
    }
  this->RowData->AddArray(arr);
}

//----------------------------------------------------------------------------
void vtkTable::RemoveColumnByName(const char* name)
{
  this->RowData->RemoveArray(name);
}

//----------------------------------------------------------------------------
void vtkTable::RemoveColumn(vtkIdType col)
{
  int column = static_cast<int>(col);
  this->RowData->RemoveArray(this->RowData->GetArrayName(column));
}

//----------------------------------------------------------------------------
const char* vtkTable::GetColumnName(vtkIdType col)
{
  int column = static_cast<int>(col);
  return this->RowData->GetArrayName(column);
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkTable::GetColumnByName(const char* name)
{
  return this->RowData->GetAbstractArray(name);
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkTable::GetColumn(vtkIdType col)
{
  int column = static_cast<int>(col);
  return this->RowData->GetAbstractArray(column);
}

//
// Table single entry functions
//

//----------------------------------------------------------------------------
void vtkTable::SetValue(vtkIdType row, vtkIdType col, vtkVariant value)
{
  vtkAbstractArray* arr = this->GetColumn(col);
  if (!arr)
    {
    return;
    }
  int comps = arr->GetNumberOfComponents();
  if (vtkDataArray::SafeDownCast(arr))
    {
    vtkDataArray* data = vtkDataArray::SafeDownCast(arr);
    if (comps == 1)
      {
      data->SetVariantValue(row, value);
      }
    else
      {
      if (value.IsArray() && vtkDataArray::SafeDownCast(value.ToArray()) &&
          value.ToArray()->GetNumberOfComponents() == comps)
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
  else if (vtkStringArray::SafeDownCast(arr))
    {
    vtkStringArray* data = vtkStringArray::SafeDownCast(arr);
    if (comps == 1)
      {
      data->SetValue(row, value.ToString());
      }
    else
      {
      if (value.IsArray() && vtkStringArray::SafeDownCast(value.ToArray()) &&
          value.ToArray()->GetNumberOfComponents() == comps)
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
  else if (vtkVariantArray::SafeDownCast(arr))
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
  else if(vtkUnicodeStringArray::SafeDownCast(arr))
    {
    vtkUnicodeStringArray* data = vtkUnicodeStringArray::SafeDownCast(arr);
    if(comps==1)
      {
      data->SetValue(row, value.ToUnicodeString());
      }
    else
      {
      if(value.IsArray() && vtkUnicodeStringArray::SafeDownCast(value.ToArray()) &&
         value.ToArray()->GetNumberOfComponents() == comps)
        {
        data->SetTuple(row, 0, vtkUnicodeStringArray::SafeDownCast(value.ToArray()));
        }
      else
        {
        vtkWarningMacro("Cannot assign this variant type to multi-component unicode string array.");
        return;
        }
      }
    }
  else
    {
    vtkWarningMacro("Unable to process array named " << col);
    }
}

//----------------------------------------------------------------------------
void vtkTable::SetValueByName(vtkIdType row, const char* col, vtkVariant value)
{
  int colIndex = -1;
  this->RowData->GetAbstractArray(col, colIndex);
  if (colIndex < 0)
    {
    vtkErrorMacro(<< "Could not find column named " << col);
    return;
    }
  this->SetValue(row, colIndex, value);
}

//----------------------------------------------------------------------------
template <typename iterT>
vtkVariant vtkTableGetVariantValue(iterT* it, vtkIdType row)
{
  return vtkVariant(it->GetValue(row));
}

//----------------------------------------------------------------------------
vtkVariant vtkTable::GetValue(vtkIdType row, vtkIdType col)
{
  vtkAbstractArray* arr = this->GetColumn(col);
  if (!arr)
    {
    return vtkVariant();
    }

  int comps = arr->GetNumberOfComponents();
  if (row >= arr->GetNumberOfTuples())
    {
    return vtkVariant();
    }
  if (vtkDataArray::SafeDownCast(arr))
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
  else if (vtkStringArray::SafeDownCast(arr))
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
  else if (vtkUnicodeStringArray::SafeDownCast(arr))
    {
    vtkUnicodeStringArray* data = vtkUnicodeStringArray::SafeDownCast(arr);
    if (comps == 1)
      {
      return vtkVariant(data->GetValue(row));
      }
    else
      {
      // Create a variant holding a vtkStringArray with one tuple.
      vtkUnicodeStringArray* sa = vtkUnicodeStringArray::New();
      sa->SetNumberOfComponents(comps);
      sa->InsertNextTuple(row, data);
      vtkVariant v(sa);
      sa->Delete();
      return v;
      }
    }
  else if (vtkVariantArray::SafeDownCast(arr))
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
vtkVariant vtkTable::GetValueByName(vtkIdType row, const char* col)
{
  int colIndex = -1;
  this->RowData->GetAbstractArray(col, colIndex);
  if (colIndex < 0)
    {
    return vtkVariant();
    }
  return this->GetValue(row, colIndex);
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

//----------------------------------------------------------------------------
void vtkTable::ShallowCopy(vtkDataObject* src)
{
  if (vtkTable* const table = vtkTable::SafeDownCast(src))
    {
    this->RowData->ShallowCopy(table->RowData);
    this->Modified();
    }

  this->Superclass::ShallowCopy(src);
}

//----------------------------------------------------------------------------
void vtkTable::DeepCopy(vtkDataObject* src)
{
  if (vtkTable* const table = vtkTable::SafeDownCast(src))
    {
    this->RowData->DeepCopy(table->RowData);
    this->Modified();
    }

  Superclass::DeepCopy(src);
}

//----------------------------------------------------------------------------
vtkFieldData* vtkTable::GetAttributesAsFieldData(int type)
{
  switch(type)
    {
    case ROW:
      return this->GetRowData();
      break;
    }
  return this->Superclass::GetAttributesAsFieldData(type);
}

//----------------------------------------------------------------------------
vtkIdType vtkTable::GetNumberOfElements(int type)
{
  switch (type)
    {
    case ROW:
      return this->GetNumberOfRows();
      break;
    }
  return this->Superclass::GetNumberOfElements(type);;
}
