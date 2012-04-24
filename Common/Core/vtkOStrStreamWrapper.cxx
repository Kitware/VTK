/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOStrStreamWrapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSystemIncludes.h" // Cannot include vtkOStrStreamWrapper.h directly.

// Need strcpy.
#include <string>

#include <sstream>

using std::ostringstream;

//----------------------------------------------------------------------------
vtkOStrStreamWrapper::vtkOStrStreamWrapper()
  : vtkOStreamWrapper(*(new ostringstream))
{
  this->Result = 0;
  this->Frozen = 0;
}

//----------------------------------------------------------------------------
vtkOStrStreamWrapper::~vtkOStrStreamWrapper()
{
  if(this->Result && !this->Frozen)
    {
    delete [] this->Result;
    }
  delete &this->ostr;
}

//----------------------------------------------------------------------------
char* vtkOStrStreamWrapper::str()
{
  if(!this->Result)
    {
    std::string s = static_cast<ostringstream*>(&this->ostr)->str();
    this->Result = new char[s.length()+1];
    strcpy(this->Result, s.c_str());
    this->freeze();
    }
  return this->Result;
}

//----------------------------------------------------------------------------
vtkOStrStreamWrapper* vtkOStrStreamWrapper::rdbuf()
{
  return this;
}

//----------------------------------------------------------------------------
void vtkOStrStreamWrapper::freeze()
{
  this->freeze(1);
}

//----------------------------------------------------------------------------
void vtkOStrStreamWrapper::freeze(int f)
{
  this->Frozen = f;
}
