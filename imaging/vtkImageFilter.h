/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFilter.h
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
// .NAME vtkImageFilter - Generic filter that has one input..
// .SECTION Description
// vtkImageFilter is a filter superclass that hides much of the pipeline 
// complexity. It handles breaking the pipeline execution into smaller
// extents so that the vtkImageCache memory limits are observed. It 
// also provides support for multithreading.



#ifndef __vtkImageFilter_h
#define __vtkImageFilter_h


#include "vtkImageSource.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsToImage.h"
#include "vtkImageCache.h"
#include "vtkMultiThreader.h"

class VTK_EXPORT vtkImageFilter : public vtkImageSource
{
public:
  vtkImageFilter();
  ~vtkImageFilter();
  static vtkImageFilter *New() {return new vtkImageFilter;};
  const char *GetClassName() {return "vtkImageFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Set the Input of a filter. 
  virtual void SetInput(vtkImageCache *input);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}
  

  // Description:
  // This method is called by the cache.  It eventually calls the
  // Execute(vtkImageData *, vtkImageData *) method.
  // ImageInformation has already been updated by this point, 
  // and outRegion is in local coordinates.
  // This method will stream to get the input, and loops over extra axes.
  // Only the UpdateExtent from output will get updated.
  virtual void InternalUpdate(vtkImageData *outData);
  
  // Description:
  // This method sets the WholeExtent, Spacing and Origin of the output.
  virtual void UpdateImageInformation();


  // Description:
  // This Method returns the MTime of the pipeline up to and including this
  // filter Note: current implementation may create a cascade of
  // GetPipelineMTime calls.  Each GetPipelineMTime call propagates the call
  // all the way to the original source.
  unsigned long int GetPipelineMTime();

  
  // Description:
  // Get input to this filter.
  vtkGetObjectMacro(Input,vtkImageCache);

  // Description:
  // Turning bypass on will cause the filter to turn off and
  // simply pass the data through.  This main purpose for this functionality
  // is support for vtkImageDecomposedFilter.  InputMemoryLimit is ignored
  // when Bypass in on.
  vtkSetMacro(Bypass,int);
  vtkGetMacro(Bypass,int);
  vtkBooleanMacro(Bypass,int);

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // subclasses should define this function
  virtual void ThreadedExecute(vtkImageData *inData, 
			       vtkImageData *outData,
			       int extent[6], int threadId);
  
protected:
  vtkImageCache *Input;     
  vtkMultiThreader *Threader;
  int Bypass;
  int Updating;
  int NumberOfThreads;
  
  virtual void ExecuteImageInformation();
  virtual void ComputeRequiredInputUpdateExtent(int inExt[6],int outExt[6]);

  virtual void RecursiveStreamUpdate(vtkImageData *outData);
  virtual void Execute(vtkImageData *inData, vtkImageData *outData);
};

#endif







