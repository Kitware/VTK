/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageReader - Superclass of transformable binary file readers.
// .SECTION Description
// vtkImageReader provides methods needed to read a region from a file.
// It supports both transforms and masks on the input data, but as a result
// is more complicated and slower than its parent class vtkImageReader2.

// .SECTION See Also
// vtkBMPReader vtkPNMReader vtkTIFFReader

#ifndef __vtkImageReader_h
#define __vtkImageReader_h

#include "vtkImageReader2.h"
#include "vtkTransform.h"

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTK_IO_EXPORT vtkImageReader : public vtkImageReader2
{
public:
  static vtkImageReader *New();
  vtkTypeRevisionMacro(vtkImageReader,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Set/get the data VOI. You can limit the reader to only
  // read a subset of the data. 
  vtkSetVector6Macro(DataVOI,int);
  vtkGetVector6Macro(DataVOI,int);
  
  // Description:
  // Set/Get the Data mask.
  vtkGetMacro(DataMask,unsigned short);
  void SetDataMask(int val) 
       {if (val == this->DataMask) { return; }
        this->DataMask = ((unsigned short)(val)); this->Modified();}
  
  // Description:
  // Set/Get transformation matrix to transform the data from slice space
  // into world space. This matrix must be a permutation matrix. To qualify,
  // the sums of the rows must be + or - 1.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);

  // Warning !!!
  // following should only be used by methods or template helpers, not users
  void ComputeInverseTransformedExtent(int inExtent[6],
                                       int outExtent[6]);
  void ComputeInverseTransformedIncrements(int inIncr[3],
                                           int outIncr[3]);

  void OpenAndSeekFile(int extent[6], int slice);
protected:
  vtkImageReader();
  ~vtkImageReader();

  unsigned short DataMask;  // Mask each pixel with ...

  vtkTransform *Transform;

  void ComputeTransformedSpacing (float Spacing[3]);
  void ComputeTransformedOrigin (float origin[3]);
  void ComputeTransformedExtent(int inExtent[6],
                                int outExtent[6]);
  void ComputeTransformedIncrements(int inIncr[3],
                                    int outIncr[3]);

  int DataVOI[6];
  
  void ExecuteInformation();
  void ExecuteData(vtkDataObject *data);
private:
  vtkImageReader(const vtkImageReader&);  // Not implemented.
  void operator=(const vtkImageReader&);  // Not implemented.
};

#endif
