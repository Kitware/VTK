/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

struct _vtkTifTag
{
  short TagId;
  short DataType;
  long  DataCount;
  long  DataOffset;
};

class VTK_EXPORT vtkTIFFReader : public vtkImageReader
{
public:
  static vtkTIFFReader *New() {return new vtkTIFFReader;};
  const char *GetClassName() {return "vtkTIFFReader";};
  
protected:
  virtual void UpdateImageInformation();
  void Swap4(long *stmp);
  void Swap2(short *stmp);
  void ReadTag(_vtkTifTag *tag, FILE *fp);
  long ReadTagLong(_vtkTifTag *tag, FILE *fp);
};

#endif


