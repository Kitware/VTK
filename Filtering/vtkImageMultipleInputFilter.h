/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputFilter.h
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
// .NAME vtkImageMultipleInputFilter - Generic filter that has N inputs.
// .SECTION Description
// vtkImageMultipleInputFilter is a super class for filters that 
// have any number of inputs. Steaming is not available in this class yet.

// .SECTION See Also
// vtkImageToImageFilter vtkImageInPlaceFilter vtkImageTwoInputFilter
// vtkImageTwoOutputFilter



#ifndef __vtkImageMultipleInputFilter_h
#define __vtkImageMultipleInputFilter_h


#include "vtkImageSource.h"
#include "vtkMultiThreader.h"


class VTK_FILTERING_EXPORT vtkImageMultipleInputFilter : public vtkImageSource
{
public:
  static vtkImageMultipleInputFilter *New();
  vtkTypeMacro(vtkImageMultipleInputFilter,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set an Input of this filter. 
  virtual void SetInput(int num, vtkImageData *input);

  // Description:
  // Adds an input to the first null position in the input list.
  // Expands the list memory if necessary
  virtual void AddInput(vtkImageData *input);
  virtual void RemoveInput(vtkImageData *input);
  
  // Description:
  // Get one input to this filter.
  vtkImageData *GetInput(int num);
  vtkImageData *GetInput();

  // Description:
  // Turning bypass on will cause the filter to turn off and
  // simply pass the data from the first input (input0) through.  
  // It is implemented for consistency with vtkImageToImageFilter.
  vtkSetMacro(Bypass,int);
  vtkGetMacro(Bypass,int);
  vtkBooleanMacro(Bypass,int);

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Putting this here until I merge graphics and imaging streaming.
  virtual int SplitExtent(int splitExt[6], int startExt[6], 
			  int num, int total);

  // Description:
  // The execute method created by the subclass.
  // This is kept public instead of protected since it is called
  // from a non-member thread function.
  virtual void ThreadedExecute(vtkImageData **inDatas, 
			       vtkImageData *outData,
			       int extent[6], int threadId);



protected:
  vtkImageMultipleInputFilter();
  ~vtkImageMultipleInputFilter();
  vtkImageMultipleInputFilter(const vtkImageMultipleInputFilter&);
  void operator=(const vtkImageMultipleInputFilter&);

  vtkMultiThreader *Threader;
  int Bypass;
  int NumberOfThreads;

  void ComputeInputUpdateExtents( vtkDataObject *output );
  
  virtual void ComputeInputUpdateExtent( int inExt[6], 
					 int outExt[6], 
					 int whichInput );


  void ExecuteData(vtkDataObject *output);

  // This one gets called by the superclass.
  void ExecuteInformation();
  // This is the one you should override.
  virtual void ExecuteInformation(vtkImageData **, vtkImageData *) {};

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkImageData not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *)
    { vtkErrorMacro( << "RemoveInput() must be called with a vtkImageData not a vtkDataObject."); };
};

#endif







