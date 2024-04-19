// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVolumeReader
 * @brief   read image files
 *
 * vtkVolumeReader is a source object that reads image files.
 *
 * VolumeReader creates structured point datasets. The dimension of the
 * dataset depends upon the number of files read. Reading a single file
 * results in a 2D image, while reading more than one file results in a
 * 3D volume.
 *
 * File names are created using FilePattern and FilePrefix as follows:
 * snprintf (filename, sizeof(filename), FilePattern, FilePrefix, number);
 * where number is in the range ImageRange[0] to ImageRange[1]. If
 * ImageRange[1] <= ImageRange[0], then slice number ImageRange[0] is
 * read. Thus to read an image set ImageRange[0] = ImageRange[1] = slice
 * number. The default behavior is to read a single file (i.e., image slice 1).
 *
 * The DataMask instance variable is used to read data files with embedded
 * connectivity or segmentation information. For example, some data has
 * the high order bit set to indicate connected surface. The DataMask allows
 * you to select this data. Other important ivars include HeaderSize, which
 * allows you to skip over initial info, and SwapBytes, which turns on/off
 * byte swapping. Consider using vtkImageReader as a replacement.
 *
 * @sa
 * vtkSliceCubes vtkMarchingCubes vtkPNMReader vtkVolume16Reader
 * vtkImageReader
 */

#ifndef vtkVolumeReader_h
#define vtkVolumeReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkVolumeReader : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkVolumeReader, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file prefix for the image file(s).
   */
  vtkSetFilePathMacro(FilePrefix);
  vtkGetFilePathMacro(FilePrefix);
  ///@}

  ///@{
  /**
   * The snprintf format used to build filename from FilePrefix and number.
   */
  vtkSetFilePathMacro(FilePattern);
  vtkGetFilePathMacro(FilePattern);
  ///@}

  ///@{
  /**
   * Set the range of files to read.
   */
  vtkSetVector2Macro(ImageRange, int);
  vtkGetVectorMacro(ImageRange, int, 2);
  ///@}

  ///@{
  /**
   * Specify the spacing for the data.
   */
  vtkSetVector3Macro(DataSpacing, double);
  vtkGetVectorMacro(DataSpacing, double, 3);
  ///@}

  ///@{
  /**
   * Specify the origin for the data.
   */
  vtkSetVector3Macro(DataOrigin, double);
  vtkGetVectorMacro(DataOrigin, double, 3);
  ///@}

  /**
   * Other objects make use of this method.
   */
  virtual vtkImageData* GetImage(int ImageNumber) = 0;

protected:
  vtkVolumeReader();
  ~vtkVolumeReader() override;

  char* FilePrefix;
  char* FilePattern;
  int ImageRange[2];
  double DataSpacing[3];
  double DataOrigin[3];

private:
  vtkVolumeReader(const vtkVolumeReader&) = delete;
  void operator=(const vtkVolumeReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
