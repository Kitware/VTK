/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPReader.h
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
// .NAME vtkBMPReader - read Windows BMP files
// .SECTION Description
// vtkBMPReader is a source object that reads Windows BMP files.
// This includes indexed and 24bit bitmaps
//
// BMPReader creates structured point datasets. The dimension of the 
// dataset depends upon the number of files read. Reading a single file 
// results in a 2D image, while reading more than one file results in a 
// 3D volume.
//
// To read a volume, files must be of the form "FileName.<number>"
// (e.g., foo.ppm.0, foo.ppm.1, ...). You must also specify the image 
// range. This range specifies the beginning and ending files to read (range
// can be any pair of non-negative numbers). 
//
// The default behavior is to read a single file. In this case, the form
// of the file is simply "FileName" (e.g., foo.bar, foo.ppm, foo.BMP). 

#ifndef __vtkBMPReader_h
#define __vtkBMPReader_h

#include <stdio.h>
#include "vtkImageReader.h"

class VTK_EXPORT vtkBMPReader : public vtkImageReader
{
public:
  vtkBMPReader();
  ~vtkBMPReader();
  static vtkBMPReader *New() {return new vtkBMPReader;};
  const char *GetClassName() {return "vtkBMPReader";};

  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkGetMacro(Depth,int);

  unsigned char *Colors;
  short Depth;
  
protected:
  virtual void ComputeDataIncrements();
  virtual void UpdateImageInformation();
  virtual void Execute(vtkImageData *out);
  
};

#endif


