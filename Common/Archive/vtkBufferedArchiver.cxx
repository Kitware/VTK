/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBufferedArchiver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBufferedArchiver.h"

#include <vtkObjectFactory.h>

#include <archive.h>
#include <archive_entry.h>

#include <set>

struct vtkBufferedArchiver::Internal
{
  struct archive* Archive;
  char* Buffer;
  size_t AllocatedSize;
  size_t BufferSize;
  std::set<std::string> Entries;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBufferedArchiver);

//----------------------------------------------------------------------------
vtkBufferedArchiver::vtkBufferedArchiver()
  : Internals(new vtkBufferedArchiver::Internal)
{
  this->Internals->AllocatedSize = 100000;
  this->Internals->Buffer = nullptr;
  this->SetArchiveName("");
}

//----------------------------------------------------------------------------
vtkBufferedArchiver::~vtkBufferedArchiver()
{
  free(this->Internals->Buffer);
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkBufferedArchiver::OpenArchive()
{
  this->Internals->Archive = archive_write_new();

  // use zip format
  archive_write_set_format_zip(this->Internals->Archive);

  this->Internals->Buffer = (char*)malloc(this->Internals->AllocatedSize);
  if (this->Internals->Buffer == nullptr)
  {
    vtkErrorMacro(<< "Error allocating memory for buffer.");
    return;
  }

  archive_write_open_memory(this->Internals->Archive, this->Internals->Buffer,
    this->Internals->AllocatedSize, &this->Internals->BufferSize);
}

//----------------------------------------------------------------------------
void vtkBufferedArchiver::CloseArchive()
{
  archive_write_free(this->Internals->Archive);
}

//----------------------------------------------------------------------------
void vtkBufferedArchiver::InsertIntoArchive(
  const std::string& relativePath, const char* data, std::size_t size)
{
  struct archive_entry* entry;

  entry = archive_entry_new();
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);
  archive_entry_set_size(entry, size);
  archive_entry_set_pathname(entry, relativePath.c_str());
  archive_write_header(this->Internals->Archive, entry);
  archive_write_data(this->Internals->Archive, data, size);
  archive_entry_free(entry);

  this->Internals->Entries.insert(relativePath);
}

//----------------------------------------------------------------------------
bool vtkBufferedArchiver::Contains(const std::string& relativePath)
{
  return this->Internals->Entries.find(relativePath) != this->Internals->Entries.end();
}

//----------------------------------------------------------------------------
const char* vtkBufferedArchiver::GetBuffer()
{
  return this->Internals->Buffer;
}

//----------------------------------------------------------------------------
const void* vtkBufferedArchiver::GetBufferAddress()
{
  return this->Internals->Buffer;
}

//----------------------------------------------------------------------------
void vtkBufferedArchiver::SetAllocatedSize(std::size_t size)
{
  this->Internals->AllocatedSize = size;
}

//----------------------------------------------------------------------------
std::size_t vtkBufferedArchiver::GetAllocatedSize()
{
  return this->Internals->AllocatedSize;
}

//----------------------------------------------------------------------------
std::size_t vtkBufferedArchiver::GetBufferSize()
{
  return this->Internals->BufferSize;
}

//----------------------------------------------------------------------------
void vtkBufferedArchiver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
