/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkTIFFReader,vtkImageReader);
  
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


