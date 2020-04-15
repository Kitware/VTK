/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedArchiver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedArchiver.h"

#include <vtkObjectFactory.h>

#include <archive.h>
#include <archive_entry.h>

#include <iterator>
#include <map>
#include <string>

struct vtkPartitionedArchiver::Internal
{
  std::map<std::string, std::pair<size_t, char*> > Buffers;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPartitionedArchiver);

//----------------------------------------------------------------------------
vtkPartitionedArchiver::vtkPartitionedArchiver()
  : Internals(new vtkPartitionedArchiver::Internal)
{
  this->SetArchiveName("");
}

//----------------------------------------------------------------------------
vtkPartitionedArchiver::~vtkPartitionedArchiver()
{
  for (auto& bufferIt : this->Internals->Buffers)
  {
    free(bufferIt.second.second);
  }
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPartitionedArchiver::OpenArchive() {}

//----------------------------------------------------------------------------
void vtkPartitionedArchiver::CloseArchive() {}

//----------------------------------------------------------------------------
void vtkPartitionedArchiver::InsertIntoArchive(
  const std::string& relativePath, const char* data, std::size_t size)
{
  struct archive* a = archive_write_new();

  // use zip format
  archive_write_set_format_zip(a);

  // Avoid buffer exhausted errors by guaranteeing a sane minimum buffer size.
  // The value 10240 is libarchive's default buffer size when writing explicitly
  // to file.
  size_t bufferSize = (size > 10240 ? size : 10240);

  size_t used = 0;
  char* b = (char*)malloc(bufferSize);
  if (b == nullptr)
  {
    vtkErrorMacro(<< "Error allocating memory for buffer.");
    archive_write_free(a);
    return;
  }

  {
    archive_write_open_memory(a, b, bufferSize, &used);

    struct archive_entry* entry;

    entry = archive_entry_new();
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_entry_set_size(entry, size);
    archive_entry_set_pathname(entry, relativePath.c_str());

    if (archive_write_header(a, entry) != ARCHIVE_OK || archive_write_data(a, data, size) < 0)
    {
      vtkErrorMacro(<< "Error writing to buffer: " << archive_error_string(a));
      archive_write_free(a);
      free(b);
      return;
    }

    archive_entry_free(entry);

    if (archive_write_close(a) != ARCHIVE_OK)
    {
      vtkErrorMacro(<< "Error closing buffer: " << archive_error_string(a));
      archive_write_free(a);
      free(b);
      return;
    }

    archive_write_free(a);
  }

  // Free previous buffer if we are overwriting a previous path
  auto bufferIt = this->Internals->Buffers.find(relativePath);
  if (bufferIt != this->Internals->Buffers.end())
  {
    free(bufferIt->second.second);
  }

  this->Internals->Buffers[relativePath] = std::make_pair(used, b);
}

//----------------------------------------------------------------------------
bool vtkPartitionedArchiver::Contains(const std::string& relativePath)
{
  return this->Internals->Buffers.find(relativePath) != this->Internals->Buffers.end();
}

//----------------------------------------------------------------------------
const char* vtkPartitionedArchiver::GetBuffer(const char* relativePath)
{
  auto bufferIt = this->Internals->Buffers.find(std::string(relativePath));
  if (bufferIt != this->Internals->Buffers.end())
  {
    return bufferIt->second.second;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
const void* vtkPartitionedArchiver::GetBufferAddress(const char* relativePath)
{
  auto bufferIt = this->Internals->Buffers.find(std::string(relativePath));
  if (bufferIt != this->Internals->Buffers.end())
  {
    return bufferIt->second.second;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
std::size_t vtkPartitionedArchiver::GetBufferSize(const char* relativePath)
{
  auto bufferIt = this->Internals->Buffers.find(std::string(relativePath));
  if (bufferIt != this->Internals->Buffers.end())
  {
    return bufferIt->second.first;
  }
  return 0;
}

//----------------------------------------------------------------------------
std::size_t vtkPartitionedArchiver::GetNumberOfBuffers()
{
  return this->Internals->Buffers.size();
}

//----------------------------------------------------------------------------
const char* vtkPartitionedArchiver::GetBufferName(size_t i)
{
  if (this->Internals->Buffers.size() <= i)
  {
    return nullptr;
  }
  return std::next(this->Internals->Buffers.begin(), i)->first.c_str();
}

//----------------------------------------------------------------------------
void vtkPartitionedArchiver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
