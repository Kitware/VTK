/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlend.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageBlend - blend images together using alpha or opacity
// .SECTION Description
// vtkImageBlend takes L, LA, RGB, or RGBA images as input and blends them 
// according to the alpha values and/or the opacity setting for each input.
// The blending rules are very similar to those for VTK texture maps.   
// The alpha value of the first input, if present, is copied to the alpha 
// value of the output.  The output always has the same number of components
// and the same extent as the first input.  

#ifndef __vtkImageBlend_h
#define __vtkImageBlend_h


#include "vtkImageMultipleInputFilter.h"

class VTK_EXPORT vtkImageBlend : public vtkImageMultipleInputFilter
{
public:
  static vtkImageBlend *New();
  vtkTypeMacro(vtkImageBlend,vtkImageMultipleInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the opacity of an input image: the alpha values of the image are
  // multiplied by the opacity.  The opacity of image idx=0 is ignored.
  void SetOpacity(int idx, double opacity);
  double GetOpacity(int idx);

  virtual void UpdateData(vtkDataObject *output);
  
protected:
  vtkImageBlend();
  ~vtkImageBlend();
  vtkImageBlend(const vtkImageBlend&) {};
  void operator=(const vtkImageBlend&) {};

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6],
				int whichInput);
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);


  double *Opacity;
  int OpacityArrayLength;
};

#endif




