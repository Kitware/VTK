/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataCompressor - Abstract interface for data compression classes.
// .SECTION Description
// vtkDataCompressor provides a universal interface for data
// compression.  Subclasses provide one compression method and one
// decompression method.  The public interface to all compressors
// remains the same, and is defined by this class.

#ifndef __vtkDataCompressor_h
#define __vtkDataCompressor_h

#include "vtkObject.h"

class vtkUnsignedCharArray;

class VTK_IO_EXPORT vtkDataCompressor : public vtkObject
{
public:
  vtkTypeMacro(vtkDataCompressor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:  
  // Get the maximum space that may be needed to store data of the
  // given uncompressed size after compression.  This is the minimum
  // size of the output buffer that can be passed to the four-argument
  // Compress method.
  virtual unsigned long GetMaximumCompressionSpace(unsigned long size)=0;
  
  // Description:
  // Compress the given input data buffer into the given output
  // buffer.  The size of the output buffer must be at least as large
  // as the value given by GetMaximumCompressionSpace for the given
  // input size.
  unsigned long Compress(const unsigned char* uncompressedData,
                         unsigned long uncompressedSize,
                         unsigned char* compressedData,
                         unsigned long compressionSpace);
  
  // Description:  
  // Uncompress the given input data into the given output buffer.
  // The size of the uncompressed data must be known by the caller.
  // It should be transmitted from the compressor by a means outside
  // of this class.
  unsigned long Uncompress(const unsigned char* compressedData,
                           unsigned long compressedSize,
                           unsigned char* uncompressedData,
                           unsigned long uncompressedSize);
  
  // Description:
  // Compress the given data.  A vtkUnsignedCharArray containing the
  // compressed data is returned with a reference count of 1.
  vtkUnsignedCharArray* Compress(const unsigned char* uncompressedData,
                                 unsigned long uncompressedSize);
  
  // Description:  
  // Uncompress the given data.  A vtkUnsignedCharArray containing the
  // compressed data is returned with a reference count of 1.  The
  // size of the uncompressed data must be known by the caller.  It
  // should be transmitted from the compressor by a means outside of
  // this class.
  vtkUnsignedCharArray* Uncompress(const unsigned char* compressedData,
                                   unsigned long compressedSize,
                                   unsigned long uncompressedSize);
protected:
  vtkDataCompressor();
  ~vtkDataCompressor();
  
  // Actual compression method.  This must be provided by a subclass.
  // Must return the size of the compressed data, or zero on error.
  virtual unsigned long CompressBuffer(const unsigned char* uncompressedData,
                                       unsigned long uncompressedSize,
                                       unsigned char* compressedData,
                                       unsigned long compressionSpace)=0;  
  // Actual decompression method.  This must be provided by a subclass.
  // Must return the size of the uncompressed data, or zero on error.
  virtual unsigned long UncompressBuffer(const unsigned char* compressedData,
                                         unsigned long compressedSize,
                                         unsigned char* uncompressedData,
                                         unsigned long uncompressedSize)=0;
private:
  vtkDataCompressor(const vtkDataCompressor&);  // Not implemented.
  void operator=(const vtkDataCompressor&);  // Not implemented.
};

#endif
