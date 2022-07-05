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

#include "vtkTable.h"
#include "vtkArrayIteratorIncludes.h"

#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"

#include <algorithm>
#include <vector>

//
// Standard functions
//

vtkStandardNewMacro(vtkTable);
vtkStandardExtendedNewMacro(vtkTable);
vtkCxxSetObjectMacro(vtkTable, RowData, vtkDataSetAttributes);

//------------------------------------------------------------------------------
vtkTable::vtkTable()
{
  this->RowArray = vtkVariantArray::New();
  this->RowData = vtkDataSetAttributes::New();

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkTable::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject::PrintSelf(os, indent);
  os << indent << "RowData: " << (this->RowData ? "" : "(none)") << endl;
  if (this->RowData)
  {
    this->RowData->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
void vtkTable::Dump(unsigned int colWidth, int rowLimit)

{
  if (!this->GetNumberOfColumns())
  {
    cout << "++\n++\n";
    return;
  }

  std::string lineStr;
  for (int c = 0; c < this->GetNumberOfColumns(); ++c)
  {
    lineStr += "+-";

    for (unsigned int i = 0; i < colWidth; ++i)
    {
      lineStr += "-";
    }
  }
  lineStr += "-+\n";

  cout << lineStr;

  for (int c = 0; c < this->GetNumberOfColumns(); ++c)
  {
    cout << "| ";
    const char* name = this->GetColumnName(c);
    std::string str = name ? name : "";

    if (colWidth < str.length())
    {
      cout << str.substr(0, colWidth);
    }
    else
    {
      cout << str;
      for (unsigned int i = static_cast<unsigned int>(str.length()); i < colWidth; ++i)
      {
        cout << " ";
      }
    }
  }

  cout << " |\n" << lineStr;

  if (rowLimit != 0)
  {
    for (vtkIdType r = 0; r < this->GetNumberOfRows(); ++r)
    {
      for (int c = 0; c < this->GetNumberOfColumns(); ++c)
      {
        cout << "| ";
        std::string str = this->GetValue(r, c).ToString();

        if (colWidth < str.length())
        {
          cout << str.substr(0, colWidth);
        }
        else
        {
          cout << str;
          for (unsigned int i = static_cast<unsigned int>(str.length()); i < colWidth; ++i)
          {
            cout << " ";
          }
        }
      }
      cout << " |\n";
      if (rowLimit != -1 && r >= rowLimit)
        break;
    }
    cout << lineStr;
    cout.flush();
  }
}

//------------------------------------------------------------------------------
void vtkTable::Initialize()
{
  this->Superclass::Initialize();
  if (this->RowData)
  {
    this->RowData->Initialize();
  }
}

//------------------------------------------------------------------------------
unsigned long vtkTable::GetActualMemorySize()
{
  return this->RowData->GetActualMemorySize() + this->Superclass::GetActualMemorySize();
}

//
// Row functions
//

//------------------------------------------------------------------------------
vtkIdType vtkTable::GetNumberOfRows()
{
  if (this->GetNumberOfColumns() > 0)
  {
    return this->GetColumn(0)->GetNumberOfTuples();
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkTable::SetNumberOfRows(vtkIdType n)
{
  // to preserve data first call Resize() on all arrays
  for (int i = 0; i < this->GetNumberOfColumns(); i++)
  {
    this->GetColumn(i)->Resize(n);
  }
  this->RowData->SetNumberOfTuples(n);
}

//------------------------------------------------------------------------------
void vtkTable::SqueezeRows()
{
  this->RowData->Squeeze();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkTable::GetRow(vtkIdType row, vtkVariantArray* values)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  values->SetNumberOfTuples(ncol);
  for (vtkIdType i = 0; i < ncol; i++)
  {
    values->SetValue(i, this->GetValue(row, i));
  }
}

//------------------------------------------------------------------------------
void vtkTable::SetRow(vtkIdType row, vtkVariantArray* values)
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

//------------------------------------------------------------------------------
void vtkTable::MoveRowData(vtkIdType first, vtkIdType last, vtkIdType delta)
{
  if ((first < 0) || (last < 0) || (first > last) || (delta == 0))
  {
    return;
  }

  // determine the move direction
  // if delta is positive then we need to start at the back of the source segment
  // and work forwards, else at the beginning of the source segment and backwards
  vtkIdType start = first;
  vtkIdType stop = last;
  vtkIdType step = +1;
  if (delta > 0)
  {
    start = last;
    stop = first;
    step = -1;
  }

  vtkIdType ncol = this->GetNumberOfColumns();
  for (vtkIdType i = 0; i < ncol; i++)
  {
    vtkAbstractArray* arr = this->GetColumn(i);
    int comps = arr->GetNumberOfComponents();
    if (vtkArrayDownCast<vtkDataArray>(arr))
    {
      vtkDataArray* data = vtkArrayDownCast<vtkDataArray>(arr);
      for (vtkIdType row = start; row * step <= stop * step; row += step)
      {
        data->SetTuple(row + delta, row, data);
      }
    }
    else if (vtkArrayDownCast<vtkStringArray>(arr))
    {
      vtkStringArray* data = vtkArrayDownCast<vtkStringArray>(arr);
      for (vtkIdType row = start; row * step <= stop * step; row += step)
      {
        for (int j = 0; j < comps; j++)
        {
          data->SetValue((row + delta) * comps + j, data->GetValue(row * comps + j));
        }
      }
    }
    else if (vtkArrayDownCast<vtkVariantArray>(arr))
    {
      vtkVariantArray* data = vtkArrayDownCast<vtkVariantArray>(arr);
      for (vtkIdType row = start; row * step <= stop * step; row += step)
      {
        for (int j = 0; j < comps; j++)
        {
          data->SetValue((row + delta) * comps + j, data->GetValue(row * comps + j));
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkTable::InsertRow(vtkIdType row)
{
  this->InsertRows(row, 1);
}

//------------------------------------------------------------------------------
void vtkTable::InsertRows(vtkIdType row, vtkIdType n)
{
  if (n <= 0)
  {
    return;
  }

  row = std::max<vtkIdType>(0, std::min<vtkIdType>(row, this->GetNumberOfRows()));

  vtkIdType NRows = this->GetNumberOfRows();
  vtkIdType newNRows = std::max<vtkIdType>(NRows, row) + n;

  // enlarge the table
  this->SetNumberOfRows(newNRows);

  // move the existing elements backwards
  vtkIdType first = row;
  vtkIdType last = NRows - 1;
  this->MoveRowData(first, last, n);
}

//------------------------------------------------------------------------------
vtkIdType vtkTable::InsertNextBlankRow(double default_num_val)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  std::vector<double> tuple(32, default_num_val);
  for (vtkIdType i = 0; i < ncol; i++)
  {
    vtkAbstractArray* arr = this->GetColumn(i);
    const size_t comps = static_cast<size_t>(arr->GetNumberOfComponents());
    if (vtkArrayDownCast<vtkDataArray>(arr))
    {
      if (comps > tuple.size())
      { // We initialize this to 32 components, but just in case...
        tuple.resize(comps, default_num_val);
      }

      vtkDataArray* data = vtkArrayDownCast<vtkDataArray>(arr);
      data->InsertNextTuple(tuple.data());
    }
    else if (vtkArrayDownCast<vtkStringArray>(arr))
    {
      vtkStringArray* data = vtkArrayDownCast<vtkStringArray>(arr);
      for (size_t j = 0; j < comps; j++)
      {
        data->InsertNextValue(std::string());
      }
    }
    else if (vtkArrayDownCast<vtkVariantArray>(arr))
    {
      vtkVariantArray* data = vtkArrayDownCast<vtkVariantArray>(arr);
      for (size_t j = 0; j < comps; j++)
      {
        data->InsertNextValue(vtkVariant());
      }
    }
    else
    {
      vtkErrorMacro(<< "Unsupported array type for InsertNextBlankRow");
    }
  }
  return this->GetNumberOfRows() - 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkTable::InsertNextRow(vtkVariantArray* values)
{
  vtkIdType ncol = this->GetNumberOfColumns();
  if (values->GetNumberOfTuples() != ncol)
  {
    vtkErrorMacro(<< "Incorrect number of tuples in SetRow."
                  << " Expected " << ncol << ", but got " << values->GetNumberOfTuples());
  }
  vtkIdType row = this->InsertNextBlankRow();
  for (vtkIdType i = 0; i < ncol; i++)
  {
    this->SetValue(row, i, values->GetValue(i));
  }
  return row;
}

//------------------------------------------------------------------------------
void vtkTable::RemoveRow(vtkIdType row)
{
  this->RemoveRows(row, 1);
}

//------------------------------------------------------------------------------
void vtkTable::RemoveRows(vtkIdType row, vtkIdType n)
{
  if (n <= 0)
  {
    return;
  }

  vtkIdType NRows = this->GetNumberOfRows();
  vtkIdType NRemove = std::max<vtkIdType>(0, std::min<vtkIdType>(n, NRows - row));
  vtkIdType newNRows = std::max<vtkIdType>(0, NRows - NRemove);
  if (newNRows == NRows)
  {
    return;
  }

  // move the existing elements forwards
  vtkIdType first = row + n;
  vtkIdType last = NRows - 1;
  this->MoveRowData(first, last, -n);

  // shrink the table
  this->SetNumberOfRows(newNRows);
}

//------------------------------------------------------------------------------
void vtkTable::RemoveAllRows()
{
  vtkIdType ncol = this->GetNumberOfColumns();
  for (vtkIdType i = 0; i < ncol; i++)
  {
    vtkAbstractArray* arr = this->GetColumn(i);
    if (vtkArrayDownCast<vtkDataArray>(arr))
    {
      arr->SetNumberOfTuples(0);
    }
    else
    {
      arr->SetNumberOfValues(0);
    }
  }
}

//
// Column functions
//

//------------------------------------------------------------------------------
vtkIdType vtkTable::GetNumberOfColumns()
{
  return this->RowData->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
void vtkTable::AddColumn(vtkAbstractArray* arr)
{
  if (this->GetNumberOfColumns() > 0 && arr->GetNumberOfTuples() != this->GetNumberOfRows())
  {
    vtkErrorMacro(<< "Column \"" << arr->GetName() << "\" must have " << this->GetNumberOfRows()
                  << " rows, but has " << arr->GetNumberOfTuples() << ".");
    return;
  }
  this->RowData->AddArray(arr);
}

//------------------------------------------------------------------------------
void vtkTable::InsertColumn(vtkAbstractArray* arr, vtkIdType index)
{
  if (this->GetNumberOfColumns() > 0 && arr->GetNumberOfTuples() != this->GetNumberOfRows())
  {
    vtkErrorMacro(<< "Column \"" << arr->GetName() << "\" must have " << this->GetNumberOfRows()
                  << " rows, but has " << arr->GetNumberOfTuples() << ".");
    return;
  }
  // ensure index is sensible
  index = std::max<vtkIdType>(0, std::min<vtkIdType>(this->GetNumberOfColumns(), index));

  // insert at end?
  if (index == this->GetNumberOfColumns())
  {
    this->AddColumn(arr);
    return;
  }

  // remove all arrays from RowData, then insert them again in correct order with new array inserted
  // note: use vtkSmartPointer to preserve a reference count, else this->RowData->RemoveArray(0)
  // will delete the array
  vtkIdType ncols = this->GetNumberOfColumns();
  std::vector<vtkSmartPointer<vtkAbstractArray>> store;
  store.reserve(ncols);

  for (int c = 0; c < ncols; c++)
  {
    if (c == index)
    {
      store.emplace_back(arr);
    }
    store.emplace_back(this->GetColumn(0));
    this->RowData->RemoveArray(0);
  }

  for (unsigned long c = 0; c < store.size(); c++)
  {
    this->RowData->AddArray(store[c]);
  }
}

//------------------------------------------------------------------------------
void vtkTable::RemoveColumnByName(const char* name)
{
  this->RowData->RemoveArray(name);
}

//------------------------------------------------------------------------------
void vtkTable::RemoveColumn(vtkIdType col)
{
  int column = static_cast<int>(col);
  this->RowData->RemoveArray(this->RowData->GetArrayName(column));
}

//------------------------------------------------------------------------------
void vtkTable::RemoveAllColumns()
{
  int narrays = this->RowData->GetNumberOfArrays();
  for (int i = 0; i < narrays; i++)
  {
    this->RowData->RemoveArray(0);
  }
}

//------------------------------------------------------------------------------
const char* vtkTable::GetColumnName(vtkIdType col)
{
  int column = static_cast<int>(col);
  return this->RowData->GetArrayName(column);
}

//------------------------------------------------------------------------------
vtkAbstractArray* vtkTable::GetColumnByName(const char* name)
{
  return this->RowData->GetAbstractArray(name);
}

//------------------------------------------------------------------------------
vtkIdType vtkTable::GetColumnIndex(const char* name)
{
  const char* col_name;
  for (int i = 0; i < this->GetNumberOfColumns(); i++)
  {
    col_name = this->GetColumnName(i);
    if (col_name && !strcmp(col_name, name))
    {
      return i;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
vtkAbstractArray* vtkTable::GetColumn(vtkIdType col)
{
  int column = static_cast<int>(col);
  return this->RowData->GetAbstractArray(column);
}

//
// Table single entry functions
//

//------------------------------------------------------------------------------
void vtkTable::SetValue(vtkIdType row, vtkIdType col, vtkVariant value)
{
  vtkAbstractArray* arr = this->GetColumn(col);
  if (!arr)
  {
    return;
  }
  int comps = arr->GetNumberOfComponents();
  if (vtkArrayDownCast<vtkDataArray>(arr))
  {
    vtkDataArray* data = vtkArrayDownCast<vtkDataArray>(arr);
    if (comps == 1)
    {
      data->SetVariantValue(row, value);
    }
    else
    {
      if (value.IsArray() && vtkArrayDownCast<vtkDataArray>(value.ToArray()) &&
        value.ToArray()->GetNumberOfComponents() == comps)
      {
        data->SetTuple(row, vtkArrayDownCast<vtkDataArray>(value.ToArray())->GetTuple(0));
      }
      else
      {
        vtkWarningMacro("Cannot assign this variant type to multi-component data array.");
        return;
      }
    }
  }
  else if (vtkArrayDownCast<vtkStringArray>(arr))
  {
    vtkStringArray* data = vtkArrayDownCast<vtkStringArray>(arr);
    if (comps == 1)
    {
      data->SetValue(row, value.ToString());
    }
    else
    {
      if (value.IsArray() && vtkArrayDownCast<vtkStringArray>(value.ToArray()) &&
        value.ToArray()->GetNumberOfComponents() == comps)
      {
        data->SetTuple(row, 0, vtkArrayDownCast<vtkStringArray>(value.ToArray()));
      }
      else
      {
        vtkWarningMacro("Cannot assign this variant type to multi-component string array.");
        return;
      }
    }
  }
  else if (vtkArrayDownCast<vtkVariantArray>(arr))
  {
    vtkVariantArray* data = vtkArrayDownCast<vtkVariantArray>(arr);
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
  else
  {
    vtkWarningMacro("Unable to process array named " << col);
  }
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
template <typename iterT>
vtkVariant vtkTableGetVariantValue(iterT* it, vtkIdType row)
{
  return vtkVariant(it->GetValue(row));
}

//------------------------------------------------------------------------------
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
  if (vtkArrayDownCast<vtkDataArray>(arr))
  {
    if (comps == 1)
    {
      vtkArrayIterator* iter = arr->NewIterator();
      vtkVariant v;
      switch (arr->GetDataType())
      {
        vtkArrayIteratorTemplateMacro(v = vtkTableGetVariantValue(static_cast<VTK_TT*>(iter), row));
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
  else if (vtkArrayDownCast<vtkStringArray>(arr))
  {
    vtkStringArray* data = vtkArrayDownCast<vtkStringArray>(arr);
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
  else if (vtkArrayDownCast<vtkVariantArray>(arr))
  {
    vtkVariantArray* data = vtkArrayDownCast<vtkVariantArray>(arr);
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

//------------------------------------------------------------------------------
vtkVariant vtkTable::GetValueByName(vtkIdType row, const char* col)
{
  int colIndex = GetColumnIndex(col);
  if (colIndex < 0)
  {
    return vtkVariant();
  }
  return this->GetValue(row, colIndex);
}

//------------------------------------------------------------------------------
vtkTable* vtkTable::GetData(vtkInformation* info)
{
  return info ? vtkTable::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkTable* vtkTable::GetData(vtkInformationVector* v, int i)
{
  return vtkTable::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkTable::ShallowCopy(vtkDataObject* src)
{
  if (vtkTable* const table = vtkTable::SafeDownCast(src))
  {
    this->RowData->ShallowCopy(table->RowData);
    this->Modified();
  }

  this->Superclass::ShallowCopy(src);
}

//------------------------------------------------------------------------------
void vtkTable::DeepCopy(vtkDataObject* src)
{
  auto mkhold = vtkMemkindRAII(this->GetIsInMemkind());
  if (vtkTable* const table = vtkTable::SafeDownCast(src))
  {
    this->RowData->DeepCopy(table->RowData);
    this->Modified();
  }

  Superclass::DeepCopy(src);
}

//------------------------------------------------------------------------------
vtkFieldData* vtkTable::GetAttributesAsFieldData(int type)
{
  switch (type)
  {
    case ROW:
      return this->GetRowData();
  }
  return this->Superclass::GetAttributesAsFieldData(type);
}

//------------------------------------------------------------------------------
vtkIdType vtkTable::GetNumberOfElements(int type)
{
  switch (type)
  {
    case ROW:
      return this->GetNumberOfRows();
  }
  return this->Superclass::GetNumberOfElements(type);
}
