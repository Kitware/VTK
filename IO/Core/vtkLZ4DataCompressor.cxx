/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLZ4DataCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLZ4DataCompressor.h"
#include "vtkObjectFactory.h"
#include "vtk_lz4.h"

vtkStandardNewMacro(vtkLZ4DataCompressor);

//----------------------------------------------------------------------------
vtkLZ4DataCompressor::vtkLZ4DataCompressor()
{
  this->AccelerationLevel = 1;
}

//----------------------------------------------------------------------------
vtkLZ4DataCompressor::~vtkLZ4DataCompressor()
{
}

//----------------------------------------------------------------------------
void vtkLZ4DataCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AccelerationLevel: " << this->AccelerationLevel << endl;
}

//----------------------------------------------------------------------------
size_t
vtkLZ4DataCompressor::CompressBuffer(unsigned char const* uncompressedData,
                                      size_t uncompressedSize,
                                      unsigned char* compressedData,
                                      size_t compressionSpace)
{
  const char *ud = reinterpret_cast<const char *>(uncompressedData);
  char *cd = reinterpret_cast<char*>(compressedData);
  // Call LZ4's compress function.
  int cs =
    LZ4_compress_fast(ud, cd,
      static_cast<int>(uncompressedSize),
      static_cast<int>(compressionSpace), this->AccelerationLevel);
  if (cs == 0)
  {
    vtkErrorMacro("LZ4 error while compressing data.");
  }
  return static_cast<size_t>(cs);
}

//----------------------------------------------------------------------------
size_t
vtkLZ4DataCompressor::UncompressBuffer(unsigned char const* compressedData,
                                        size_t compressedSize,
                                        unsigned char* uncompressedData,
                                        size_t uncompressedSize)
{
  char* ud = reinterpret_cast<char*>(uncompressedData);
  const char* cd = reinterpret_cast<const char*>(compressedData);
  int us =
    LZ4_decompress_safe(cd,ud,
      static_cast<int>(compressedSize),
      static_cast<int>(uncompressedSize));
  if (us < 0)
  {
    vtkErrorMacro("Zlib error while uncompressing data.");
    return 0;
  }
  // Make sure the output size matched that expected.
  if(us != static_cast<int>(uncompressedSize))
  {
    vtkErrorMacro("Decompression produced incorrect size.\n"
                  "Expected " << uncompressedSize << " and got " << us);
    return 0;
  }
  return static_cast<size_t>(us);
}

//----------------------------------------------------------------------------
size_t
vtkLZ4DataCompressor::GetMaximumCompressionSpace(size_t size)
{
  return LZ4_COMPRESSBOUND(size);
}
