/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMandelbrotSource.h
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
// .NAME vtkImageMandelbrotSource - Mandelbrot image.
// .SECTION Description
// vtkImageMandelbrotSource creates an unsigned char image of the Mandelbrot
// set.  The values in the image are the number of iterations it takes for
// the magnitude of the value to get over 2.  The equation repeated is
// z = z^2 + C (z and C are complex).  Initial value of z is zero, and the 
// real value of C is mapped onto the x axis, and the imaginary value of C
// is mapped onto the Y Axis.  I was thinking of extending this source
// to generate Julia Sets (initial value of Z varies).  This would be 4
// possible parameters to vary, but there are no more 4d images :(
// The third dimension (z axis) is the imaginary value of the initial value.

#ifndef __vtkImageMandelbrotSource_h
#define __vtkImageMandelbrotSource_h

#include "vtkImageSource.h"

class VTK_EXPORT vtkImageMandelbrotSource : public vtkImageSource
{
public:
  static vtkImageMandelbrotSource *New();
  vtkTypeMacro(vtkImageMandelbrotSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  // Description:
  // Set/Get the extent of the whole output Volume.
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int minX, int maxX, int minY, int maxY, 
			    int minZ, int maxZ);
  vtkGetVector6Macro(WholeExtent,int);
  
  // Description:
  // Set the projection from  the 4D space (4 parameters / 2 imaginary numbers)
  // to the axes of the 3D Volume. 
  // 0=C_Real, 1=C_Imaginary, 2=X_Real, 4=X_Imaginary
  vtkSetVector3Macro(ProjectionAxes, int);
  vtkGetVector3Macro(ProjectionAxes, int);

  // Description:
  // Imaginary and real value for C (constant in equation) 
  // and X (initial value).
  vtkSetVector4Macro(OriginCX, double);
  //void SetOriginCX(double cReal, double cImag, double xReal, double xImag);
  vtkGetVector4Macro(OriginCX, double);

  // Description:
  // Imaginary and real value for C (constant in equation) 
  // and X (initial value).
  vtkSetVector4Macro(SampleCX, double);
  //void SetOriginCX(double cReal, double cImag, double xReal, double xImag);
  vtkGetVector4Macro(SampleCX, double);

  // Description:
  // The maximum number of cycles run to see if the value goes over 2
  vtkSetClampMacro(MaximumNumberOfIterations, unsigned short, 1, 5000);
  vtkGetMacro(MaximumNumberOfIterations, unsigned short);

  // Description:
  // Convienence for Viewer.  Pan 3D volume relative to spacing. 
  // Zoom constant factor.
  void Zoom(double factor);
  void Pan(double x, double y, double z);

  // Description:
  // Convienence for Viewer.  Copy the OriginCX and the SpacingCX.
  // What about other parameters ???
  void CopyOriginAndSample(vtkImageMandelbrotSource *source); 

#ifndef VTK_REMOVE_LEGACY_CODE
  // Description:
  // Convienence/Legacy - set all the spacing values the same.
  void SetSample(double v) 
    {VTK_LEGACY_METHOD(SetSampleCX,"3.2"); this->SetSampleCX(v, v, v, v);}
#endif
  
protected:
  vtkImageMandelbrotSource();
  ~vtkImageMandelbrotSource();
  vtkImageMandelbrotSource(const vtkImageMandelbrotSource&) {};
  void operator=(const vtkImageMandelbrotSource&) {};

  int ProjectionAxes[3];

  // WholeExtent in 3 space (after projection).
  int WholeExtent[6];

  // Complex constant/initial-value at origin.
  double OriginCX[4];
  // Initial complex value at origin.
  double SampleCX[4];
  unsigned short MaximumNumberOfIterations;

  virtual void ExecuteData(vtkDataObject *outData);
  virtual void ExecuteInformation();
  float EvaluateSet(double p[4]);
};


#endif


