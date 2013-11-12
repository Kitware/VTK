/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageAppendComponents - Collects components from two inputs into
// one output.
// .SECTION Description
// vtkImageAppendComponents takes the components from two inputs and merges
// them into one output. If Input1 has M components, and Input2 has N
// components, the output will have M+N components with input1
// components coming first.


#ifndef __vtkImageAppendComponents_h
#define __vtkImageAppendComponents_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImageAppendComponents : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageAppendComponents *New();
  vtkTypeMacro(vtkImageAppendComponents,vtkThreadedImageAlgorithm);

  // Description:
  // Replace one of the input connections with a new input.  You can
  // only replace input connections that you previously created with
  // AddInputConnection() or, in the case of the first input,
  // with SetInputConnection().
  virtual void ReplaceNthInputConnection(int idx, vtkAlgorithmOutput* input);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(int num, vtkDataObject *input);
  void SetInputData(vtkDataObject *input) { this->SetInputData(0, input); };

  // Description:
  // Get one input to this filter. This method is only for support of
  // old-style pipeline connections.  When writing new code you should
  // use vtkAlgorithm::GetInputConnection(0, num).
  vtkDataObject *GetInput(int num);
  vtkDataObject *GetInput() { return this->GetInput(0); };

  // Description:
  // Get the number of inputs to this filter. This method is only for
  // support of old-style pipeline connections.  When writing new code
  // you should use vtkAlgorithm::GetNumberOfInputConnections(0).
  int GetNumberOfInputs() { return this->GetNumberOfInputConnections(0); };

protected:
  vtkImageAppendComponents() {}
  ~vtkImageAppendComponents() {}

  virtual int RequestInformation (vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  void ThreadedRequestData (vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector,
                            vtkImageData ***inData, vtkImageData **outData,
                            int ext[6], int id);

  // Implement methods required by vtkAlgorithm.
  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkImageAppendComponents(const vtkImageAppendComponents&);  // Not implemented.
  void operator=(const vtkImageAppendComponents&);  // Not implemented.
};

#endif




// VTK-HeaderTest-Exclude: vtkImageAppendComponents.h
