/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLZMADataCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLZMADataCompressor.h"
#include "vtkObjectFactory.h"
#include "vtk_lzma.h"

vtkStandardNewMacro(vtkLZMADataCompressor);

//----------------------------------------------------------------------------
vtkLZMADataCompressor::vtkLZMADataCompressor()
{
  this->CompressionLevel = 5;
}

//----------------------------------------------------------------------------
vtkLZMADataCompressor::~vtkLZMADataCompressor() = default;

//----------------------------------------------------------------------------
void vtkLZMADataCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CompressionLevel: " << this->CompressionLevel << endl;
}

//----------------------------------------------------------------------------
size_t vtkLZMADataCompressor::CompressBuffer(unsigned char const* uncompressedData,
  size_t uncompressedSize, unsigned char* compressedData, size_t compressionSpace)
{
  size_t out_pos = 0;
  lzma_ret lzma_ret_ = lzma_easy_buffer_encode(static_cast<uint32_t>(this->CompressionLevel),
    LZMA_CHECK_CRC32, nullptr, reinterpret_cast<const uint8_t*>(uncompressedData), uncompressedSize,
    reinterpret_cast<uint8_t*>(compressedData), &out_pos, compressionSpace);
  switch (lzma_ret_)
  {
    case LZMA_OK:
      break;
    case LZMA_MEM_ERROR:
      vtkErrorMacro("Memory allocation failed.");
      break;
    case LZMA_OPTIONS_ERROR:
      vtkErrorMacro("Specified preset is not supported: " << this->CompressionLevel);
      break;
    case LZMA_UNSUPPORTED_CHECK:
      vtkErrorMacro("Specified integrity check is not supported.");
      break;
    case LZMA_STREAM_END:
    case LZMA_NO_CHECK:
    case LZMA_MEMLIMIT_ERROR:
    case LZMA_FORMAT_ERROR:
    case LZMA_DATA_ERROR:
    case LZMA_BUF_ERROR:
    case LZMA_PROG_ERROR:
    case LZMA_GET_CHECK:
    default:
      vtkErrorMacro("Unknown error.");
  }

  return static_cast<size_t>(out_pos);
}

//----------------------------------------------------------------------------
size_t vtkLZMADataCompressor::UncompressBuffer(unsigned char const* compressedData,
  size_t compressedSize, unsigned char* uncompressedData, size_t uncompressedSize)
{
  size_t in_pos = 0;
  size_t out_pos = 0;
  uint64_t memlim = UINT64_MAX;
  lzma_ret lzma_ret_ =
    lzma_stream_buffer_decode(reinterpret_cast<uint64_t*>(&memlim), // No memory limit
      static_cast<uint32_t>(0),                                     // Don't use any decoder flags
      nullptr, // Use default allocators (malloc/free)
      reinterpret_cast<const uint8_t*>(compressedData), &in_pos, compressedSize,
      reinterpret_cast<uint8_t*>(uncompressedData), &out_pos, uncompressedSize);
  switch (lzma_ret_)
  {
    case LZMA_OK:
      break;
    case LZMA_MEM_ERROR:
      vtkErrorMacro("Memory allocation failed.");
      break;
    case LZMA_OPTIONS_ERROR:
      vtkErrorMacro("Specified preset is not supported.");
      break;
    case LZMA_UNSUPPORTED_CHECK:
      vtkErrorMacro("Specified integrity check is not supported.");
      break;
    case LZMA_DATA_ERROR:
      vtkErrorMacro("LZMA Data error.");
      break;
    case LZMA_NO_CHECK:
      vtkErrorMacro("LZMA_TELL_UNSUPPORTED_CHECK flag is set..");
      break;
    case LZMA_MEMLIMIT_ERROR:
      vtkErrorMacro("Memory usage limit was reached: " << memlim << " bytes");
      break;
    case LZMA_BUF_ERROR:
      vtkErrorMacro("LZMA output buffer was too small.");
      break;
    case LZMA_PROG_ERROR:
      vtkErrorMacro("LZMA program error.");
      break;
    case LZMA_STREAM_END:
    case LZMA_GET_CHECK:
    case LZMA_FORMAT_ERROR:
    default:
      vtkErrorMacro("Unknown error.");
  }

  return static_cast<size_t>(out_pos);
}
//----------------------------------------------------------------------------
int vtkLZMADataCompressor::GetCompressionLevel()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning CompressionLevel "
                << this->CompressionLevel);
  return this->CompressionLevel;
}
//----------------------------------------------------------------------------
void vtkLZMADataCompressor::SetCompressionLevel(int compressionLevel)
{
  int min = 1;
  int max = 9;
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting CompressionLevel to "
                << compressionLevel);
  if (this->CompressionLevel !=
    (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel)))
  {
    this->CompressionLevel =
      (compressionLevel < min ? min : (compressionLevel > max ? max : compressionLevel));
    this->Modified();
  }
}

//----------------------------------------------------------------------------
size_t vtkLZMADataCompressor::GetMaximumCompressionSpace(size_t size)
{
  return static_cast<size_t>(size + (size >> 2) + 128);
}
