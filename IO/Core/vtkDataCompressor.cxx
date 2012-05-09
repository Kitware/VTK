/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataCompressor.h"
#include "vtkUnsignedCharArray.h"


//----------------------------------------------------------------------------
vtkDataCompressor::vtkDataCompressor()
{
}

//----------------------------------------------------------------------------
vtkDataCompressor::~vtkDataCompressor()
{
}

//----------------------------------------------------------------------------
void vtkDataCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
size_t
vtkDataCompressor::Compress(unsigned char const* uncompressedData,
                            size_t uncompressedSize,
                            unsigned char* compressedData,
                            size_t compressionSpace)
{
  return this->CompressBuffer(uncompressedData, uncompressedSize,
                              compressedData, compressionSpace);
}

//----------------------------------------------------------------------------
size_t
vtkDataCompressor::Uncompress(unsigned char const* compressedData,
                              size_t compressedSize,
                              unsigned char* uncompressedData,
                              size_t uncompressedSize)
{
  return this->UncompressBuffer(compressedData, compressedSize,
                                uncompressedData, uncompressedSize);
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray*
vtkDataCompressor::Compress(unsigned char const* uncompressedData,
                            size_t uncompressedSize)
{
  // Get the amount of space needed for compressed data.
  size_t compressionSpace =
    this->GetMaximumCompressionSpace(uncompressedSize);

  // Allocate a buffer.
  vtkUnsignedCharArray* outputArray = vtkUnsignedCharArray::New();
  outputArray->SetNumberOfComponents(1);
  outputArray->SetNumberOfTuples(compressionSpace);
  unsigned char* compressedData = outputArray->GetPointer(0);

  // Compress the data.
  size_t compressedSize =
    this->CompressBuffer(uncompressedData, uncompressedSize,
                         compressedData, compressionSpace);

  // Make sure compression succeeded.
  if(!compressedSize)
    {
    outputArray->Delete();
    return 0;
    }

  // Store the actual size.
  outputArray->SetNumberOfTuples(compressedSize);

  return outputArray;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray*
vtkDataCompressor::Uncompress(unsigned char const* compressedData,
                              size_t compressedSize,
                              size_t uncompressedSize)
{
  // Allocate a buffer.
  vtkUnsignedCharArray* outputArray = vtkUnsignedCharArray::New();
  outputArray->SetNumberOfComponents(1);
  outputArray->SetNumberOfTuples(uncompressedSize);
  unsigned char* uncompressedData = outputArray->GetPointer(0);

  // Decompress the data.
  size_t decSize =
    this->UncompressBuffer(compressedData, compressedSize,
                           uncompressedData, uncompressedSize);

  // Make sure the decompression succeeded.
  if(!decSize)
    {
    outputArray->Delete();
    return 0;
    }

  // Store the actual size.
  outputArray->SetNumberOfTuples(decSize);

  return outputArray;
}
