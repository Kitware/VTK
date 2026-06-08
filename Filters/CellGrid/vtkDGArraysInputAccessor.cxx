// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGArraysInputAccessor.h"

#include "vtkDataArray.h"

VTK_ABI_NAMESPACE_BEGIN

vtkDGArraysInputAccessor::vtkDGArraysInputAccessor(vtkDataArray* cellIds, vtkDataArray* rst)
{
  this->SetCellIds(cellIds);
  this->SetRST(rst);
}

vtkDGArraysInputAccessor::vtkDGArraysInputAccessor(const vtkDGArraysInputAccessor& other)
{
  if (&other == this)
  {
    return;
  }
  this->SetCellIds(other.CellIds);
  this->SetRST(other.RST);
  this->Key = other.Key;
}

vtkDGArraysInputAccessor::~vtkDGArraysInputAccessor()
{
  this->SetCellIds(nullptr);
  this->SetRST(nullptr);
}

vtkDGArraysInputAccessor& vtkDGArraysInputAccessor::operator=(const vtkDGArraysInputAccessor& other)
{
  if (&other == this)
  {
    return *this;
  }
  this->SetCellIds(other.CellIds);
  this->SetRST(other.RST);
  this->Key = other.Key;
  return *this;
}

void vtkDGArraysInputAccessor::SetCellIds(vtkDataArray* cellIds)
{
  if (this->CellIds)
  {
    this->CellIds->Delete();
  }
  this->CellIds = cellIds;
  if (this->CellIds)
  {
    this->CellIds->Register(nullptr);
  }
}

void vtkDGArraysInputAccessor::SetRST(vtkDataArray* rst)
{
  if (this->RST)
  {
    this->RST->Delete();
  }
  this->RST = rst;
  if (this->RST)
  {
    this->RST->Register(nullptr);
  }
}

vtkIdType vtkDGArraysInputAccessor::GetCellId(vtkTypeUInt64 iteration)
{
  vtkTypeUInt64 cellId;
  this->CellIds->GetUnsignedTuple(iteration, &cellId);
  return static_cast<vtkIdType>(cellId);
}

vtkVector3d vtkDGArraysInputAccessor::GetParameter(vtkTypeUInt64 iteration)
{
  vtkVector3d rst;
  this->RST->GetTuple(iteration, rst.GetData());
  return rst;
}

void vtkDGArraysInputAccessor::Restart()
{
  this->Key = 0;
}

bool vtkDGArraysInputAccessor::IsAtEnd() const
{
  return this->Key >= static_cast<vtkTypeUInt64>(this->CellIds->GetNumberOfValues());
}

std::size_t vtkDGArraysInputAccessor::size() const
{
  return static_cast<std::size_t>(this->CellIds->GetNumberOfValues());
}

vtkTypeUInt64 vtkDGArraysInputAccessor::operator++()
{
  auto nn = static_cast<vtkTypeUInt64>(this->CellIds->GetNumberOfValues());
  if (this->Key < nn)
  {
    ++this->Key;
  }
  return this->Key;
}

vtkTypeUInt64 vtkDGArraysInputAccessor::operator++(int)
{
  auto nn = static_cast<vtkTypeUInt64>(this->CellIds->GetNumberOfValues());
  auto vv = this->Key;
  if (this->Key < nn)
  {
    ++this->Key;
  }
  return vv;
}

vtkDGArraysInputAccessor& vtkDGArraysInputAccessor::operator+=(vtkTypeUInt64 count)
{
  auto nn = static_cast<vtkTypeUInt64>(this->CellIds->GetNumberOfValues());
  if (this->Key + count > nn)
  {
    this->Key = nn;
  }
  else
  {
    this->Key += nn;
  }
  return *this;
}

VTK_ABI_NAMESPACE_END
