// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSystemIncludes.h" // Cannot include vtkOStrStreamWrapper.h directly.

// Need strcpy.
#include <string>

#include <sstream>

using std::ostringstream;

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkOStrStreamWrapper::vtkOStrStreamWrapper()
  : vtkOStreamWrapper(*(new ostringstream))
{
  this->Result = nullptr;
  this->Frozen = 0;
}

//------------------------------------------------------------------------------
vtkOStrStreamWrapper::~vtkOStrStreamWrapper()
{
  if (!this->Frozen)
  {
    delete[] this->Result;
  }
  delete &this->ostr;
}

//------------------------------------------------------------------------------
char* vtkOStrStreamWrapper::str()
{
  if (!this->Result)
  {
    std::string s = static_cast<ostringstream*>(&this->ostr)->str();
    this->Result = new char[s.length() + 1];
    strcpy(this->Result, s.c_str());
    this->freeze();
  }
  return this->Result;
}

//------------------------------------------------------------------------------
vtkOStrStreamWrapper* vtkOStrStreamWrapper::rdbuf()
{
  return this;
}

//------------------------------------------------------------------------------
void vtkOStrStreamWrapper::freeze()
{
  this->freeze(1);
}

//------------------------------------------------------------------------------
void vtkOStrStreamWrapper::freeze(int f)
{
  this->Frozen = f;
}
VTK_ABI_NAMESPACE_END
