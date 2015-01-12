/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridAppend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .NAME vtkStructuredGridAppend - Collects data from multiple inputs into one structured grid.
// .SECTION Description
// vtkStructuredGridAppend takes the components from multiple inputs and merges
// them into one output. All inputs must have the same number of scalar components.
// All inputs must have the same scalar type.


#ifndef vtkStructuredGridAppend_h
#define vtkStructuredGridAppend_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkStructuredGridAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkStructuredGridAppend : public vtkStructuredGridAlgorithm
{
 public:
  static vtkStructuredGridAppend *New();
  vtkTypeMacro(vtkStructuredGridAppend,vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  vtkStructuredGridAppend();
  ~vtkStructuredGridAppend();

  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  // see vtkAlgorithm for docs.
  virtual int FillInputPortInformation(int, vtkInformation*);

 private:
  vtkStructuredGridAppend(const vtkStructuredGridAppend&);  // Not implemented.
  void operator=(const vtkStructuredGridAppend&);  // Not implemented.
};

#endif
