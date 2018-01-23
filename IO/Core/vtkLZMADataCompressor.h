/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLZMADataCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLZMADataCompressor
 * @brief   Data compression using LZMA Utils.
 *
 * vtkLZMADataCompressor provides a concrete vtkDataCompressor class
 * using LZMA for compressing and uncompressing data.
 *
 * @par Thanks:
 * This vtkDataCompressor contributed by Quincy Wofford (qwofford@lanl.gov)
 * and John Patchett (patchett@lanl.gov), Los Alamos National Laboratory
 * (2017)
 *
*/

#ifndef vtkLZMADataCompressor_h
#define vtkLZMADataCompressor_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkDataCompressor.h"


class VTKIOCORE_EXPORT vtkLZMADataCompressor : public vtkDataCompressor
{
public:
  vtkTypeMacro(vtkLZMADataCompressor,vtkDataCompressor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkLZMADataCompressor* New();

  /**
   *  Get the maximum space that may be needed to store data of the
   *  given uncompressed size after compression.  This is the minimum
   *  size of the output buffer that can be passed to the four-argument
   *  Compress method.
   */
  size_t GetMaximumCompressionSpace(size_t size) override;
  /**
   *  Get/Set the compression level.
   */

  // Compression level setter required by vtkDataCompressor.
  void SetCompressionLevel(int compressionLevel) override;

  // Compression level getter required by vtkDataCompressor.
  int  GetCompressionLevel() override;

protected:
  vtkLZMADataCompressor();
  ~vtkLZMADataCompressor() override;

  int CompressionLevel;

  // Compression method required by vtkDataCompressor.
  size_t CompressBuffer(unsigned char const* uncompressedData,
                        size_t uncompressedSize,
                        unsigned char* compressedData,
                        size_t compressionSpace) override;
  // Decompression method required by vtkDataCompressor.
  size_t UncompressBuffer(unsigned char const* compressedData,
                          size_t compressedSize,
                          unsigned char* uncompressedData,
                          size_t uncompressedSize) override;
private:
  vtkLZMADataCompressor(const vtkLZMADataCompressor&) = delete;
  void operator=(const vtkLZMADataCompressor&) = delete;
};

#endif
