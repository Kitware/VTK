/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAlgorithm - Superclass for all sources, filters, and sinks in VTK.
// .SECTION Description
// vtkAlgorithm is the superclass for all sources, filters, and sinks
// in VTK.  It defines a generalized interface for executing data
// processing algorithms.
//
// Instances may be used independently or within pipelines with a
// variety of architectures and update mechanisms.  Pipelines are
// controlled by instances of vtkExecutive.  Every vtkAlgorithm
// instance has an associated vtkExecutive when it is used in a
// pipeline.  The executive is responsible for data flow.

#ifndef __vtkAlgorithm_h
#define __vtkAlgorithm_h

#include "vtkObject.h"

class vtkAlgorithmInternals;
class vtkData;
class vtkExecutive;
class vtkInformationVector;
class vtkAlgorithmOutput;

class VTK_COMMON_EXPORT vtkAlgorithm : public vtkObject
{
public:
  static vtkAlgorithm *New();
  vtkTypeRevisionMacro(vtkAlgorithm,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Check whether this algorithm has an assigned executive.  This
  // will NOT create a default executive.
  int HasExecutive();

  // Description:
  // Get this algorithm's executive.  If it has none, a default
  // executive will be created.
  vtkExecutive* GetExecutive();

  // Description:
  // Set this algorithm's executive.  This algorithm is removed from
  // any executive to which it has previously been assigned and then
  // assigned to the given executive.  A algorithm's executive should
  // not be changed after pipeline connectivity has been established.
  void SetExecutive(vtkExecutive* executive);

  // Description:
  // Upstream/Downstream requests form the generalized interface
  // through which executives invoke a algorithm's functionality.
  // Upstream requests correspond to information flow from the
  // algorithm's outputs to its inputs.
  //
  // An upstream request is defined by the contents of the output
  // information vector passed to ProcessUpstreamRequest.  The results
  // of an upstream request are stored in the input information vector
  // passed to ProcessUpstreamRequest.
  virtual int ProcessUpstreamRequest(vtkInformationVector* inInfo,
                                     vtkInformationVector* outInfo);

  // Description:
  // Upstream/Downstream requests form the generalized interface
  // through which executives invoke a algorithm's functionality.
  // Downstream requests correspond to information flow from the
  // algorithm's inputs to its outputs.
  //
  // An downstream request is defined by the contents of the input
  // information vector passed to ProcessDownstreamRequest.  The
  // results of an downstream request are stored in the output
  // information vector passed to ProcessDownstreamRequest.
  virtual int ProcessDownstreamRequest(vtkInformationVector* inInfo,
                                       vtkInformationVector* outInfo);

  // Description:
  // Get the information objects associated with this algorithm's
  // input ports.  There is one input port per input to the algorithm.
  // Each input port tells executives what kind of data and downstream
  // requests this algorithm can handle for that input.
  vtkInformationVector* GetInputPortInformation();

  // Description:
  // Get the information objects associated with this algorithm's
  // output ports.  There is one output port per output from the
  // algorithm.  Each output port tells executives what kind of
  // upstream requests this algorithm can handle for that output.
  vtkInformationVector* GetOutputPortInformation();

  // Description:
  // Set an input of this algorithm.  The input must correspond to the
  // output of another algorithm.
  //
  // TODO: Accept an actual vtkDataObject instance as input and use it
  // to establish the pipeline connection to maintain backward
  // compatability.
  void SetInput(int index, vtkAlgorithmOutput* input);

  // Description:
  // Get a proxy object corresponding to the given output of this
  // algorithm.  The proxy object can be passed to another algorithm's
  // SetInput method to establish a pipeline connection.
  //
  // TODO: Use an actual vtkDataObject instance as the proxy object to
  // maintain backward compatability.
  vtkAlgorithmOutput* GetOutput(int index);

  // Description:
  // Set the connection for the given input port index.  Removes
  // any other connections.
  void SetInputConnection(int index, vtkAlgorithmOutput* input);

  // Description:
  // Add a connection to the given input port index.
  void AddInputConnection(int index, vtkAlgorithmOutput* input);

  // Description:
  // Remove a connection from the given input port index.
  void RemoveInputConnection(int index, vtkAlgorithmOutput* input);

  // Description:
  // Get a proxy object corresponding to the given output port of this
  // algorithm.  The proxy object can be passed to another algorithm's
  // InputConnection methods to modify pipeline connectivity.
  vtkAlgorithmOutput* GetOutputPort(int index);

  // Description:
  // Get the number of input ports used by the algorithm.
  int GetNumberOfInputPorts();

  // Description:
  // Get the number of output ports provided by the algorithm.
  int GetNumberOfOutputPorts();

  // Description:
  // Get the number of input currently connected to a port.
  int GetNumberOfInputConnections(int port);

  // Description:
  // Get the algorithm output port connected to an input port.
  vtkAlgorithmOutput* GetInputConnection(int port, int index);

  // Description:
  // Bring this algorithm's outputs up-to-date.
  virtual void Update();

  // Description:
  // Decrement the count of references to this object and participate
  // in garbage collection.
  virtual void UnRegister(vtkObjectBase* o);

protected:
  vtkAlgorithm();
  ~vtkAlgorithm();

  // Description:
  // Fill the input port information objects for this algorithm.  This
  // is invoked by the first call to GetInputPortInformation so
  // subclasses can specify what they can handle.
  virtual void FillInputPortInformation(vtkInformationVector* portInfo);

  // Description:
  // Fill the output port information objects for this algorithm.
  // This is invoked by the first call to GetOutputPortInformation so
  // subclasses can specify what they can handle.
  virtual void FillOutputPortInformation(vtkInformationVector* portInfo);

  // Description:
  // Set the number of input ports used by the algorithm.
  void SetNumberOfInputPorts(int n);

  // Description:
  // Set the number of output ports provided by the algorithm.
  void SetNumberOfOutputPorts(int n);

  // Helper methods to check input/output port index ranges.
  int InputPortIndexInRange(int index, const char* action);
  int OutputPortIndexInRange(int index, const char* action);

  // Garbage collection support.
  virtual void ReportReferences(vtkGarbageCollector*);
  virtual void RemoveReferences();
  virtual void GarbageCollectionStarting();
  int GarbageCollecting;
private:
  vtkAlgorithmInternals* AlgorithmInternal;
  static void ConnectionAdd(vtkAlgorithm* producer, int producerPort,
                            vtkAlgorithm* consumer, int consumerPort);
  static void ConnectionRemove(vtkAlgorithm* producer, int producerPort,
                               vtkAlgorithm* consumer, int consumerPort);
  static void ConnectionRemoveAllInput(vtkAlgorithm* consumer, int port);
  static void ConnectionRemoveAllOutput(vtkAlgorithm* producer, int port);

private:
  vtkAlgorithm(const vtkAlgorithm&);  // Not implemented.
  void operator=(const vtkAlgorithm&);  // Not implemented.
};

#endif
