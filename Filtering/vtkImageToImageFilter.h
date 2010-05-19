/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageToImageFilter - Generic filter that has one input of type vtkImageData
// .SECTION Description
// vtkImageToImageFilter is a filter superclass that hides much of the
// pipeline  complexity. It handles breaking the pipeline execution
// into smaller extents so that the vtkImageData limits are observed. It
// also provides support for multithreading. If you don't need any of this
// functionality, consider using vtkSimpleImageToImageFilter instead.
// .SECTION Warning
// This used to be the parent class for most imaging filter in VTK4.x, now
// this role has been replaced by vtkImageAlgorithm. You should consider
// using vtkImageAlgorithm instead, when writing filter for VTK5 and above.
// This class was kept to ensure full backward compatibility.
// .SECTION See also
// vtkSimpleImageToImageFilter vtkImageSpatialFilter vtkImageAlgorithm

#ifndef __vtkImageToImageFilter_h
#define __vtkImageToImageFilter_h

#include "vtkImageSource.h"

class vtkMultiThreader;

class VTK_FILTERING_EXPORT vtkImageToImageFilter : public vtkImageSource
{
public:
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

  // This also copies other arrays from point and cell data from input to output.
  virtual vtkImageData *AllocateOutputData(vtkDataObject *out);
  
  // The method that starts the multithreading
  void MultiThread(vtkImageData *input, vtkImageData *output);

  void ComputeInputUpdateExtents( vtkDataObject *output );
  virtual void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);

  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkImageToImageFilter(const vtkImageToImageFilter&);  // Not implemented.
  void operator=(const vtkImageToImageFilter&);  // Not implemented.
};

#endif







