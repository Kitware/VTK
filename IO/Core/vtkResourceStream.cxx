/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkResourceStream.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
