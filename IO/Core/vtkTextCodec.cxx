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

namespace
{
// iterator to throw away input.
class bucketIterator : public vtkTextCodec::OutputIterator
{
public:
  bucketIterator& operator=(const vtkTypeUInt32&) override { return *this; }
};

} // end anonymous namespace

const char* vtkTextCodec::Name()
{
  return "";
}

bool vtkTextCodec::CanHandle(const char* NameStr)
{
  return (0 == strcmp(NameStr, Name()));
}

bool vtkTextCodec::IsValid(istream& InputStream)
{
  bool returnBool = true;
  // get the position of the stream so we can restore it when we are done
  istream::pos_type StreamPos = InputStream.tellg();

  try
  {
    bucketIterator bucket;
    this->ToUnicode(InputStream, bucket);
  }
  catch (...)
  {
    returnBool = false;
  }

  // reset the stream
  InputStream.clear();
  InputStream.seekg(StreamPos);

  return returnBool;
}

void vtkTextCodec::ToUnicode(istream& inputStream, vtkTextCodec::OutputIterator& output)
{
  try
  {
    while (!inputStream.eof())
    {
      const vtkTypeUInt32 CodePoint = NextUTF32CodePoint(inputStream);
      *output++ = CodePoint;
    }
  }
  catch (...)
  {
    if (!inputStream.eof())
    {
      throw;
    }
  }
}

vtkUnicodeString::value_type vtkTextCodec::NextUnicode(istream& inputStream)
{
  return NextUTF32CodePoint(inputStream);
}

vtkTextCodec::~vtkTextCodec() = default;

vtkTextCodec::vtkTextCodec() = default;

namespace
{
class vtkUnicodeStringOutputIterator : public vtkTextCodec::OutputIterator
{
public:
  vtkUnicodeStringOutputIterator& operator++(int) override;
  vtkUnicodeStringOutputIterator& operator*() override;
  vtkUnicodeStringOutputIterator& operator=(const vtkTypeUInt32& value) override;

  vtkUnicodeStringOutputIterator(vtkUnicodeString& outputString);
  ~vtkUnicodeStringOutputIterator() override;

private:
  vtkUnicodeStringOutputIterator(const vtkUnicodeStringOutputIterator&) = delete;
  vtkUnicodeStringOutputIterator& operator=(const vtkUnicodeStringOutputIterator&) = delete;

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

vtkUnicodeStringOutputIterator& vtkUnicodeStringOutputIterator::operator=(
  const vtkTypeUInt32& value)
{
  this->OutputString += value;
  return *this;
}

vtkUnicodeStringOutputIterator::vtkUnicodeStringOutputIterator(vtkUnicodeString& outputString)
  : OutputString(outputString)
  , StringPosition(0)
{
}

vtkUnicodeStringOutputIterator::~vtkUnicodeStringOutputIterator() = default;
}

std::string vtkTextCodec::ToString(istream& inputStream)
{
  return ToUnicode(inputStream).utf8_str();
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
