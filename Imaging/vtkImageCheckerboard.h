/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCheckerboard.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    This work was supported by PHS Research Grant No. 1 P41 RR13218-01
             from the National Center for Research Resources

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
// .NAME vtkImageCheckerboard - show two images at once using a checkboard pattern
// .SECTION Description
//  vtkImageCheckerboard displays two images as one using a checkerboard
//  pattern. This filter can be used to compare two images. The
//  checkerboard pattern is controlled by the NumberOfDivisions
//  ivar. This controls the number of checkerboard divisions in the whole
//  extent of the image.

#ifndef __vtkImageCheckerboard_h
#define __vtkImageCheckerboard_h

#include "vtkImageTwoInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageCheckerboard : public vtkImageTwoInputFilter
{
public:
  static vtkImageCheckerboard *New();
  vtkTypeMacro(vtkImageCheckerboard,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the number of divisions along each axis.
  vtkSetVector3Macro(NumberOfDivisions,int);
  vtkGetVectorMacro(NumberOfDivisions,int,3);

protected:
  vtkImageCheckerboard();
  ~vtkImageCheckerboard() {};

  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);
  int NumberOfDivisions[3];
private:
  vtkImageCheckerboard(const vtkImageCheckerboard&) {};  // Not implemented.
  void operator=(const vtkImageCheckerboard&) {};  // Not implemented.
};

#endif













