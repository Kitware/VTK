/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReaderInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkTIFFReaderInternal
 * @brief
 *
 */

#ifndef vtkTIFFReaderInternal_h
#define vtkTIFFReaderInternal_h

extern "C"
{
#include "vtk_tiff.h"
}

class vtkTIFFReader::vtkTIFFReaderInternal
{
public:
  vtkTIFFReaderInternal();
  ~vtkTIFFReaderInternal() = default;

  bool Initialize();
  void Clean();
  bool CanRead();
  bool Open(const char* filename);
  TIFF* Image;
  bool IsOpen;
  unsigned int Width;
  unsigned int Height;
  unsigned short NumberOfPages;
  unsigned short CurrentPage;
  unsigned short SamplesPerPixel;
  unsigned short Compression;
  unsigned short BitsPerSample;
  unsigned short Photometrics;
  bool HasValidPhotometricInterpretation;
  unsigned short PlanarConfig;
  unsigned short Orientation;
  unsigned long int TileDepth;
  unsigned int TileRows;
  unsigned int TileColumns;
  unsigned int TileWidth;
  unsigned int TileHeight;
  unsigned short NumberOfTiles;
  unsigned int SubFiles;
  unsigned int ResolutionUnit;
  float XResolution;
  float YResolution;
  short SampleFormat;
  static void ErrorHandler(const char* module, const char* fmt, va_list ap);

private:
  vtkTIFFReaderInternal(const vtkTIFFReaderInternal&) = delete;
  void operator=(const vtkTIFFReaderInternal&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkTIFFReaderInternal.h
