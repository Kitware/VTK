// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLZ4DataCompressor
 * @brief   Data compression using LZ4.
 *
 * vtkLZ4DataCompressor provides a concrete vtkDataCompressor class
 * using LZ4 for compressing and uncompressing data.
 */

#ifndef vtkLZ4DataCompressor_h
#define vtkLZ4DataCompressor_h

#include "vtkDataCompressor.h"
#include "vtkIOCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCORE_EXPORT vtkLZ4DataCompressor : public vtkDataCompressor
{
public:
  vtkTypeMacro(vtkLZ4DataCompressor, vtkDataCompressor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkLZ4DataCompressor* New();

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
  // Compression level getter required by vtkDataCompressor.
  int GetCompressionLevel() override;

  // Compression level setter required by vtkDataCompresor.
  void SetCompressionLevel(int compressionLevel) override;

  // Direct setting of AccelerationLevel allows more direct
  // control over LZ4 compressor
  vtkSetClampMacro(AccelerationLevel, int, 1, VTK_INT_MAX);
  vtkGetMacro(AccelerationLevel, int);

protected:
  vtkLZ4DataCompressor();
  ~vtkLZ4DataCompressor() override;

  int AccelerationLevel;

  // Compression method required by vtkDataCompressor.
  size_t CompressBuffer(unsigned char const* uncompressedData, size_t uncompressedSize,
    unsigned char* compressedData, size_t compressionSpace) override;
  // Decompression method required by vtkDataCompressor.
  size_t UncompressBuffer(unsigned char const* compressedData, size_t compressedSize,
    unsigned char* uncompressedData, size_t uncompressedSize) override;

private:
  vtkLZ4DataCompressor(const vtkLZ4DataCompressor&) = delete;
  void operator=(const vtkLZ4DataCompressor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
