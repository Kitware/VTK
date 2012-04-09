/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedImageAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThreadedImageAlgorithm - Generic filter that has one input..
// .SECTION Description
// vtkThreadedImageAlgorithm is a filter superclass that hides much of the 
// pipeline  complexity. It handles breaking the pipeline execution 
// into smaller extents so that the vtkImageData limits are observed. It 
// also provides support for multithreading. If you don't need any of this
// functionality, consider using vtkSimpleImageToImageAlgorithm instead.
// .SECTION See also
// vtkSimpleImageToImageAlgorithm

#ifndef __vtkThreadedImageAlgorithm_h
#define __vtkThreadedImageAlgorithm_h

#include "vtkImageAlgorithm.h"

class vtkImageData;
class vtkMultiThreader;

class VTK_FILTERING_EXPORT vtkThreadedImageAlgorithm : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkThreadedImageAlgorithm,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If the subclass does not define an Execute method, then the task
  // will be broken up, multiple threads will be spawned, and each thread
  // will call this method. It is public so that the thread functions
  // can call this method.
  virtual void ThreadedRequestData(vtkInformation *request, 
                                   vtkInformationVector **inputVector, 
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData, 
                                   vtkImageData **outData,
                                   int extent[6], int threadId);
  
  // also support the old signature
  virtual void ThreadedExecute(vtkImageData *inData, 
                               vtkImageData *outData,
                               int extent[6], int threadId);
  
  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Putting this here until I merge graphics and imaging streaming.
  virtual int SplitExtent(int splitExt[6], int startExt[6], 
                          int num, int total); 

protected:
  vtkThreadedImageAlgorithm();
  ~vtkThreadedImageAlgorithm();

  vtkMultiThreader *Threader;
  int NumberOfThreads;
  
  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

private:
  vtkThreadedImageAlgorithm(const vtkThreadedImageAlgorithm&);  // Not implemented.
  void operator=(const vtkThreadedImageAlgorithm&);  // Not implemented.
};

#endif







