/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppend.h
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
// .NAME vtkImageAppend - Collects data from multiple inputs into one image.
// .SECTION Description
// vtkImageAppend takes the components from multiple inputs and merges
// them into one output. The output images are append along the "AppendAxis".
// Except for the append axis, all inputs must have the same extent.  
// All inputs must have the same number of scalar components.  
// A future extension might be to pad or clip inputs to have the same extent.
// The output has the same origin and spacing as the first input.
// The origin and spacing of all other inputs are ignored.  All inputs
// must have the same scalar type.


#ifndef __vtkImageAppend_h
#define __vtkImageAppend_h


#include "vtkImageMultipleInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageAppend : public vtkImageMultipleInputFilter
{
public:
  static vtkImageAppend *New();
  vtkTypeMacro(vtkImageAppend,vtkImageMultipleInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This axis is expanded to hold the multiple images.
  vtkSetMacro(AppendAxis, int);
  vtkGetMacro(AppendAxis, int);
  
  // Description:
  // By default "PreserveExtents" is off and the append axis is used.  
  // When "PreseveExtents" is on, the extent of the inputs is used to 
  // place the image in the output.  The whole extent of the output is 
  // the union of the input whole extents.  Any portion of the 
  // output not covered by the inputs is set to zero.  The origin and 
  // spacing is taken from the first input.
  vtkSetMacro(PreserveExtents, int);
  vtkGetMacro(PreserveExtents, int);
  vtkBooleanMacro(PreserveExtents, int);

protected:
  vtkImageAppend();
  ~vtkImageAppend();
  vtkImageAppend(const vtkImageAppend&);
  void operator=(const vtkImageAppend&);

  int PreserveExtents;
  int AppendAxis;
  // Array holds the AppendAxisExtent shift for each input.
  int *Shifts;

  void ExecuteInformation(vtkImageData **inputs, vtkImageData *output);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int whichInput);
  void ExecuteInformation(){this->vtkImageMultipleInputFilter::ExecuteInformation();};
  
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);

  void InitOutput(int outExt[6], vtkImageData *outData);
};

#endif




