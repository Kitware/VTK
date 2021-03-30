/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkUTF16TextCodec.cxx

Copyright (c)
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkUTF16TextCodec.h"

#include "vtkObjectFactory.h"
#include "vtkTextCodecFactory.h"

#include <stdexcept>

vtkStandardNewMacro(vtkUTF16TextCodec);

namespace
{
//////////////////////////////////////////////////////////////////////////////
// utf16_to_unicode

vtkTypeUInt32 utf16_to_unicode_next(const bool big_endian, istream& InputStream)
{
  vtkTypeUInt8 first_byte = InputStream.get();

  if (InputStream.eof())
  {
    throw std::runtime_error("Premature end-of-sequence extracting UTF-16 code unit.");
  }
  vtkTypeUInt8 second_byte = InputStream.get();

  vtkTypeUInt32 returnCode =
    big_endian ? first_byte << 8 | second_byte : second_byte << 8 | first_byte;

  if (returnCode >= 0xd800 && returnCode <= 0xdfff)
  {
    if (InputStream.eof())
    {
      throw std::runtime_error(
        "Premature end-of-sequence extracting UTF-16 trail surrogate first byte.");
    }
    vtkTypeUInt8 third_byte = InputStream.get();

    if (InputStream.eof())
    {
      throw std::runtime_error(
        "Premature end-of-sequence extracting UTF-16 trail surrogate second byte.");
    }
    vtkTypeUInt8 fourth_byte = InputStream.get();

    const vtkTypeUInt32 second_code_unit =
      big_endian ? third_byte << 8 | fourth_byte : fourth_byte << 8 | third_byte;
    if (second_code_unit >= 0xdc00 && second_code_unit <= 0xdfff)
    {
      returnCode = vtkTypeUInt32(vtkTypeInt32(returnCode << 10) + vtkTypeInt32(second_code_unit) +
        (0x10000 - (0xd800 << 10) - 0xdc00));
    }
    else
    {
      throw std::runtime_error("Invalid UTF-16 trail surrogate.");
    }
  }
  return returnCode;
}

} // end anonymous namespace

vtkUTF16TextCodec::vtkUTF16TextCodec()
  : _endianExplicitlySet(false)
  , _bigEndian(true)
{
}

vtkUTF16TextCodec::~vtkUTF16TextCodec() = default;

const char* vtkUTF16TextCodec::Name()
{
  return "UTF-16";
}

bool vtkUTF16TextCodec::CanHandle(const char* NameString)
{
  if (vtkTextCodec::CanHandle(NameString))
  {
    _endianExplicitlySet = false;
    return true;
  }
  else if (0 == strcmp(NameString, "UTF-16BE"))
  {
    SetBigEndian(true);
    return true;
  }
  else if (0 == strcmp(NameString, "UTF-16LE"))
  {
    SetBigEndian(false);
    return true;
  }
  else
  {
    return false;
  }
}

void vtkUTF16TextCodec::SetBigEndian(bool state)
{
  _endianExplicitlySet = true;
  _bigEndian = state;
}

void vtkUTF16TextCodec::FindEndianness(istream& InputStream)
{
  _endianExplicitlySet = false;

  try
  {
    istream::char_type c1, c2;
    c1 = InputStream.get();
    if (InputStream.fail())
      throw "End of Input reached while reading header.";

    c2 = InputStream.get();
    if (InputStream.fail())
      throw "End of Input reached while reading header.";

    if (static_cast<unsigned char>(c1) == 0xfe && static_cast<unsigned char>(c2) == 0xff)
    {
      _bigEndian = true;
    }

    else if (static_cast<unsigned char>(c1) == 0xff && static_cast<unsigned char>(c2) == 0xfe)
    {
      _bigEndian = false;
    }

    else
    {
      throw std::runtime_error(
        "Cannot detect UTF-16 endianness.  Try 'UTF-16BE' or 'UTF-16LE' instead.");
    }
  }
  catch (char* cstr)
  {
    throw std::runtime_error(cstr);
  }
  catch (...)
  {
    throw std::runtime_error(
      "Cannot detect UTF-16 endianness.  Try 'UTF-16BE' or 'UTF-16LE' instead.");
  }
}

void vtkUTF16TextCodec::ToUnicode(istream& InputStream, vtkTextCodec::OutputIterator& output)
{
  if (!_endianExplicitlySet)
  {
    FindEndianness(InputStream);
  }

  vtkTextCodec::ToUnicode(InputStream, output);
}

vtkTypeUInt32 vtkUTF16TextCodec::NextUTF32CodePoint(istream& inputStream)
{
  return utf16_to_unicode_next(_bigEndian, inputStream);
}

void vtkUTF16TextCodec::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkUTF16TextCodec (" << this << ") \n";
  indent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
