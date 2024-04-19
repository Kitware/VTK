// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResourceStream.h"

VTK_ABI_NAMESPACE_BEGIN

struct vtkResourceStream::vtkInternals
{
  bool SupportSeek;
};

//------------------------------------------------------------------------------
vtkResourceStream::vtkResourceStream(bool supportSeek)
  : Impl{ new vtkInternals{} }
{
  this->Impl->SupportSeek = supportSeek;
}

//------------------------------------------------------------------------------
vtkResourceStream::~vtkResourceStream() = default;

//------------------------------------------------------------------------------
vtkTypeInt64 vtkResourceStream::Seek(vtkTypeInt64, SeekDirection)
{
  return 0;
}

//------------------------------------------------------------------------------
vtkTypeInt64 vtkResourceStream::Tell()
{
  return this->Seek(0, SeekDirection::Current);
}

bool vtkResourceStream::SupportSeek() const
{
  return this->Impl->SupportSeek;
}

//------------------------------------------------------------------------------
void vtkResourceStream::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Support seek: " << (this->Impl->SupportSeek ? "yes" : "no") << "\n";
}

VTK_ABI_NAMESPACE_END
