/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterateFilter.h
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
// .NAME vtkImageIterateFilter - Multiple executes per update.
// .SECTION Description
// vtkImageIterateFilter is a filter superclass that supports calling execute
// multiple times per update.  The largest hack/open issue is that the input
// and output caches are temporarily changed to "fool" the subclasses.  I
// believe the correct solution is to pass the in and out cache to the
// subclasses methods as arguments.  Now the data is passes.  Can the caches
// be passed, and data retrieved from the cache? 

#ifndef __vtkImageIterateFilter_h
#define __vtkImageIterateFilter_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageIterateFilter : public vtkImageToImageFilter
{
public:
  vtkTypeMacro(vtkImageIterateFilter,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get which iteration is current being performed. Normally the
  // user will not access this method.
  vtkGetMacro(Iteration,int);
  vtkGetMacro(NumberOfIterations,int);  

  void ComputeInputUpdateExtents( vtkDataObject *output );
  //BTX
  void ComputeInputUpdateExtent( int [6], int [6] )
    { vtkErrorMacro( << "ComputeInputUpdateExtent should be implemented in subclass" );};
  //ETX

protected:
  vtkImageIterateFilter();
  ~vtkImageIterateFilter();
  vtkImageIterateFilter(const vtkImageIterateFilter&);
  void operator=(const vtkImageIterateFilter&);

  // Superclass API. Sets defaults, then calls 
  // ExecuteInformation(vtkImageData *inData, vtkImageData *outData)
  // for each iteration
  void ExecuteInformation();
  // Called for each iteration (differs from superclass in arguments).
  // You should override this method if needed.
  virtual void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  
  // Superclass API: Calls Execute(vtkImageData *inData, vtkImageData *outData)
  // for each iteration.
  void ExecuteData(vtkDataObject *output);
  virtual void IterativeExecuteData(vtkImageData *in, vtkImageData *out) = 0;
  
  // Replaces "EnlargeOutputUpdateExtent"
  virtual void AllocateOutputScalars(vtkImageData *outData);
  
  // Allows subclass to specify the number of iterations  
  virtual void SetNumberOfIterations(int num);
  
  // for filters that execute multiple times.
  int NumberOfIterations;
  int Iteration;
  // A list of intermediate caches that is created when 
  // is called SetNumberOfIterations()
  vtkImageData **IterationData;
  
  // returns correct vtkImageDatas based on current iteration.
  vtkImageData *GetIterationInput();
  vtkImageData *GetIterationOutput();
};

#endif







