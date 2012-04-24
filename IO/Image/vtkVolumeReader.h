/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVolumeReader - read image files
// .SECTION Description
// vtkVolumeReader is a source object that reads image files.
//
// VolumeReader creates structured point datasets. The dimension of the
// dataset depends upon the number of files read. Reading a single file
// results in a 2D image, while reading more than one file results in a
// 3D volume.
//
// File names are created using FilePattern and FilePrefix as follows:
// sprintf (filename, FilePattern, FilePrefix, number);
// where number is in the range ImageRange[0] to ImageRange[1]. If
// ImageRange[1] <= ImageRange[0], then slice number ImageRange[0] is
// read. Thus to read an image set ImageRange[0] = ImageRange[1] = slice
// number. The default behavior is to read a single file (i.e., image slice 1).
//
// The DataMask instance variable is used to read data files with imbedded
// connectivity or segmentation information. For example, some data has
// the high order bit set to indicate connected surface. The DataMask allows
// you to select this data. Other important ivars include HeaderSize, which
// allows you to skip over initial info, and SwapBytes, which turns on/off
// byte swapping. Consider using vtkImageReader as a replacement.

// .SECTION See Also
// vtkSliceCubes vtkMarchingCubes vtkPNMReader vtkVolume16Reader
// vtkImageReader

#ifndef __vtkVolumeReader_h
#define __vtkVolumeReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIOIMAGE_EXPORT vtkVolumeReader : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkVolumeReader,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file prefix for the image file(s).
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);

  // Description:
  // The sprintf format used to build filename from FilePrefix and number.
  vtkSetStringMacro(FilePattern);
  vtkGetStringMacro(FilePattern);

  // Description:
  // Set the range of files to read.
  vtkSetVector2Macro(ImageRange,int);
  vtkGetVectorMacro(ImageRange,int,2);

  // Description:
  // Specify the spacing for the data.
  vtkSetVector3Macro(DataSpacing,double);
  vtkGetVectorMacro(DataSpacing,double,3);

  // Description:
  // Specify the origin for the data.
  vtkSetVector3Macro(DataOrigin,double);
  vtkGetVectorMacro(DataOrigin,double,3);

  // Description:
  // Other objects make use of this method.
  virtual vtkImageData *GetImage(int ImageNumber) = 0;

protected:
  vtkVolumeReader();
  ~vtkVolumeReader();

  char *FilePrefix;
  char *FilePattern;
  int ImageRange[2];
  double DataSpacing[3];
  double DataOrigin[3];
private:
  vtkVolumeReader(const vtkVolumeReader&);  // Not implemented.
  void operator=(const vtkVolumeReader&);  // Not implemented.
};

#endif


