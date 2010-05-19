/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessObject - abstract class specifies interface for visualization filters
//
// .SECTION Description
// vtkProcessObject is an abstract object that specifies behavior and
// interface of visualization network process objects (sources, filters,
// mappers). Source objects are creators of visualization data; filters
// input, process, and output visualization data; and mappers transform data
// into another form (like rendering primitives or write data to a file).
//
// vtkProcessObject fires events for Start and End events before and after
// object execution (via Execute()). These events can be used for any purpose
// (e.g., debugging info, highlighting/notifying user interface, etc.)
//
// Another event, Progress, can be observed. Some filters fire this 
// event periodically during their execution. The use is similar to that of 
// Start and End events. Filters may also check their AbortExecute
// flag to determine whether to prematurely end their execution.
//
// An important feature of subclasses of vtkProcessObject is that it is
// possible to control the memory-management model (i.e., retain output
// versus delete output data). If enabled the ReleaseDataFlag enables the
// deletion of the output data once the downstream process object finishes
// processing the data (please see text).  
//
// .SECTION See Also
// vtkDataObject vtkSource vtkFilter vtkMapper vtkWriter

#ifndef __vtkProcessObject_h
#define __vtkProcessObject_h

#include "vtkAlgorithm.h"

class vtkDataObject;

class VTK_FILTERING_EXPORT vtkProcessObject : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkProcessObject,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return an array with all the inputs of this process object.
  // This is useful for tracing back in the pipeline to construct
  // graphs etc.
  vtkDataObject **GetInputs();
  int GetNumberOfInputs();

  // Description:
  // This method will rearrange the input array so that all NULL entries 
  // are removed.
  void SqueezeInputArray();
  
  // Description:
  // Remove all the input data.
  void RemoveAllInputs();

  // Description:
  // Reimplemented from vtkAlgorithm to maintain backward
  // compatibility for vtkProcessObject.
  virtual void SetInputConnection(vtkAlgorithmOutput* input) {
    this->vtkAlgorithm::SetInputConnection(input); }
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void AddInputConnection(vtkAlgorithmOutput* input)
    {
      this->AddInputConnection(0, input);
    }
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void SetNthInputConnection(int port, int index,
                                     vtkAlgorithmOutput* input);
  virtual void SetNumberOfInputConnections(int port, int n);
protected:
  vtkProcessObject();
  ~vtkProcessObject();

  int NumberOfInputs;
  int NumberOfRequiredInputs;
  vtkDataObject **Inputs;     //An array of the inputs to the filter

  // Called to allocate the input array.  Copies old inputs.
  void SetNumberOfInputs(int num);

  // protected methods for setting inputs.
  virtual void SetNthInput(int num, vtkDataObject *input);
  virtual void AddInput(vtkDataObject *input);
  virtual void RemoveInput(vtkDataObject *input);

  virtual void ReportReferences(vtkGarbageCollector*);

  // Implement methods required by vtkAlgorithm.
  virtual int FillInputPortInformation(int, vtkInformation*);
  virtual int FillOutputPortInformation(int, vtkInformation*);

  // Helper methods for compatibility layer.
  void AddInputInternal(vtkDataObject* input);
  void RemoveInputInternal(vtkDataObject* input);
  void SetupInputs();

private:
  vtkProcessObject(const vtkProcessObject&);  // Not implemented.
  void operator=(const vtkProcessObject&);  // Not implemented.
};

#endif

