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
// .NAME vtkImageAlgorithm - Generic algorithm superclass for image algs
// .SECTION Description
// vtkImageToImageAlgorithm is a filter superclass that hides much of the 
// pipeline  complexity. It handles breaking the pipeline execution 
// into smaller extents so that the vtkImageData limits are observed. It 
// also provides support for multithreading. If you don't need any of this
// functionality, consider using vtkSimpleImageToImageAlgorithm instead.
// .SECTION See also
// vtkSimpleImageToImageAlgorithm

#ifndef __vtkImageAlgorithm_h
#define __vtkImageAlgorithm_h

#include "vtkAlgorithm.h"

class vtkImageData;

class VTK_FILTERING_EXPORT vtkImageAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkImageAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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

  // Description:
  // Turn release data flag on or off for all output ports.
  void ReleaseDataFlagOn();
  void ReleaseDataFlagOff();

protected:
  vtkImageAlgorithm();
  ~vtkImageAlgorithm();

  // convinience method
  virtual void ExecuteInformation(vtkInformation *request, 
                                  vtkInformationVector *inputVector, 
                                  vtkInformationVector *outputVector);
  virtual void ComputeInputUpdateExtent(vtkInformation *,
                                        vtkInformationVector *,
                                        vtkInformationVector *);

  // This is called by the superclass.
  // This is the method you should override.
  virtual void ExecuteData(vtkInformation *request, 
                           vtkInformationVector *inputVector, 
                           vtkInformationVector *outputVector);

  // Description:
  // This method is the old style execute method
  virtual void ExecuteData(vtkDataObject *output);
  virtual void Execute();

  // just allocate the output data
  virtual void AllocateOutputData(vtkImageData *out, 
                                  int *uExtent);
  virtual vtkImageData *AllocateOutputData(vtkDataObject *out);

  // copy the other point and cell data
  virtual void CopyAttributeData(vtkImageData *in, vtkImageData *out);
  
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







