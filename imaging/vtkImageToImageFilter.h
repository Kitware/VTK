/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageFilter.h
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
// .NAME vtkImageToImageFilter - Generic filter that has one input..
// .SECTION Description
// vtkImageToImageFilter is a filter superclass that hides much of the 
// pipeline  complexity. It handles breaking the pipeline execution 
// into smaller extents so that the vtkImageData limits are observed. It 
// also provides support for multithreading. If you don't need any of this
// functionality, consider using vtkSimpleImageToImageFilter instead.
// .SECTION See also
// vtkSimpleImageToImageFilter

#ifndef __vtkImageToImageFilter_h
#define __vtkImageToImageFilter_h


#include "vtkImageSource.h"
#include "vtkMultiThreader.h"

class VTK_EXPORT vtkImageToImageFilter : public vtkImageSource
{
public:
  static vtkImageToImageFilter *New();
  vtkTypeMacro(vtkImageToImageFilter,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the Input of a filter. 
  virtual void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  // Description:
  // Obsolete feature - do not use.
  void SetBypass( int ) {};
  void BypassOn() {};
  void BypassOff() {};
  vtkGetMacro(Bypass,int);

  // Description:
  // If the subclass does not define an Execute method, then the task
  // will be broken up, multiple threads will be spawned, and each thread
  // will call this method. It is public so that the thread functions
  // can call this method.
  virtual void ThreadedExecute(vtkImageData *inData, 
			       vtkImageData *outData,
			       int extent[6], int threadId);
  
  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  void SetInputMemoryLimit(int) 
    {vtkErrorMacro( << "SetInputMemoryLimit is obsolete: Use a vtkImageDataStreamer instead!" );};
  long GetInputMemoryLimit()
    {vtkErrorMacro( << "GetInputMemoryLimit is obsolete: Use a vtkImageDataStreamer instead!" ); return 0;};

  // Description:
  // Putting this here until I merge graphics and imaging streaming.
  virtual int SplitExtent(int splitExt[6], int startExt[6], 
			  int num, int total);
  
protected:
  vtkImageToImageFilter();
  ~vtkImageToImageFilter();
  vtkImageToImageFilter(const vtkImageToImageFilter&) {};
  void operator=(const vtkImageToImageFilter&) {};

  vtkMultiThreader *Threader;
  int Bypass;
  int BypassWasOn;
  int NumberOfThreads;
  
  // This is called by the superclass.
  void ExecuteInformation();
  // This is the method you should override.
  virtual void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);

  // This is called by the superclass.
  // This is the method you should override.
  void ExecuteData(vtkDataObject *output);
  
  // The method that starts the multithreading
  void MultiThread(vtkImageData *input, vtkImageData *output);

  void ComputeInputUpdateExtents( vtkDataObject *output );
  virtual void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
};

#endif







