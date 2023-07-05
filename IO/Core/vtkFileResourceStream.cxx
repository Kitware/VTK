// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFileResourceStream.h"

#include "vtkObjectFactory.h"

#include <vtksys/FStream.hxx>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkFileResourceStream);

//------------------------------------------------------------------------------
struct vtkFileResourceStream::vtkInternals
{
  vtksys::ifstream File;
};

//------------------------------------------------------------------------------
vtkFileResourceStream::vtkFileResourceStream()
  : vtkResourceStream{ true }
  , Impl{ new vtkFileResourceStream::vtkInternals }
{
}

//------------------------------------------------------------------------------
vtkFileResourceStream::~vtkFileResourceStream() = default;

//------------------------------------------------------------------------------
bool vtkFileResourceStream::Open(VTK_FILEPATH const char* path)
{
  if (this->Impl->File.is_open())
  {
    this->Impl->File.close();
  }

  if (path)
  {
    this->Impl->File.open(path, std::ios_base::binary);
  }

  this->Modified();
  return this->Impl->File.is_open();
}

//------------------------------------------------------------------------------
std::size_t vtkFileResourceStream::Read(void* buffer, std::size_t bytes)
{
  if (bytes == 0)
  {
    return 0;
  }

  this->Impl->File.read(static_cast<char*>(buffer), bytes);

  return static_cast<std::size_t>(this->Impl->File.gcount());
}

//------------------------------------------------------------------------------
bool vtkFileResourceStream::EndOfStream()
{
  return !(this->Impl->File && this->Impl->File.is_open());
}

//------------------------------------------------------------------------------
vtkTypeInt64 vtkFileResourceStream::Seek(vtkTypeInt64 pos, SeekDirection dir)
{
  // We want EndOfFile to be false after a Seek call, so we have to clear the stream
  // seekg will clear eofbit, but we need to clear failbit. badbit should be kept.
  this->Impl->File.clear(this->Impl->File.rdstate() & ~(std::ios_base::failbit));

  switch (dir)
  {
    case SeekDirection::Begin:
      return static_cast<vtkTypeInt64>(this->Impl->File.seekg(pos, std::ios_base::beg).tellg());
    case SeekDirection::Current:
      return static_cast<vtkTypeInt64>(this->Impl->File.seekg(pos, std::ios_base::cur).tellg());
    case SeekDirection::End:
      return static_cast<vtkTypeInt64>(this->Impl->File.seekg(pos, std::ios_base::end).tellg());
  }

  return -1;
}

//------------------------------------------------------------------------------
vtkTypeInt64 vtkFileResourceStream::Tell()
{
  return static_cast<vtkTypeInt64>(this->Impl->File.tellg());
}

//------------------------------------------------------------------------------
void vtkFileResourceStream::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Open: " << (this->Impl->File.is_open() ? "yes" : "no") << "\n";
}

VTK_ABI_NAMESPACE_END
