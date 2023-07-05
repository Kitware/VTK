// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2010 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkASCIITextCodec.h"

#include "vtkObjectFactory.h"
#include "vtkTextCodecFactory.h"

#include <stdexcept>

VTK_ABI_NAMESPACE_BEGIN
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
VTK_ABI_NAMESPACE_END
