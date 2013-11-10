/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSQLDatabase.cxx

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

#include "vtkTextCodec.h"

const char* vtkTextCodec::Name()
{
  return "";
}


bool vtkTextCodec::CanHandle(const char*)
{
  return false;
}


bool vtkTextCodec::IsValid(istream&)
{
  return false;
}


vtkTextCodec::~vtkTextCodec()
{
}


vtkTextCodec::vtkTextCodec()
{
}


namespace
{
  class vtkUnicodeStringOutputIterator : public vtkTextCodec::OutputIterator
  {
  public:
    virtual vtkUnicodeStringOutputIterator& operator++(int);
    virtual vtkUnicodeStringOutputIterator& operator*();
    virtual vtkUnicodeStringOutputIterator& operator=(const vtkUnicodeString::value_type value);

    vtkUnicodeStringOutputIterator(vtkUnicodeString& outputString);
    ~vtkUnicodeStringOutputIterator();

  private:
    vtkUnicodeStringOutputIterator(const vtkUnicodeStringOutputIterator&); // Not implemented
    const vtkUnicodeStringOutputIterator& operator=(const vtkUnicodeStringOutputIterator&); // Not Implemented

    vtkUnicodeString& OutputString;
    unsigned int StringPosition;
  };

  vtkUnicodeStringOutputIterator& vtkUnicodeStringOutputIterator::operator++(int)
  {
    this->StringPosition++;
    return *this;
  }

  vtkUnicodeStringOutputIterator& vtkUnicodeStringOutputIterator::operator*()
  {
    return *this;
  }

  vtkUnicodeStringOutputIterator& vtkUnicodeStringOutputIterator::operator=(const vtkUnicodeString::value_type value)
  {
    this->OutputString += value;
    return *this;
  }

  vtkUnicodeStringOutputIterator::vtkUnicodeStringOutputIterator(vtkUnicodeString& outputString) :
    OutputString(outputString), StringPosition(0)
  {
  }

  vtkUnicodeStringOutputIterator::~vtkUnicodeStringOutputIterator()
  {
  }
}


vtkUnicodeString vtkTextCodec::ToUnicode(istream& InputStream)
{
  // create an output string stream
  vtkUnicodeString returnString;

  vtkUnicodeStringOutputIterator StringIterator(returnString);
  this->ToUnicode(InputStream, StringIterator);

  return returnString;
}


void vtkTextCodec::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkTextCodec (" << this << ") \n";
  indent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
