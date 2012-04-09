/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkZLibDataCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkZLibDataCompressor.h"
#include "vtkObjectFactory.h"
#include "vtk_zlib.h"

vtkStandardNewMacro(vtkZLibDataCompressor);

//----------------------------------------------------------------------------
vtkZLibDataCompressor::vtkZLibDataCompressor()
{
  this->CompressionLevel = Z_DEFAULT_COMPRESSION;
}

//----------------------------------------------------------------------------
vtkZLibDataCompressor::~vtkZLibDataCompressor()
{ 
}

//----------------------------------------------------------------------------
void vtkZLibDataCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CompressionLevel: " << this->CompressionLevel << endl;
}

//----------------------------------------------------------------------------
unsigned long
vtkZLibDataCompressor::CompressBuffer(const unsigned char* uncompressedData,
                                      unsigned long uncompressedSize,
                                      unsigned char* compressedData,
                                      unsigned long compressionSpace)
{
  unsigned long compressedSize = compressionSpace;
  Bytef* cd = reinterpret_cast<Bytef*>(compressedData);
  const Bytef* ud = reinterpret_cast<const Bytef*>(uncompressedData);
  
  // Call zlib's compress function.
  if(compress2(cd, &compressedSize, ud, uncompressedSize, this->CompressionLevel) != Z_OK)
    {
    vtkErrorMacro("Zlib error while compressing data.");
    return 0;
    }
  
  return compressedSize;
}

//----------------------------------------------------------------------------
unsigned long
vtkZLibDataCompressor::UncompressBuffer(const unsigned char* compressedData,
                                        unsigned long compressedSize,
                                        unsigned char* uncompressedData,
                                        unsigned long uncompressedSize)
{  
  unsigned long decSize = uncompressedSize;
  Bytef* ud = reinterpret_cast<Bytef*>(uncompressedData);
  const Bytef* cd = reinterpret_cast<const Bytef*>(compressedData);
  
  // Call zlib's uncompress function.
  if(uncompress(ud, &decSize, cd, compressedSize) != Z_OK)
    {    
    vtkErrorMacro("Zlib error while uncompressing data.");
    return 0;
    }
  
  // Make sure the output size matched that expected.
  if(decSize != uncompressedSize)
    {
    vtkErrorMacro("Decompression produced incorrect size.\n"
                  "Expected " << uncompressedSize << " and got " << decSize);
    return 0;
    }
  
  return decSize;
}

//----------------------------------------------------------------------------
unsigned long
vtkZLibDataCompressor::GetMaximumCompressionSpace(unsigned long size)
{
  // ZLib specifies that destination buffer must be 0.1% larger + 12 bytes.
  return size + (size+999)/1000 + 12;
}
