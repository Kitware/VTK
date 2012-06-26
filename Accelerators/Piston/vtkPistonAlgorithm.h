/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPistonAlgorithm - Superclass for algorithms that produce only
// PistonDataObjects
// .SECTION Description
// vtkPistonAlgorithm is a convenience class to make writing algorithms
// that operate in piston space easer. Basically one does that by subclassing
// this class and overriding Execute() to call into a method that calls into
// an external function compiled with the cuda compiler.
//
// There are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this class
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be PistonDataObject. If that
// isn't the case then please override this method in your subclass. This
// class breaks out the downstream requests into separate functions such as
// ExecuteData and ExecuteInformation.

#ifndef __vtkPistonAlgorithm_h
#define __vtkPistonAlgorithm_h

#include "vtkAcceleratorsPistonModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkPistonDataObject;

class VTKACCELERATORSPISTON_EXPORT vtkPistonAlgorithm : public vtkAlgorithm
{
public:
  static vtkPistonAlgorithm *New();
  vtkTypeMacro(vtkPistonAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Interface the algorithm to the Pipeline's passes.
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);


  // Description:
  // A convenience method to reduce code duplication that gets
  // the output as the expected type or NULL.
  vtkPistonDataObject *GetPistonDataObjectOutput(int port);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(int num, vtkDataObject *input);
  void SetInputData(vtkDataObject *input) { this->SetInputData(0, input); };

protected:
  vtkPistonAlgorithm();
  ~vtkPistonAlgorithm();

  // Description:
  // Overridden to say that we take in and produce vtkPistonDataObjects
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  // Description:
  // Produce empty output of the proper type for RequestData to fill in.
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  // Description:
  // Produce meta-data about what RequestData will produce.
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  // Description:
  // Participate in pipeline's sub extent determination.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // Description:
  // Method that does the actual calculation.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // Typically Execute uses this to copy input bounds to output.
  // Algorithms for which this heuristic is poor, should override and
  // do it exactly, perhaps by asking the GPU to calculate it,
  virtual void PassBoundsForward(vtkPistonDataObject *id,
                                 vtkPistonDataObject *od);

private:
  vtkPistonAlgorithm(const vtkPistonAlgorithm&);  // Not implemented.
  void operator=(const vtkPistonAlgorithm&);  // Not implemented.
};

#endif
