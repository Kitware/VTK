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
// .NAME vtkLZ4DataCompressor - Data compression using zlib.
// .SECTION Description
// vtkLZ4DataCompressor provides a concrete vtkDataCompressor class
// using LZ4 for compressing and uncompressing data.

#ifndef vtkLZ4DataCompressor_h
#define vtkLZ4DataCompressor_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkDataCompressor.h"

class VTKIOCORE_EXPORT vtkLZ4DataCompressor : public vtkDataCompressor
{
public:
  vtkTypeMacro(vtkLZ4DataCompressor,vtkDataCompressor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkLZ4DataCompressor* New();

  // Description:
  // Get the maximum space that may be needed to store data of the
  // given uncompressed size after compression.  This is the minimum
  // size of the output buffer that can be passed to the four-argument
  // Compress method.
  size_t GetMaximumCompressionSpace(size_t size) VTK_OVERRIDE;

  // Description:
  // Get/Set the compression level.
  vtkSetClampMacro(AccelerationLevel, int, 1, VTK_INT_MAX);
  vtkGetMacro(AccelerationLevel, int);

protected:
  vtkLZ4DataCompressor();
  ~vtkLZ4DataCompressor() VTK_OVERRIDE;

  int AccelerationLevel;

  // Compression method required by vtkDataCompressor.
  size_t CompressBuffer(unsigned char const* uncompressedData,
                        size_t uncompressedSize,
                        unsigned char* compressedData,
                        size_t compressionSpace) VTK_OVERRIDE;
  // Decompression method required by vtkDataCompressor.
  size_t UncompressBuffer(unsigned char const* compressedData,
                          size_t compressedSize,
                          unsigned char* uncompressedData,
                          size_t uncompressedSize) VTK_OVERRIDE;
private:
  vtkLZ4DataCompressor(const vtkLZ4DataCompressor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLZ4DataCompressor&) VTK_DELETE_FUNCTION;
};

#endif
