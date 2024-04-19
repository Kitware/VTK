// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZLibDataCompressor
 * @brief   Data compression using zlib.
 *
 * vtkZLibDataCompressor provides a concrete vtkDataCompressor class
 * using zlib for compressing and uncompressing data.
 */

#ifndef vtkZLibDataCompressor_h
#define vtkZLibDataCompressor_h

#include "vtkDataCompressor.h"
#include "vtkIOCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCORE_EXPORT vtkZLibDataCompressor : public vtkDataCompressor
{
public:
  vtkTypeMacro(vtkZLibDataCompressor, vtkDataCompressor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkZLibDataCompressor* New();

  /**
   * Get the maximum space that may be needed to store data of the
   * given uncompressed size after compression.  This is the minimum
   * size of the output buffer that can be passed to the four-argument
   * Compress method.
   */
  size_t GetMaximumCompressionSpace(size_t size) override;

  ///@{
  /**
   *  Get/Set the compression level.
   */
  // Compression level getter required by vtkDataCompressor.
  int GetCompressionLevel() override;

  // Compression level setter required by vtkDataCompresor.
  void SetCompressionLevel(int compressionLevel) override;
  ///@}

protected:
  vtkZLibDataCompressor();
  ~vtkZLibDataCompressor() override;

  int CompressionLevel;

  // Compression method required by vtkDataCompressor.
  size_t CompressBuffer(unsigned char const* uncompressedData, size_t uncompressedSize,
    unsigned char* compressedData, size_t compressionSpace) override;
  // Decompression method required by vtkDataCompressor.
  size_t UncompressBuffer(unsigned char const* compressedData, size_t compressedSize,
    unsigned char* uncompressedData, size_t uncompressedSize) override;

private:
  vtkZLibDataCompressor(const vtkZLibDataCompressor&) = delete;
  void operator=(const vtkZLibDataCompressor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
