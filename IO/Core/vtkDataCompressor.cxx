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
unsigned long
vtkDataCompressor::Compress(const unsigned char* uncompressedData,
                            unsigned long uncompressedSize,
                            unsigned char* compressedData,
                            unsigned long compressionSpace)
{
  return this->CompressBuffer(uncompressedData, uncompressedSize,
                              compressedData, compressionSpace);
}

//----------------------------------------------------------------------------
unsigned long
vtkDataCompressor::Uncompress(const unsigned char* compressedData,
                              unsigned long compressedSize,
                              unsigned char* uncompressedData,
                              unsigned long uncompressedSize)
{
  return this->UncompressBuffer(compressedData, compressedSize,
                                uncompressedData, uncompressedSize);
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray*
vtkDataCompressor::Compress(const unsigned char* uncompressedData,
                            unsigned long uncompressedSize)
{
  // Get the amount of space needed for compressed data.
  unsigned long compressionSpace =
    this->GetMaximumCompressionSpace(uncompressedSize);
  
  // Allocate a buffer.
  vtkUnsignedCharArray* outputArray = vtkUnsignedCharArray::New();
  outputArray->SetNumberOfComponents(1);
  outputArray->SetNumberOfTuples(compressionSpace);
  unsigned char* compressedData = outputArray->GetPointer(0);
  
  // Compress the data.
  unsigned long compressedSize =
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
vtkDataCompressor::Uncompress(const unsigned char* compressedData,
                              unsigned long compressedSize,
                              unsigned long uncompressedSize)
{
  // Allocate a buffer.
  vtkUnsignedCharArray* outputArray = vtkUnsignedCharArray::New();
  outputArray->SetNumberOfComponents(1);
  outputArray->SetNumberOfTuples(uncompressedSize);
  unsigned char* uncompressedData = outputArray->GetPointer(0);
  
  // Decompress the data.
  unsigned long decSize =
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
