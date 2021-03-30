/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkASCIITextCodec.cxx

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

#include "vtkASCIITextCodec.h"

#include "vtkObjectFactory.h"
#include "vtkTextCodecFactory.h"

#include <stdexcept>

vtkStandardNewMacro(vtkASCIITextCodec);

const char* vtkASCIITextCodec::Name()
{
  return "US-ASCII";
}

bool vtkASCIITextCodec::CanHandle(const char* NameStr)
{
  return vtkTextCodec::CanHandle(NameStr) || (0 == strcmp(NameStr, "ASCII"));
}

vtkTypeUInt32 vtkASCIITextCodec::NextUTF32CodePoint(istream& InputStream)
{
  vtkTypeUInt32 CodePoint = InputStream.get();

  if (!InputStream.eof())
  {
    if (CodePoint > 0x7f)
      throw std::runtime_error("Detected a character that isn't valid US-ASCII.");

    return CodePoint;
  }
  else
  {
    throw std::runtime_error("End of Input");
  }
}

vtkASCIITextCodec::vtkASCIITextCodec() = default;

vtkASCIITextCodec::~vtkASCIITextCodec() = default;

void vtkASCIITextCodec::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkASCIITextCodec (" << this << ") \n";
  indent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}
