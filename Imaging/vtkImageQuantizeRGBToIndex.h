/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageQuantizeRGBToIndex.h
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
// .NAME vtkImageQuantizeRGBToIndex - generalized histograms up to 4 dimensions
// .SECTION Description
// vtkImageQuantizeRGBToIndex takes a 3 component RGB image as
// input and produces a one component index image as output, along with
// a lookup table that contains the color definitions for the index values.
// This filter works on the entire input extent - it does not perform
// streaming, and it does not supported threaded execution (because it has
// to process the entire image).
//
// To use this filter, you typically set the number of colors 
// (between 2 and 65536), execute it, and then retrieve the lookup table.
// The colors can then be using the lookup table and the image index.

#ifndef __vtkImageQuantizeRGBToIndex_h
#define __vtkImageQuantizeRGBToIndex_h

#include "vtkImageToImageFilter.h"
#include "vtkLookupTable.h"

class VTK_IMAGING_EXPORT vtkImageQuantizeRGBToIndex : public vtkImageToImageFilter
{
public:
  static vtkImageQuantizeRGBToIndex *New();
  vtkTypeMacro(vtkImageQuantizeRGBToIndex,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / Get the number of color index values to produce - must be 
  // a number between 2 and 65536.
  vtkSetClampMacro( NumberOfColors, int, 2, 65536 );
  vtkGetMacro( NumberOfColors, int );

  // Description:
  // Get the resulting lookup table that contains the color definitions
  // corresponding to the index values in the output image.
  vtkGetObjectMacro( LookupTable, vtkLookupTable );

  vtkGetMacro( InitializeExecuteTime, float );
  vtkGetMacro( BuildTreeExecuteTime, float );
  vtkGetMacro( LookupIndexExecuteTime, float );

//BTX
  // Description: 
  // For internal use only - get the type of the image
  vtkGetMacro( InputType, int );

  // Description: 
  // For internal use only - set the times for execution
  vtkSetMacro( InitializeExecuteTime, float );
  vtkSetMacro( BuildTreeExecuteTime, float );
  vtkSetMacro( LookupIndexExecuteTime, float );
//ETX

protected:
  vtkImageQuantizeRGBToIndex();
  ~vtkImageQuantizeRGBToIndex();

  vtkLookupTable  *LookupTable;
  int             NumberOfColors;
  int             InputType;

  float           InitializeExecuteTime;
  float           BuildTreeExecuteTime;
  float           LookupIndexExecuteTime;

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};

  virtual void ExecuteData(vtkDataObject *out);
private:
  vtkImageQuantizeRGBToIndex(const vtkImageQuantizeRGBToIndex&);  // Not implemented.
  void operator=(const vtkImageQuantizeRGBToIndex&);  // Not implemented.
};

#endif








