/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSource.h
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
// .NAME vtkImageGaussianSource - Create an image with Gaussian pixel values.
// .SECTION Description
// vtkImageGaussianSource just produces images with pixel values determined 
// by a Gaussian.


#ifndef __vtkImageGaussianSource_h
#define __vtkImageGaussianSource_h

#include "vtkImageSource.h"

class VTK_IMAGING_EXPORT vtkImageGaussianSource : public vtkImageSource
{
public:
  static vtkImageGaussianSource *New();
  vtkTypeMacro(vtkImageGaussianSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
		      int zMin, int zMax);
  
  // Description:
  // Set/Get the center of the Gaussian.
  vtkSetVector3Macro(Center, float);
  vtkGetVector3Macro(Center, float);

  // Description:
  // Set/Get the Maximum value of the gaussian
  vtkSetMacro(Maximum, float);
  vtkGetMacro(Maximum, float);

  // Description:
  // Set/Get the standard deviation of the gaussian
  vtkSetMacro(StandardDeviation, float);
  vtkGetMacro(StandardDeviation, float);

protected:
  vtkImageGaussianSource();
  ~vtkImageGaussianSource() {};

  float StandardDeviation;
  int WholeExtent[6];
  float Center[3];
  float Maximum;

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *data);
private:
  vtkImageGaussianSource(const vtkImageGaussianSource&);  // Not implemented.
  void operator=(const vtkImageGaussianSource&);  // Not implemented.
};


#endif
