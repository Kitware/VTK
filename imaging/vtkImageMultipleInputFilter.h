/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageMultipleInputFilter - Generic filter that has N inputs.
// .SECTION Description
// vtkImageMultipleInputFilter is a super class for filters that 
// any number of inputs.  Steaming is not available in this class yet.

// .SECTION See Also
// vtkImageFilter vtImageInPlaceFilter vtkImageTwoInputFilter
// vtkImageTwoOutputFilter



#ifndef __vtkImageMultipleInputFilter_h
#define __vtkImageMultipleInputFilter_h


#include "vtkImageSource.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsToImage.h"
#include "vtkImageCache.h"
#include "vtkMultiThreader.h"


class VTK_EXPORT vtkImageMultipleInputFilter : public vtkImageSource
{
public:
  vtkImageMultipleInputFilter();
  ~vtkImageMultipleInputFilter();
  static vtkImageMultipleInputFilter *New() 
    {return new vtkImageMultipleInputFilter;};
  const char *GetClassName() {return "vtkImageMultipleInputFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput(int num, vtkImageCache *input);
  void SetInput(int num, vtkStructuredPoints *spts)
    {this->SetInput(num, spts->GetStructuredPointsToImage()->GetOutput());}
  virtual void AddInput(vtkImageCache *input);
  void AddInput(vtkStructuredPoints *spts)
    {this->AddInput(spts->GetStructuredPointsToImage()->GetOutput());}
  
  void InternalUpdate(vtkImageData *outData);
  void UpdateImageInformation();
  unsigned long int GetPipelineMTime();
  
  // Description:
  // Get one input to this filter.
  vtkImageCache *GetInput(int num) {return this->Inputs[num];};

  // Description:
  // Get the number of inputs to this filter
  vtkGetMacro(NumberOfInputs, int);
  
  // Description:
  // Turning bypass on will causse the filter to turn off and
  // simply pass the data from the first input (input0) through.  
  // It is implemented for consitancy with vtkImageFilter.
  vtkSetMacro(Bypass,int);
  vtkGetMacro(Bypass,int);
  vtkBooleanMacro(Bypass,int);

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // subclasses should define this function
  virtual void ThreadedExecute(vtkImageData **inDatas, 
			       vtkImageData *outData,
			       int extent[6], int threadId);

protected:
  int NumberOfInputs;
  vtkImageCache **Inputs;     // An Array of the inputs to the filter
  vtkMultiThreader *Threader;
  int Bypass;
  int Updating;
  int NumberOfThreads;
  
  // Called to allocate the input array.  Copies old inputs.
  void SetNumberOfInputs(int num);

  virtual void ExecuteImageInformation();
  virtual void ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6],
						int whichInput);
  void RecursiveStreamUpdate(vtkImageData *outData, int axis);
  virtual void Execute(vtkImageData **inDatas, vtkImageData *outData);
};

#endif







