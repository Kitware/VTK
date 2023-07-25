// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkTextCodec.h"
#include <vtk_utf8.h>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
// iterator to throw away input.
class bucketIterator : public vtkTextCodec::OutputIterator
{
public:
  bucketIterator& operator=(const vtkTypeUInt32&) override { return *this; }
};

class stringIterator : public vtkTextCodec::OutputIterator
{
public:
  stringIterator(std::string& output)
    : Output(output)
  {
  }
  stringIterator& operator=(const vtkTypeUInt32& value) override
  {
    utf8::append(value, std::back_inserter(this->Output));
    return *this;
  }

private:
  std::string& Output;
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

vtkTextCodec::~vtkTextCodec() = default;

vtkTextCodec::vtkTextCodec() = default;

std::string vtkTextCodec::ToString(istream& inputStream)
{
  std::string result;
  stringIterator iterator(result);
  this->ToUnicode(inputStream, iterator);
  return result;
}

void vtkTextCodec::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkTextCodec (" << this << ") \n";
  indent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
