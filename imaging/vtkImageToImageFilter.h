/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageFilter.h
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
// .NAME vtkImageToImageFilter - Generic filter that has one input..
// .SECTION Description
// vtkImageToImageFilter is a filter superclass that hides much of the pipeline 
// complexity. It handles breaking the pipeline execution into smaller
// extents so that the vtkImageData memory limits are observed. It 
// also provides support for multithreading.



#ifndef __vtkImageToImageFilter_h
#define __vtkImageToImageFilter_h


#include "vtkImageSource.h"
#include "vtkMultiThreader.h"

class VTK_EXPORT vtkImageToImageFilter : public vtkImageSource
{
public:
  static vtkImageToImageFilter *New() {return new vtkImageToImageFilter;};
  const char *GetClassName() {return "vtkImageToImageFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the Input of a filter. 
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  // Description:
  // Turning bypass on will cause the filter to turn off and
  // simply pass the data through.  This main purpose for this functionality
  // is support for vtkImageDecomposedFilter.  InputMemoryLimit is ignored
  // when Bypass in on.
  vtkSetMacro(Bypass,int);
  vtkGetMacro(Bypass,int);
  vtkBooleanMacro(Bypass,int);

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

  // ----- Streaming -----

  // Description:
  // The InputMemoryLimit will cause this filter to initiate streaming.
  // The problem is divided up until the memory used by the input data
  // is less than this limit. (units kiloBytes)
  void SetInputMemoryLimit(int limit);
  long GetInputMemoryLimit();

  // Description:
  // Putting this here until I merge graphics and imaging streaming.
  virtual int SplitExtent(int splitExt[6], int startExt[6], 
			  int num, int total);
  
  // Legacy !!!!!!!!!!!!!!! ---------------------------------
  
  // Description:
  // Legacy method.  May go away at any time. It should be called 
  // UpdateInformation.
  virtual void UpdateImageInformation() 
    {vtkWarningMacro("Use UpdateInformation instead of UpdateImageInformation");  
    this->UpdateInformation();
    }
  
  // Description:
  // Legacy method.  May go away at any time. You should call ExecuteInformation
  // which should call vtkImageToImageFilter::ExecuteInformation to set up defaults,
  // and the change what needs to be changed.
  virtual void ExecuteImageInformation() {this->LegacyHack = 0;}
  int LegacyHack;
  
  
protected:
  vtkImageToImageFilter();
  ~vtkImageToImageFilter();

  vtkMultiThreader *Threader;
  int Bypass;
  int BypassWasOn;
  int NumberOfThreads;
  
  // This is called by the superclass.
  void ExecuteInformation();
  // This is the method you should override.
  virtual void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);

  // This is called by the superclass.
  void Execute();
  // This is the method you should override.
  virtual void Execute(vtkImageData *inData, vtkImageData *outData);
  // scalars are allocated here.   
  void StreamExecuteStart(); 

  // Overide this if your filter is not a pixel for pixel operation.
  // Given outExt, tell me what you need for inExt.
  virtual void ComputeRequiredInputUpdateExtent(int inExt[6],int outExt[6]);
  // These replace ComputeRequiredInputUpdateExtent if your filter
  // will initiate streaming.
  int GetNumberOfStreamDivisions();  
  int ComputeDivisionExtents(vtkDataObject *output, 
			     int division, int numDivisions);

};

#endif







