/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSinusoidSource.h
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
// .NAME vtkImageSinusoidSource - Create an image with sinusoidal pixel values.
// .SECTION Description
// vtkImageSinusoidSource just produces images with pixel values determined 
// by a sinusoid.


#ifndef __vtkImageSinusoidSource_h
#define __vtkImageSinusoidSource_h

#include "vtkImageSource.h"

class VTK_IMAGING_EXPORT vtkImageSinusoidSource : public vtkImageSource
{
public:
  static vtkImageSinusoidSource *New();
  vtkTypeMacro(vtkImageSinusoidSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
		      int zMin, int zMax);
  
  // Description:
  // Set/Get the direction vector which determines the sinusoidal
  // orientation. The magnitude is ignored.
  void SetDirection(float,float,float);
  void SetDirection(float dir[3]);
  vtkGetVector3Macro(Direction, float);
  
  // Description:
  // Set/Get the period of the sinusoid in pixels.
  vtkSetMacro(Period, float);
  vtkGetMacro(Period, float);

  // Description:
  // Set/Get the phase: 0->2Pi.  0 => Cosine, pi/2 => Sine.
  vtkSetMacro(Phase, float);
  vtkGetMacro(Phase, float);

  // Description:
  // Set/Get the magnitude of the sinusoid.
  vtkSetMacro(Amplitude, float);
  vtkGetMacro(Amplitude, float);

protected:
  vtkImageSinusoidSource();
  ~vtkImageSinusoidSource() {};

  int WholeExtent[6];
  float Direction[3];
  float Period;
  float Phase;
  float Amplitude;

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *data);
private:
  vtkImageSinusoidSource(const vtkImageSinusoidSource&);  // Not implemented.
  void operator=(const vtkImageSinusoidSource&);  // Not implemented.
};


#endif



