// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMetaImageWriter
 * @brief   write a binary UNC meta image data
 *
 * One of the formats for which a reader is already available in the toolkit is
 * the MetaImage file format. This is a fairly simple yet powerful format
 * consisting of a text header and a binary data section. The following
 * instructions describe how you can write a MetaImage header for the data that
 * you download from the BrainWeb page.
 *
 * The minimal structure of the MetaImage header is the following:
 *
 *    NDims = 3
 *    DimSize = 181 217 181
 *    ElementType = MET_UCHAR
 *    ElementSpacing = 1.0 1.0 1.0
 *    ElementByteOrderMSB = False
 *    ElementDataFile = brainweb1.raw
 *
 *    * NDims indicate that this is a 3D image. ITK can handle images of
 *      arbitrary dimension.
 *    * DimSize indicates the size of the volume in pixels along each
 *      direction.
 *    * ElementType indicate the primitive type used for pixels. In this case
 *      is "unsigned char", implying that the data is digitized in 8 bits /
 *      pixel.
 *    * ElementSpacing indicates the physical separation between the center of
 *      one pixel and the center of the next pixel along each direction in space.
 *      The units used are millimeters.
 *    * ElementByteOrderMSB indicates is the data is encoded in little or big
 *      endian order. You might want to play with this value when moving data
 *      between different computer platforms.
 *    * ElementDataFile is the name of the file containing the raw binary data
 *      of the image. This file must be in the same directory as the header.
 *
 * MetaImage headers are expected to have extension: ".mha" or ".mhd"
 *
 * Once you write this header text file, it should be possible to read the
 * image into your ITK based application using the itk::FileIOToImageFilter
 * class.
 *
 *
 *
 * @sa
 * vtkImageWriter vtkMetaImageReader
 */

#ifndef vtkMetaImageWriter_h
#define vtkMetaImageWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

namespace vtkmetaio
{
class MetaImage;
} // forward declaration

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkMetaImageWriter : public vtkImageWriter
{
public:
  vtkTypeMacro(vtkMetaImageWriter, vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with FlipNormals turned off and Normals set to true.
   */
  static vtkMetaImageWriter* New();

  /**
   * Specify file name of meta file
   */
  void SetFileName(VTK_FILEPATH const char* fname) override;
  VTK_FILEPATH VTK_FUTURE_CONST char* GetFileName() VTK_FUTURE_CONST override
  {
    return this->MHDFileName;
  }

  ///@{
  /**
   * Specify the file name of the raw image data.
   */
  virtual void SetRAWFileName(VTK_FILEPATH const char* fname);
  virtual VTK_FILEPATH VTK_FUTURE_CONST char* GetRAWFileName() VTK_FUTURE_CONST;
  ///@}

  virtual void SetCompression(bool compress) { this->Compress = compress; }
  virtual bool GetCompression() { return this->Compress; }

  // This is called by the superclass.
  // This is the method you should override.
  void Write() override;

protected:
  vtkMetaImageWriter();
  ~vtkMetaImageWriter() override;

  vtkSetFilePathMacro(MHDFileName);
  char* MHDFileName;
  bool Compress;

private:
  vtkMetaImageWriter(const vtkMetaImageWriter&) = delete;
  void operator=(const vtkMetaImageWriter&) = delete;

  vtkmetaio::MetaImage* MetaImagePtr;
};

VTK_ABI_NAMESPACE_END
#endif
