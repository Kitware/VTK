/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataElement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataElement.h"

#include "vtkTable.h"
#include "vtkAbstractArray.h"

//------------------------------------------------------------------------------
vtkDataElement::vtkDataElement() :
  Type(SCALAR),
  Dimension(0),
  Valid(false),
  Table(NULL),
  AbstractArray(NULL),
  Index(-1)
{
}

//------------------------------------------------------------------------------
vtkDataElement::vtkDataElement(vtkVariant v) :
  Type(SCALAR),
  Dimension(0),
  Valid(true),
  Scalar(v),
  Table(NULL),
  AbstractArray(NULL),
  Index(-1)
{
}

//------------------------------------------------------------------------------
vtkDataElement::vtkDataElement(vtkTable* table) :
  Type(TABLE),
  Dimension(0),
  Valid(true),
  Table(table),
  AbstractArray(NULL),
  Index(-1)
{
}

//------------------------------------------------------------------------------
vtkDataElement::vtkDataElement(vtkTable* table, vtkIdType row) :
  Type(TABLE_ROW),
  Dimension(0),
  Valid(true),
  Table(table),
  AbstractArray(NULL),
  Index(row)
{
}

//------------------------------------------------------------------------------
vtkDataElement::vtkDataElement(vtkAbstractArray* arr) :
  Type(ABSTRACT_ARRAY),
  Dimension(0),
  Valid(true),
  Table(NULL),
  AbstractArray(arr),
  Index(-1)
{
}

//------------------------------------------------------------------------------
vtkDataElement::vtkDataElement(vtkAbstractArray* arr, vtkIdType index,
                               int type) :
  Type(type),
  Dimension(0),
  Valid(true),
  Table(NULL),
  AbstractArray(arr),
  Index(index)
{
}

//-----------------------------------------------------------------------------
vtkIdType vtkDataElement::GetNumberOfChildren()
{
  switch (this->Type)
    {
    case TABLE:
      if (this->Dimension == 0)
        {
        return this->Table->GetNumberOfRows();
        }
      else
        {
        return this->Table->GetNumberOfColumns();
        }
    case TABLE_ROW:
      return this->Table->GetNumberOfColumns();
    case ABSTRACT_ARRAY:
      if (this->Dimension == 0)
        {
        return this->AbstractArray->GetNumberOfTuples();
        }
      else
        {
        return this->AbstractArray->GetNumberOfComponents();
        }
    case ABSTRACT_ARRAY_TUPLE:
      return this->AbstractArray->GetNumberOfComponents();
    case ABSTRACT_ARRAY_COMPONENT:
      return this->AbstractArray->GetNumberOfTuples();
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataElement vtkDataElement::GetChild(vtkIdType i)
{
  switch (this->Type)
    {
    case TABLE:
      if (this->Dimension == 0)
        {
        return vtkDataElement(this->Table, i);
        }
      else
        {
        return vtkDataElement(this->Table->GetColumn(i));
        }
    case TABLE_ROW:
      return vtkDataElement(this->Table->GetValue(this->Index, i));
    case ABSTRACT_ARRAY:
      if (this->Dimension == 0)
        {
        return vtkDataElement(this->AbstractArray, i, ABSTRACT_ARRAY_TUPLE);
        }
      else
        {
        return vtkDataElement(this->AbstractArray, i, ABSTRACT_ARRAY_COMPONENT);
        }
    case ABSTRACT_ARRAY_TUPLE:
      return vtkDataElement(this->AbstractArray->GetVariantValue(this->Index*this->AbstractArray->GetNumberOfComponents() + i));
    case ABSTRACT_ARRAY_COMPONENT:
      return vtkDataElement(this->AbstractArray->GetVariantValue(i*this->AbstractArray->GetNumberOfComponents() + this->Index));
    }
  return vtkDataElement();
}

//-----------------------------------------------------------------------------
vtkVariant vtkDataElement::GetValue(vtkIdType i)
{
  switch (this->Type)
    {
    case TABLE:
      if (this->Dimension == 0)
        {
        return this->Table->GetValue(i, 0);
        }
      else
        {
        return this->Table->GetValue(0, i);
        }
    case TABLE_ROW:
      return this->Table->GetValue(this->Index, i);
    case ABSTRACT_ARRAY:
      if (this->Dimension == 0)
        {
        return this->AbstractArray->GetVariantValue(i*this->AbstractArray->GetNumberOfComponents());
        }
      else
        {
        return this->AbstractArray->GetVariantValue(i);
        }
    case ABSTRACT_ARRAY_TUPLE:
      return this->AbstractArray->GetVariantValue(this->Index*this->AbstractArray->GetNumberOfComponents() + i);
    case ABSTRACT_ARRAY_COMPONENT:
      return this->AbstractArray->GetVariantValue(i*this->AbstractArray->GetNumberOfComponents() + this->Index);
    case SCALAR:
      return this->Scalar;
    }
  return vtkVariant();
}

//-----------------------------------------------------------------------------
vtkVariant vtkDataElement::GetValue(std::string str)
{
  switch (this->Type)
    {
    case TABLE_ROW:
      return this->Table->GetValueByName(this->Index, str.c_str());
    }
  return vtkVariant();
}

//-----------------------------------------------------------------------------
bool vtkDataElement::IsValid()
{
  return this->Valid;
}
