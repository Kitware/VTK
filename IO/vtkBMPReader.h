/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPReader.h
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

// .SECTION See Also
// vtkBMPWriter

#ifndef __vtkBMPReader_h
#define __vtkBMPReader_h

#include <stdio.h>
#include "vtkImageReader.h"

class VTK_IO_EXPORT vtkBMPReader : public vtkImageReader
{
public:
  static vtkBMPReader *New();
  vtkTypeMacro(vtkBMPReader,vtkImageReader);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the depth of the BMP, either 8 or 24.
  vtkGetMacro(Depth,int);

//BTX
  // Description:
  // Returns the color lut.
  vtkGetMacro(Colors,unsigned char *);
//ETX

protected:
  vtkBMPReader();
  ~vtkBMPReader();
  vtkBMPReader(const vtkBMPReader&);
  void operator=(const vtkBMPReader&);

  unsigned char *Colors;
  short Depth;
  
  virtual void ComputeDataIncrements();
  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);
};
#endif


