// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkDiagonalMatrixSource.h"
#include "vtkArrayData.h"
#include "vtkDenseArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"

//------------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDiagonalMatrixSource);

//------------------------------------------------------------------------------

vtkDiagonalMatrixSource::vtkDiagonalMatrixSource()
  : ArrayType(DENSE)
  , Extents(3)
  , Diagonal(1.0)
  , SuperDiagonal(0.0)
  , SubDiagonal(0.0)
  , RowLabel(nullptr)
  , ColumnLabel(nullptr)
{
  this->SetRowLabel("rows");
  this->SetColumnLabel("columns");

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------

vtkDiagonalMatrixSource::~vtkDiagonalMatrixSource()
{
  this->SetRowLabel(nullptr);
  this->SetColumnLabel(nullptr);
}

//------------------------------------------------------------------------------

void vtkDiagonalMatrixSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayType: " << this->ArrayType << endl;
  os << indent << "Extents: " << this->Extents << endl;
  os << indent << "Diagonal: " << this->Diagonal << endl;
  os << indent << "SuperDiagonal: " << this->SuperDiagonal << endl;
  os << indent << "SubDiagonal: " << this->SubDiagonal << endl;
  os << indent << "RowLabel: " << (this->RowLabel ? this->RowLabel : "") << endl;
  os << indent << "ColumnLabel: " << (this->ColumnLabel ? this->ColumnLabel : "") << endl;
}

//------------------------------------------------------------------------------

int vtkDiagonalMatrixSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (this->Extents < 1)
  {
    vtkErrorMacro(<< "Invalid matrix extents: " << this->Extents << "x" << this->Extents
                  << " array is not supported.");
    return 0;
  }

  vtkArray* array = nullptr;
  switch (this->ArrayType)
  {
    case DENSE:
      array = this->GenerateDenseArray();
      break;
    case SPARSE:
      array = this->GenerateSparseArray();
      break;
    default:
      vtkErrorMacro(<< "Invalid array type: " << this->ArrayType << ".");
      return 0;
  }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(array);
  array->Delete();

  return 1;
}

vtkArray* vtkDiagonalMatrixSource::GenerateDenseArray()
{
  vtkDenseArray<double>* const array = vtkDenseArray<double>::New();
  array->Resize(vtkArrayExtents::Uniform(2, this->Extents));
  array->SetDimensionLabel(0, this->RowLabel);
  array->SetDimensionLabel(1, this->ColumnLabel);

  array->Fill(0.0);

  if (this->Diagonal != 0.0)
  {
    for (vtkIdType i = 0; i != this->Extents; ++i)
      array->SetValue(vtkArrayCoordinates(i, i), this->Diagonal);
  }

  if (this->SuperDiagonal != 0.0)
  {
    for (vtkIdType i = 0; i + 1 != this->Extents; ++i)
      array->SetValue(vtkArrayCoordinates(i, i + 1), this->SuperDiagonal);
  }

  if (this->SubDiagonal != 0.0)
  {
    for (vtkIdType i = 0; i + 1 != this->Extents; ++i)
      array->SetValue(vtkArrayCoordinates(i + 1, i), this->SubDiagonal);
  }

  return array;
}

vtkArray* vtkDiagonalMatrixSource::GenerateSparseArray()
{
  vtkSparseArray<double>* const array = vtkSparseArray<double>::New();
  array->Resize(vtkArrayExtents::Uniform(2, this->Extents));
  array->SetDimensionLabel(0, this->RowLabel);
  array->SetDimensionLabel(1, this->ColumnLabel);

  if (this->Diagonal != 0.0)
  {
    for (vtkIdType i = 0; i != this->Extents; ++i)
      array->AddValue(vtkArrayCoordinates(i, i), this->Diagonal);
  }

  if (this->SuperDiagonal != 0.0)
  {
    for (vtkIdType i = 0; i + 1 != this->Extents; ++i)
      array->AddValue(vtkArrayCoordinates(i, i + 1), this->SuperDiagonal);
  }

  if (this->SubDiagonal != 0.0)
  {
    for (vtkIdType i = 0; i + 1 != this->Extents; ++i)
      array->AddValue(vtkArrayCoordinates(i + 1, i), this->SubDiagonal);
  }

  return array;
}
VTK_ABI_NAMESPACE_END
