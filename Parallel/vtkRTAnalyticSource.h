/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRTAnalyticSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkRTAnalyticSource - Create an image for regression testing
// .SECTION Description
// vtkRTAnalyticSource just produces images with pixel values determined 
// by a Maximum*Gaussian*XMag*sin(XFreq*x)*sin(YFreq*y)*cos(ZFreq*z)


#ifndef __vtkRTAnalyticSource_h
#define __vtkRTAnalyticSource_h

#include "vtkImageSource.h"

class VTK_PARALLEL_EXPORT vtkRTAnalyticSource : public vtkImageSource
{
public:
  static vtkRTAnalyticSource *New();
  vtkTypeMacro(vtkRTAnalyticSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
		      int zMin, int zMax);
  vtkGetVector6Macro(WholeExtent, int);
  
  // Description:
  // Set/Get the center of function.
  vtkSetVector3Macro(Center, float);
  vtkGetVector3Macro(Center, float);

  // Description:
  // Set/Get the Maximum value of the function.
  vtkSetMacro(Maximum, float);
  vtkGetMacro(Maximum, float);

  // Description:
  // Set/Get the standard deviation of the function.
  vtkSetMacro(StandardDeviation, float);
  vtkGetMacro(StandardDeviation, float);

  // Description:
  // Set the natural frequencies in x,y and z
  vtkSetMacro(XFreq, float);
  vtkGetMacro(XFreq, float);
  vtkSetMacro(YFreq, float);
  vtkGetMacro(YFreq, float);
  vtkSetMacro(ZFreq, float);
  vtkGetMacro(ZFreq, float);

  vtkSetMacro(XMag, float);
  vtkGetMacro(XMag, float);
  vtkSetMacro(YMag, float);
  vtkGetMacro(YMag, float);
  vtkSetMacro(ZMag, float);
  vtkGetMacro(ZMag, float);

protected:
  vtkRTAnalyticSource();
  ~vtkRTAnalyticSource() {};

  float XFreq;
  float YFreq;
  float ZFreq;
  float XMag;
  float YMag;
  float ZMag;
  float StandardDeviation;
  int WholeExtent[6];
  float Center[3];
  float Maximum;

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *data);
private:
  vtkRTAnalyticSource(const vtkRTAnalyticSource&);  // Not implemented.
  void operator=(const vtkRTAnalyticSource&);  // Not implemented.
};


#endif
