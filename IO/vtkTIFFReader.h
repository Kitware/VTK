/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTIFFReader - read TIFF files
// .SECTION Description
// vtkTIFFReader is a source object that reads TIFF files. This object
// only supports reading a subset of the TIFF formats. Specifically,
// it will not read LZ compressed TIFFs.
//
// TIFFReader creates structured point datasets. The dimension of the 
// dataset depends upon the number of files read. Reading a single file 
// results in a 2D image, while reading more than one file results in a 
// 3D volume.
//
// To read a volume, files must be of the form "FileName.<number>"
// (e.g., foo.tiff.0, foo.tiff.1, ...). You must also specify the image 
// range. This range specifies the beginning and ending files to read.
// The range is specified by setting fifth and sixth values of the 
// DataExtent.

#ifndef __vtkTIFFReader_h
#define __vtkTIFFReader_h

#include <stdio.h>
#include "vtkImageReader.h"

//BTX
#if (_MIPS_SZLONG == 64)
typedef int vtkTiffLong;
typedef unsigned int vtkTiffUnsignedLong;
#else
typedef long vtkTiffLong;
typedef unsigned long vtkTiffUnsignedLong;
#endif

struct _vtkTifTag
{
  short TagId;
  short DataType;
  vtkTiffLong  DataCount;
  vtkTiffLong  DataOffset;
};
//ETX
class VTK_IO_EXPORT vtkTIFFReader : public vtkImageReader
{
public:
  static vtkTIFFReader *New();
  vtkTypeRevisionMacro(vtkTIFFReader,vtkImageReader);
  
protected:
  vtkTIFFReader() {};
  ~vtkTIFFReader() {};

  virtual void ExecuteInformation();

  //BTX
#if (!(_MIPS_SZLONG == 64))
  void Swap4(int *stmp);
#endif
  void Swap4(vtkTiffLong *stmp);
  void Swap2(short *stmp);
  void ReadTag(_vtkTifTag *tag, FILE *fp);
  vtkTiffLong ReadTagLong(_vtkTifTag *tag, FILE *fp);
  //ETX
private:
  vtkTIFFReader(const vtkTIFFReader&);  // Not implemented.
  void operator=(const vtkTIFFReader&);  // Not implemented.
};

#endif


