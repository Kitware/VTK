/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRFFT.h
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
// .NAME vtkImageRFFT -  Reverse Fast Fourier Transform.
// .SECTION Description
// vtkImageRFFT implements the reverse fast Fourier transform.  The input
// can have real or complex data in any components and data types, but
// the output is always complex floats with real values in component0, and
// imaginary values in component1.  The filter is fastest for images that
// have power of two sizes.  The filter uses a butterfly fitlers for each
// prime factor of the dimension.  This makes images with prime number dimensions 
// (i.e. 17x17) much slower to compute.  Multi dimensional (i.e volumes) 
// FFT's are decomposed so that each axis executes in series.
// In most cases the RFFT will produce an image whose imaginary values are all
// zero's. In this case vtkImageExtractComponents can be used to remove
// this imaginary components leaving only the real image.

// .SECTION See Also
// vtkImageExtractComponenents
 


#ifndef __vtkImageRFFT_h
#define __vtkImageRFFT_h


#include "vtkImageFourierFilter.h"

class VTK_IMAGING_EXPORT vtkImageRFFT : public vtkImageFourierFilter
{
public:
  static vtkImageRFFT *New();
  vtkTypeRevisionMacro(vtkImageRFFT,vtkImageFourierFilter);

  
  // Description:
  // For streaming and threads.  Splits output update extent into num pieces.
  // This method needs to be called num times.  Results must not overlap for
  // consistent starting extent.  Subclass can override this method.  This
  // method returns the number of pieces resulting from a successful split.
  // This can be from 1 to "total".  If 1 is returned, the extent cannot be
  // split.
  int SplitExtent(int splitExt[6], int startExt[6], 
                  int num, int total);

  virtual void IterativeExecuteData(vtkImageData *in, vtkImageData *out) {
    this->MultiThread(in,out);};

protected:
  vtkImageRFFT() {};
  ~vtkImageRFFT() {};

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageIterateFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int threadId);
private:
  vtkImageRFFT(const vtkImageRFFT&);  // Not implemented.
  void operator=(const vtkImageRFFT&);  // Not implemented.
};

#endif










