/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOpenClose3D.h
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
// .NAME vtkImageOpenClose3D - Will perform opening or closing.
// .SECTION Description
// vtkImageOpenClose3D performs opening or closing by having two 
// vtkImageErodeDilates in series.  The size of operation
// is determined by the method SetKernelSize, and the operator is an ellipse.
// OpenValue and CloseValue determine how the filter behaves.  For binary
// images Opening and closing behaves as expected.
// Close value is first dilated, and then eroded.
// Open value is first eroded, and then dilated.
// Degenerate two dimensional opening/closing can be achieved by setting the
// one axis the 3D KernelSize to 1.
// Values other than open value and close value are not touched.
// This enables the filter to processes segmented images containing more than
// two tags.


#ifndef __vtkImageOpenClose3D_h
#define __vtkImageOpenClose3D_h


#include "vtkImageToImageFilter.h"
#include "vtkImageDilateErode3D.h"

class VTK_IMAGING_EXPORT vtkImageOpenClose3D : public vtkImageToImageFilter
{
public:
  // Description:
  // Default open value is 0, and default close value is 255.
  static vtkImageOpenClose3D *New();
  vtkTypeMacro(vtkImageOpenClose3D,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method considers the sub filters MTimes when computing this objects
  // modified time.
  unsigned long int GetMTime();
  
  // Description:
  // Turn debugging output on. (in sub filters also)
  void DebugOn();
  void DebugOff();

  // Description:
  // Pass modified message to sub filters.
  void Modified();
  
  // Foward Source messages to filter1

  // Description:
  // This method returns the cache to make a connection
  // It justs feeds the request to the sub filter.
  vtkImageData *GetOutput();
  vtkImageData *GetOutput(int idx)
    {return (vtkImageData *) this->vtkImageSource::GetOutput(idx); };


  // Foward filter messages

  // Description:
  // Set the Input of the filter.
  void SetInput(vtkImageData *Input);

  // Forward dilateErode messages to both filters.

  // Description:
  // Selects the size of gaps or objects removed.
  void SetKernelSize(int size0, int size1, int size2);

  // Description:
  // Determines the value that will opened.  
  // Open value is first eroded, and then dilated.
  void SetOpenValue(float value);
  float GetOpenValue();

  // Description:
  // Determines the value that will closed.
  // Close value is first dilated, and then eroded
  void SetCloseValue(float value);
  float GetCloseValue();
  
  // Description:
  // Needed for Progress functions
  vtkGetObjectMacro(Filter0, vtkImageDilateErode3D);
  vtkGetObjectMacro(Filter1, vtkImageDilateErode3D);

protected:
  vtkImageOpenClose3D();
  ~vtkImageOpenClose3D();
  
  vtkImageDilateErode3D *Filter0;
  vtkImageDilateErode3D *Filter1;
private:
  vtkImageOpenClose3D(const vtkImageOpenClose3D&);  // Not implemented.
  void operator=(const vtkImageOpenClose3D&);  // Not implemented.
};

#endif



