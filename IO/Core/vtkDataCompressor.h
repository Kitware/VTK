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
/**
 * @class   vtkDataCompressor
 * @brief   Abstract interface for data compression classes.
 *
 * vtkDataCompressor provides a universal interface for data
 * compression.  Subclasses provide one compression method and one
 * decompression method.  The public interface to all compressors
 * remains the same, and is defined by this class.
 *
 * @par Note:
 * vtkDataCompressor CompressionLevel maye take on values 1 to 9. With
 * values of 1 giving best compression write performance, and a value of 9
 * giving best compression ratio. Subclasses of vtkDataCompressor objects
 * should be implemented with this in mind to provide a predictable
 * compressor interface for vtkDataCompressor users.
 *
 * @pat Thanks:
 * Homogeneous CompressionLevel behavior contributed by Quincy Wofford
 * (qwofford@lanl.gov) and John Patchett (patchett@lanl.gov)
 *
*/

#ifndef vtkDataCompressor_h
#define vtkDataCompressor_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkUnsignedCharArray;

class VTKIOCORE_EXPORT vtkDataCompressor : public vtkObject
{
public:
  vtkTypeMacro(vtkDataCompressor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the maximum space that may be needed to store data of the
   * given uncompressed size after compression.  This is the minimum
   * size of the output buffer that can be passed to the four-argument
   * Compress method.
   */
  virtual size_t GetMaximumCompressionSpace(size_t size)=0;

  /**
   * Compress the given input data buffer into the given output
   * buffer.  The size of the output buffer must be at least as large
   * as the value given by GetMaximumCompressionSpace for the given
   * input size.
   */
  size_t Compress(unsigned char const* uncompressedData,
                  size_t uncompressedSize,
                  unsigned char* compressedData,
                  size_t compressionSpace);

  /**
   * Uncompress the given input data into the given output buffer.
   * The size of the uncompressed data must be known by the caller.
   * It should be transmitted from the compressor by a means outside
   * of this class.
   */
  size_t Uncompress(unsigned char const* compressedData,
                    size_t compressedSize,
                    unsigned char* uncompressedData,
                    size_t uncompressedSize);

  /**
   * Compress the given data.  A vtkUnsignedCharArray containing the
   * compressed data is returned with a reference count of 1.
   */
  vtkUnsignedCharArray* Compress(unsigned char const* uncompressedData,
                                 size_t uncompressedSize);

  /**
   * Uncompress the given data.  A vtkUnsignedCharArray containing the
   * compressed data is returned with a reference count of 1.  The
   * size of the uncompressed data must be known by the caller.  It
   * should be transmitted from the compressor by a means outside of
   * this class.
   */
  vtkUnsignedCharArray* Uncompress(unsigned char const* compressedData,
                                   size_t compressedSize,
                                   size_t uncompressedSize);

  /** Compression performance varies greatly with compression level
   *  Require level setting from any vtkDataCompressor
   *  Different compressors handle performance parameters differently
   *  vtkDataCompressors should take a value between 1 and 9
   *  where 1 is fastest compression, and 9 is best compression.
   */
  virtual void SetCompressionLevel(int compressionLevel) = 0;
  virtual int GetCompressionLevel() = 0;

protected:
  vtkDataCompressor();
  ~vtkDataCompressor() override;

  // Actual compression method.  This must be provided by a subclass.
  // Must return the size of the compressed data, or zero on error.
  virtual size_t CompressBuffer(unsigned char const* uncompressedData,
                                size_t uncompressedSize,
                                unsigned char* compressedData,
                                size_t compressionSpace)=0;
  // Actual decompression method.  This must be provided by a subclass.
  // Must return the size of the uncompressed data, or zero on error.
  virtual size_t UncompressBuffer(unsigned char const* compressedData,
                                  size_t compressedSize,
                                  unsigned char* uncompressedData,
                                  size_t uncompressedSize)=0;


private:
  vtkDataCompressor(const vtkDataCompressor&) = delete;
  void operator=(const vtkDataCompressor&) = delete;
};

#endif
