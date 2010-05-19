/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSource - abstract class specifies interface for visualization network source
// .SECTION Description
// vtkSource is an abstract object that specifies behavior and interface
// of source objects. Source objects are objects that begin visualization
// pipeline. Sources include readers (read data from file or communications
// port) and procedural sources (generate data programmatically). vtkSource 
// objects are also objects that generate output data. In this sense
// vtkSource is used as a superclass to vtkFilter.
//
// Concrete subclasses of vtkSource must define Update() and Execute() 
// methods. The public method Update() invokes network execution and will
// bring the network up-to-date. The protected Execute() method actually
// does the work of data creation/generation. The difference between the two
// methods is that Update() implements input consistency checks and modified
// time comparisons and then invokes the Execute() which is an implementation 
// of a particular algorithm.
//
// An important feature of subclasses of vtkSource is that it is possible 
// to control the memory-management model (i.e., retain output versus delete
// output data). If enabled the ReleaseDataFlag enables the deletion of the
// output data once the downstream process object finishes processing the
// data (please see text).

// .SECTION See Also
// vtkProcessObject vtkDataSetReader vtkFilter vtkPolyDataSource 
// vtkStructuredGridSource vtkStructuredPointsSource vtkUnstructuredGridSource

#ifndef __vtkSource_h
#define __vtkSource_h

#include "vtkProcessObject.h"

class vtkDataObject;
class vtkDataObjectToSourceFriendship;

class VTK_FILTERING_EXPORT vtkSource : public vtkProcessObject
{
public:
  vtkTypeMacro(vtkSource,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bring object up-to-date before execution. Update() checks modified
  // time against last execution time, and re-executes object if necessary.
  virtual void Update();

  // Description:
  // Like update, but make sure the update extent is the whole extent in
  // the output.
  virtual void UpdateWholeExtent();

  // Description:
  // Updates any global information about the data 
  // (like spacing for images)
  virtual void UpdateInformation();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // The update extent for this object is propagated up the pipeline.
  // This propagation may early terminate based on the PipelineMTime.
  virtual void PropagateUpdateExtent(vtkDataObject *output);

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // Propagate back up the pipeline for ports and trigger the update on the
  // other side of the port to allow for asynchronous parallel processing in
  // the pipeline.
  // This propagation may early terminate based on the PipelineMTime.
  virtual void TriggerAsynchronousUpdate();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // Propagate the update back up the pipeline, and perform the actual 
  // work of updating on the way down. When the propagate arrives at a
  // port, block and wait for the asynchronous update to finish on the
  // other side.
  // This propagation may early terminate based on the PipelineMTime.
  virtual void UpdateData(vtkDataObject *output);

  // Description:
  // What is the input update extent that is required to produce the
  // desired output? By default, the whole input is always required but
  // this is overridden in many subclasses. 
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

  // Description:
  // Turn on/off flag to control whether this object's data is released
  // after being used by a source.
  virtual void SetReleaseDataFlag(int);
  virtual int GetReleaseDataFlag();
  vtkBooleanMacro(ReleaseDataFlag,int);

  // Description:
  // Return an array with all the inputs of this process object.
  // This is useful for tracing back in the pipeline to construct
  // graphs etc.
  vtkDataObject **GetOutputs();
  vtkGetMacro(NumberOfOutputs,int);

  // Description:
  // Release/disconnect all outputs of this source. This is intended to be
  // called prior to Delete() if the user is concerned about outputs holding
  // on to the filter/source.
  void UnRegisterAllOutputs(void);

  // Description:
  // Return what index output the passed in output is, return -1 if it
  // does not match any of the outputs
  int GetOutputIndex(vtkDataObject *out);

  // Description:
  // Set this algorithm's executive.  This algorithm is removed from
  // any executive to which it has previously been assigned and then
  // assigned to the given executive.
  virtual void SetExecutive(vtkExecutive* executive);

  // Description:
  // Transform pipeline requests from executives into old-style
  // pipeline calls.  This works with the
  // vtkStreamingDemandDrivenPipeline executive to maintain backward
  // compatibility for filters written as subclasses of vtkSource.
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkSource();
  ~vtkSource();

  // Description:
  // This method is the one that should be used by subclasses, right now the 
  // default implementation is to call the backwards compatibility method
  virtual void ExecuteData(vtkDataObject *output);

  // Description:
  // This method is the old style execute method
  virtual void Execute();

  // By default, UpdateInformation calls this method to copy information
  // unmodified from the input to the output.
  virtual void ExecuteInformation();

  // Called after ExecuteData to call DataHasBeenGenerated on the
  // outputs.  It can be overridden by subclasses to call
  // DataHasBeenGenerated on only a subset of the outputs.  The
  // argument is the pointer to the output data object that was passed
  // to ExecuteData.
  virtual void MarkGeneratedOutputs(vtkDataObject*);

  // Called to allocate the input array.  Copies old inputs.
  void SetNumberOfOutputs(int num);

  // method used internally for getting an output.
  vtkDataObject *GetOutput(int idx);

  // protected methods for setting inputs.
  virtual void SetNthOutput(int num, vtkDataObject *output);
  virtual void AddOutput(vtkDataObject *output);
  virtual void RemoveOutput(vtkDataObject *output);
  
  vtkDataObject **Outputs;     // An Array of the outputs to the filter
  int NumberOfOutputs;
  int Updating;
  // Time when ExecuteInformation was last called.
  vtkTimeStamp InformationTime;

  virtual void ReportReferences(vtkGarbageCollector*);

  // Output port information must match the current outputs.
  int FillOutputPortInformation(int, vtkInformation*);

  // Reimplemented from vtkAlgorithm to maintain backward
  // compatibility for vtkProcessObject.
  virtual void SetNumberOfOutputPorts(int n);

  //BTX
  friend class vtkDataObjectToSourceFriendship;
  //ETX

private:
  vtkSource(const vtkSource&);  // Not implemented.
  void operator=(const vtkSource&);  // Not implemented.
};

#endif



