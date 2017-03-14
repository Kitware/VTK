/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransposeTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTransposeTable.h"

#include "vtkAbstractArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVariantArray.h"

#include <sstream>

///////////////////////////////////////////////////////////////////////////////

class vtkTransposeTableInternal
{
public:
  vtkTransposeTableInternal(vtkTransposeTable* parent) : Parent(parent) {}

  bool TransposeTable(vtkTable* inTable, vtkTable* outTable);

protected:

  bool InsertColumn(int, vtkAbstractArray*);

  template<typename ArrayType, typename ValueType>
  bool TransposeColumn(int, bool);

  vtkTransposeTable* Parent;
  vtkTable* InTable;
  vtkTable* OutTable;
};

//----------------------------------------------------------------------------

template<typename ArrayType, typename ValueType>
bool vtkTransposeTableInternal::TransposeColumn(int columnId, bool useVariant)
{
  vtkAbstractArray* column = this->InTable->GetColumn(columnId);
  ArrayType* typeColumn = ArrayType::SafeDownCast(column);
  if (!typeColumn && !useVariant)
  {
    return false;
  }

  int numberOfRowsInTransposedColumn = this->InTable->GetNumberOfColumns();
  if (this->Parent->GetUseIdColumn())
  {
    columnId--;
    numberOfRowsInTransposedColumn--;
  }

  for (int r = 0; r < column->GetNumberOfTuples() *
    column->GetNumberOfComponents(); ++r)
  {
    vtkSmartPointer<ArrayType> transposedColumn;
    if (columnId == 0)
    {
      transposedColumn = vtkSmartPointer<ArrayType>::New();
      transposedColumn->SetNumberOfValues(numberOfRowsInTransposedColumn);
      this->OutTable->AddColumn(transposedColumn);
    }
    else
    {
      transposedColumn = ArrayType::SafeDownCast(this->OutTable->GetColumn(r));
    }

    if (!useVariant)
    {
      ValueType value = typeColumn->GetValue(r);
      transposedColumn->SetValue(columnId, value);
    }
    else
    {
      vtkVariant value = column->GetVariantValue(r);
      transposedColumn->SetVariantValue(columnId, value);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkTransposeTableInternal::InsertColumn(int pos, vtkAbstractArray* col)
{
  if (!col || ((this->OutTable->GetNumberOfRows() !=
    col->GetNumberOfComponents() * col->GetNumberOfTuples()) &&
    (this->OutTable->GetNumberOfRows() != 0)))
  {
    return false;
  }

  int nbColsOutTable = this->OutTable->GetNumberOfColumns();

  vtkNew<vtkTable> updatedTable;
  for (int c = 0; c < nbColsOutTable; c++)
  {
    vtkAbstractArray* column = this->OutTable->GetColumn(c);
    if (c == pos)
    {
      updatedTable->AddColumn(col);
    }
    updatedTable->AddColumn(column);
  }
  if (pos == nbColsOutTable)
  {
    updatedTable->AddColumn(col);
  }

  this->OutTable->ShallowCopy(updatedTable.GetPointer());

  return true;
}

//----------------------------------------------------------------------------
bool vtkTransposeTableInternal::TransposeTable(vtkTable* inTable,
                                               vtkTable* outTable)
{
  this->InTable = inTable;
  this->OutTable = outTable;

  int idColOffset = this->Parent->GetUseIdColumn() ? 1 : 0;

  // Check column type consistency
  bool useVariant = false;
  vtkAbstractArray* firstCol = this->InTable->GetColumn(idColOffset);
  for (int c = idColOffset; c < this->InTable->GetNumberOfColumns(); c++)
  {
    if (strcmp(firstCol->GetClassName(),
      this->InTable->GetColumn(c)->GetClassName()) != 0)
    {
      useVariant = true;
      break;
    }
  }
  for (int c = idColOffset; c < this->InTable->GetNumberOfColumns(); c++)
  {
    vtkAbstractArray* column = this->InTable->GetColumn(c);
    if (!column)
    {
      return false;
    }
    if (!useVariant)
    {
#define TransposeTypedColumn(_vt, _ta, _t) \
  case _vt:\
    if (!this->TransposeColumn<_ta, _t>(c, useVariant))\
    {\
      vtkErrorWithObjectMacro(this->Parent, <<\
        "Unable to transpose column " << c);\
        return false;\
    }\
    break;

      switch (column->GetDataType())
      {
        TransposeTypedColumn(VTK_DOUBLE, vtkDoubleArray,
          double);
        TransposeTypedColumn(VTK_FLOAT, vtkFloatArray,
          float);
        TransposeTypedColumn(VTK_CHAR, vtkCharArray,
          char);
        TransposeTypedColumn(VTK_SIGNED_CHAR, vtkSignedCharArray,
          signed char);
        TransposeTypedColumn(VTK_SHORT, vtkShortArray,
          short);
        TransposeTypedColumn(VTK_INT, vtkIntArray,
          int);
        TransposeTypedColumn(VTK_LONG, vtkLongArray,
          long);
        TransposeTypedColumn(VTK_LONG_LONG, vtkLongLongArray,
          long long);
        TransposeTypedColumn(VTK_UNSIGNED_CHAR, vtkUnsignedCharArray,
          unsigned char);
        TransposeTypedColumn(VTK_UNSIGNED_SHORT, vtkUnsignedShortArray,
          unsigned short);
        TransposeTypedColumn(VTK_UNSIGNED_INT, vtkUnsignedIntArray,
          unsigned int);
        TransposeTypedColumn(VTK_UNSIGNED_LONG, vtkUnsignedLongArray,
          unsigned long);
        TransposeTypedColumn(VTK_UNSIGNED_LONG_LONG, vtkUnsignedLongLongArray,
          unsigned long long);
        TransposeTypedColumn(VTK_ID_TYPE, vtkIdTypeArray,
          vtkIdType);
        TransposeTypedColumn(VTK_STRING, vtkStringArray,
          vtkStdString);
#undef TransposeTypedColumn
        default:
          useVariant = true;
          break;
      }
    }
    if (useVariant)
    {
      if (!this->TransposeColumn<vtkVariantArray, vtkVariant>(c, useVariant))
      {
        vtkErrorWithObjectMacro(this->Parent, << "Unable to transpose column " << c);
        return false;
      }
    }
  }

  // Compute the number of chars needed to write the largest column id
  std::stringstream ss;
  ss << firstCol->GetNumberOfTuples();
  std::size_t maxBLen = ss.str().length();

  // Set id column on transposed table
  firstCol = this->InTable->GetColumn(0);
  for (int r = 0; r < firstCol->GetNumberOfComponents() *
    firstCol->GetNumberOfTuples(); r++)
  {
    vtkAbstractArray* destColumn = this->OutTable->GetColumn(r);
    if (this->Parent->GetUseIdColumn())
    {
      destColumn->SetName(firstCol->GetVariantValue(r).ToString());
    }
    else
    {
      // Set the column name to the (padded) row id.
      // We padd ids with 0 to avoid downstream dictionary sort issues.
      std::stringstream ss2;
      ss2 << std::setw(maxBLen) << std::setfill('0');
      ss2 << r;
      destColumn->SetName(ss2.str().c_str());
    }
  }

  // Create and insert the id column
  if (this->Parent->GetAddIdColumn())
  {
    vtkNew<vtkStringArray> stringArray;
    stringArray->SetName(this->Parent->GetUseIdColumn() ?
      this->InTable->GetColumn(0)->GetName() : this->Parent->GetIdColumnName());
    stringArray->SetNumberOfValues(
      this->InTable->GetNumberOfColumns() - idColOffset);
    for (int c = idColOffset; c < this->InTable->GetNumberOfColumns(); ++c)
    {
      stringArray->SetValue(c - idColOffset, this->InTable->GetColumn(c)->GetName());
    }
    this->InsertColumn(0, stringArray.GetPointer());
  }

  return true;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTransposeTable);

//----------------------------------------------------------------------------
vtkTransposeTable::vtkTransposeTable()
{
  this->AddIdColumn = true;
  this->UseIdColumn = false;
  this->IdColumnName = 0;
  this->SetIdColumnName("ColName");
}

//----------------------------------------------------------------------------
vtkTransposeTable::~vtkTransposeTable()
{
  delete [] IdColumnName;
}

//----------------------------------------------------------------------------
void vtkTransposeTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkTransposeTable::RequestData(vtkInformation*,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{
  vtkTable* inTable = vtkTable::GetData(inputVector[0]);
  vtkTable* outTable = vtkTable::GetData(outputVector, 0);

  if (inTable->GetNumberOfColumns() == 0)
  {
    vtkErrorMacro(<<
      "vtkTransposeTable requires vtkTable containing at least one column.");
    return 0;
  }

  vtkTransposeTableInternal intern(this);
  return intern.TransposeTable(inTable, outTable) ? 1 : 0;
}
