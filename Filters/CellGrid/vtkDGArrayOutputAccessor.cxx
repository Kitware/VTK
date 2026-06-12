// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGArrayOutputAccessor.h"

#include "vtkDoubleArray.h"

VTK_ABI_NAMESPACE_BEGIN

vtkDGArrayOutputAccessor::vtkDGArrayOutputAccessor(vtkDoubleArray* result)
{
  this->SetResult(result);
}

vtkDGArrayOutputAccessor::vtkDGArrayOutputAccessor(const vtkDGArrayOutputAccessor& other)
{
  if (&other == this)
  {
    return;
  }
  this->SetResult(other.Result);
  this->Key = other.Key;
}

vtkDGArrayOutputAccessor::~vtkDGArrayOutputAccessor()
{
  this->SetResult(nullptr);
}

vtkDGArrayOutputAccessor& vtkDGArrayOutputAccessor::operator=(const vtkDGArrayOutputAccessor& other)
{
  if (&other == this)
  {
    return *this;
  }
  this->SetResult(other.Result);
  this->Key = other.Key;
  return *this;
}

void vtkDGArrayOutputAccessor::SetResult(vtkDoubleArray* result)
{
  if (this->Result)
  {
    this->Result->Delete();
  }
  this->Result = result;
  if (this->Result)
  {
    this->Result->Register(nullptr);
  }
}

vtkDGArrayOutputAccessor::Tuple vtkDGArrayOutputAccessor::operator[](vtkTypeUInt64 tupleId)
{
  if (tupleId >= static_cast<vtkTypeUInt64>(this->Result->GetNumberOfTuples()))
  {
    return Tuple();
  }
  int sz = this->Result->GetNumberOfComponents();
  return Tuple(this->Result->GetPointer(0) + tupleId * sz, sz);
}

vtkDGArrayOutputAccessor::Tuple vtkDGArrayOutputAccessor::GetTuple()
{
  if (this->Key >= static_cast<vtkTypeUInt64>(this->Result->GetNumberOfTuples()))
  {
    return Tuple();
  }
  int sz = this->Result->GetNumberOfComponents();
  return Tuple(this->Result->GetPointer(0) + this->Key * sz, sz);
}

void vtkDGArrayOutputAccessor::Restart()
{
  this->Key = 0;
}

bool vtkDGArrayOutputAccessor::IsAtEnd() const
{
  return this->Key >= static_cast<vtkTypeUInt64>(this->Result->GetNumberOfValues());
}

std::size_t vtkDGArrayOutputAccessor::size() const
{
  return static_cast<std::size_t>(this->Result->GetNumberOfValues());
}

vtkTypeUInt64 vtkDGArrayOutputAccessor::operator++()
{
  auto nn = static_cast<vtkTypeUInt64>(this->Result->GetNumberOfValues());
  if (this->Key < nn)
  {
    ++this->Key;
  }
  return this->Key;
}

vtkTypeUInt64 vtkDGArrayOutputAccessor::operator++(int)
{
  auto nn = static_cast<vtkTypeUInt64>(this->Result->GetNumberOfValues());
  auto vv = this->Key;
  if (this->Key < nn)
  {
    ++this->Key;
  }
  return vv;
}

vtkDGArrayOutputAccessor& vtkDGArrayOutputAccessor::operator+=(vtkTypeUInt64 count)
{
  auto nn = static_cast<vtkTypeUInt64>(this->Result->GetNumberOfValues());
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
