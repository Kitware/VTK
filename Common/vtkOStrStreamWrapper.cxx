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
#include <string.h>

// Use ANSI ostrstream or ostringstream.
#ifdef VTK_USE_ANSI_STDLIB
# ifndef VTK_NO_ANSI_STRING_STREAM
#  include <sstream>
using std::ostringstream;
# else
#  include <strstream>
using std::ostrstream;
using std::ends;
# endif

// Use old-style strstream.
#else
# if defined(_MSC_VER)
#  include <strstrea.h>
# else
#  include <strstream.h>
# endif
#endif

//----------------------------------------------------------------------------
#if defined(VTK_USE_ANSI_STDLIB) && !defined(VTK_NO_ANSI_STRING_STREAM)
vtkOStrStreamWrapper::vtkOStrStreamWrapper():
  vtkOStreamWrapper(*(new ostringstream))
{
  this->Result = 0;
  this->Frozen = 0;
}
#else
vtkOStrStreamWrapper::vtkOStrStreamWrapper():
  vtkOStreamWrapper(*(new ostrstream))
{
  this->Result = 0;
  this->Frozen = 0;
}
#endif

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
#if defined(VTK_USE_ANSI_STDLIB) && !defined(VTK_NO_ANSI_STRING_STREAM)
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
#else
char* vtkOStrStreamWrapper::str()
{
  if(!this->Result)
    {
    this->ostr << ends;
    this->Result = static_cast<ostrstream*>(&this->ostr)->str();
    this->freeze();
    }
  return this->Result;
}
#endif

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
