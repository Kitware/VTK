/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkZLibDataCompressor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkZLibDataCompressor - Data compression using zlib.
// .SECTION Description
// vtkZLibDataCompressor provides a concrete vtkDataCompressor class
// using zlib for compressing and uncompressing data.

#ifndef __vtkZLibDataCompressor_h
#define __vtkZLibDataCompressor_h

#include "vtkDataCompressor.h"

class VTK_IO_EXPORT vtkZLibDataCompressor : public vtkDataCompressor
{
public:
  vtkTypeRevisionMacro(vtkZLibDataCompressor,vtkDataCompressor);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkZLibDataCompressor* New();  
  
  // Description:  
  // Get the maximum space that may be needed to store data of the
  // given uncompressed size after compression.  This is the minimum
  // size of the output buffer that can be passed to the four-argument
  // Compress method.
  unsigned long GetMaximumCompressionSpace(unsigned long size);
  
protected:
  vtkZLibDataCompressor();
  ~vtkZLibDataCompressor();
  
  // Compression method required by vtkDataCompressor.
  unsigned long CompressBuffer(const unsigned char* uncompressedData,
                               unsigned long uncompressedSize,
                               unsigned char* compressedData,
                               unsigned long compressionSpace);
  // Decompression method required by vtkDataCompressor.
  unsigned long UncompressBuffer(const unsigned char* compressedData,
                                 unsigned long compressedSize,
                                 unsigned char* uncompressedData,
                                 unsigned long uncompressedSize);
private:
  vtkZLibDataCompressor(const vtkZLibDataCompressor&);  // Not implemented.
  void operator=(const vtkZLibDataCompressor&);  // Not implemented.
};

#endif
