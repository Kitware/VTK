/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkZLibDataCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkZLibDataCompressor
 * @brief   Data compression using zlib.
 *
 * vtkZLibDataCompressor provides a concrete vtkDataCompressor class
 * using zlib for compressing and uncompressing data.
*/

#ifndef vtkZLibDataCompressor_h
#define vtkZLibDataCompressor_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkDataCompressor.h"

class VTKIOCORE_EXPORT vtkZLibDataCompressor : public vtkDataCompressor
{
public:
  vtkTypeMacro(vtkZLibDataCompressor,vtkDataCompressor);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkZLibDataCompressor* New();

  /**
   * Get the maximum space that may be needed to store data of the
   * given uncompressed size after compression.  This is the minimum
   * size of the output buffer that can be passed to the four-argument
   * Compress method.
   */
  size_t GetMaximumCompressionSpace(size_t size);

  //@{
  /**
   * Get/Set the compression level.
   */
  vtkSetClampMacro(CompressionLevel, int, 0, 9);
  vtkGetMacro(CompressionLevel, int);
  //@}

protected:
  vtkZLibDataCompressor();
  ~vtkZLibDataCompressor();

  int CompressionLevel;

  // Compression method required by vtkDataCompressor.
  size_t CompressBuffer(unsigned char const* uncompressedData,
                        size_t uncompressedSize,
                        unsigned char* compressedData,
                        size_t compressionSpace);
  // Decompression method required by vtkDataCompressor.
  size_t UncompressBuffer(unsigned char const* compressedData,
                          size_t compressedSize,
                          unsigned char* uncompressedData,
                          size_t uncompressedSize);
private:
  vtkZLibDataCompressor(const vtkZLibDataCompressor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkZLibDataCompressor&) VTK_DELETE_FUNCTION;
};

#endif
