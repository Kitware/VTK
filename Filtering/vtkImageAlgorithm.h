/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageToImageAlgorithm - Generic filter that has one input..
// .SECTION Description
// vtkImageToImageAlgorithm is a filter superclass that hides much of the 
// pipeline  complexity. It handles breaking the pipeline execution 
// into smaller extents so that the vtkImageData limits are observed. It 
// also provides support for multithreading. If you don't need any of this
// functionality, consider using vtkSimpleImageToImageAlgorithm instead.
// .SECTION See also
// vtkSimpleImageToImageAlgorithm

#ifndef __vtkImageToImageAlgorithm_h
#define __vtkImageToImageAlgorithm_h

#include "vtkAlgorithm.h"

class vtkImageData;
class vtkMultiThreader;

class VTK_FILTERING_EXPORT vtkImageAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkImageAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If the subclass does not define an Execute method, then the task
  // will be broken up, multiple threads will be spawned, and each thread
  // will call this method. It is public so that the thread functions
  // can call this method.
  virtual void ThreadedExecute(vtkImageData ***inData, 
                               vtkImageData **outData,
                               int extent[6], int threadId);
  
  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Putting this here until I merge graphics and imaging streaming.
  virtual int SplitExtent(int splitExt[6], int startExt[6], 
                          int num, int total); 
  // Description:
  // Get the output data object for a port on this algorithm.
  vtkImageData* GetOutput();
  vtkImageData* GetOutput(int);

  // Description:
  // Set an input of this algorithm.
  void SetInput(vtkImageData*);
  void SetInput(int, vtkImageData*);

  // Description:
  // Add an input of this algorithm.
  void AddInput(vtkImageData*);
  void AddInput(int, vtkImageData*);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessDownstreamRequest(vtkInformation *, 
                                       vtkInformationVector *, 
                                       vtkInformationVector *);
  virtual int ProcessUpstreamRequest(vtkInformation *, 
                                     vtkInformationVector *, 
                                     vtkInformationVector *);

protected:
  vtkImageAlgorithm();
  ~vtkImageAlgorithm();

  vtkMultiThreader *Threader;
  int NumberOfThreads;
  
  // convinience method
  virtual void ExecuteInformation(vtkInformation *request, 
                                  vtkInformationVector *inputVector, 
                                  vtkInformationVector *outputVector);
  virtual void ComputeInputUpdateExtent(vtkInformation *,
                                        vtkInformationVector *,
                                        vtkInformationVector *);

  // This is called by the superclass.
  // This is the method you should override.
  void ExecuteData(vtkInformation *request, 
                   vtkInformationVector *inputVector, 
                   vtkInformationVector *outputVector);

  // just allocate the output data
  virtual void AllocateOutputData(vtkImageData *out, 
                                  int *uExtent);

  // copy the other point and cell data
  virtual void CopyAttributeData(vtkImageData *in, vtkImageData *out);
  
  // The method that starts the multithreading
  void MultiThread(vtkImageData *input, vtkImageData *output);

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  
  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

private:
  vtkImageAlgorithm(const vtkImageAlgorithm&);  // Not implemented.
  void operator=(const vtkImageAlgorithm&);  // Not implemented.
};

#endif







