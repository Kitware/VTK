/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataPipeline - Executive supporting composite datasets.
// .SECTION Description
// vtkCompositeDataPipeline is an executive that supports the processing of
// composite dataset. It supports algorithms that are aware of composite
// dataset as well as those that are not. Type checking is performed at run
// time. Algorithms that are not composite dataset-aware have to support
// all dataset types contained in the composite dataset. The pipeline
// execution can be summarized as follows: 
//
// * REQUEST_INFORMATION: The producers have to provide information about
// the contents of the composite dataset in this pass. This is accomplished
// by creating and populating a vtkMultiGroupDataInformation and setting
// it using the COMPOSITE_DATA_INFORMATION() key in the output information
// vector. Sources that can produce more than one piece (note that a piece is
// different than a block; each piece consistes of 0 or more blocks) should
// set MAXIMUM_NUMBER_OF_PIECES to -1.
//
// * REQUEST_UPDATE_EXTENT: This pass is identical to the one implemented
// in vtkStreamingDemandDrivenPipeline
//
// * REQUEST_DATA: This is where the algorithms execute. If the 
// vtkCompositeDataPipeline is assigned to a simple filter, 
// it will invoke the  vtkStreamingDemandDrivenPipeline passes in a loop, 
// passing a different block each time and will collect the results in a 
// composite dataset (vtkMultiGroupDataSet).
// .SECTION See also
//  vtkMultiGroupDataInformation vtkCompositeDataSet vtkMultiGroupDataSet

#ifndef __vtkCompositeDataPipeline_h
#define __vtkCompositeDataPipeline_h

#include "vtkStreamingDemandDrivenPipeline.h"

class vtkCompositeDataSet;
class vtkMultiGroupDataSet;
class vtkInformationDoubleKey;
class vtkInformationIntegerVectorKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationDataObjectKey;

class VTK_FILTERING_EXPORT vtkCompositeDataPipeline : public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkCompositeDataPipeline* New();
  vtkTypeRevisionMacro(vtkCompositeDataPipeline,vtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generalized interface for asking the executive to fullfill update
  // requests.
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

  // Description:
  // Returns the data object stored with the DATA_OBJECT() in the
  // output port
  vtkDataObject* GetCompositeOutputData(int port);

  // Description:
  // Returns the data object stored with the DATA_OBJECT() in the
  // input port
  vtkDataObject* GetCompositeInputData(
    int port, int index, vtkInformationVector **inInfoVec);

  // Description:
  // vtkCompositeDataPipeline specific keys
  static vtkInformationStringKey*        COMPOSITE_DATA_TYPE_NAME();
  static vtkInformationObjectBaseKey*    COMPOSITE_DATA_INFORMATION();
  static vtkInformationIntegerKey*       MARKED_FOR_UPDATE();
  static vtkInformationStringKey*        INPUT_REQUIRED_COMPOSITE_DATA_TYPE();
  static vtkInformationObjectBaseKey*    UPDATE_BLOCKS();

protected:
  vtkCompositeDataPipeline();
  ~vtkCompositeDataPipeline();

  virtual int ForwardUpstream(vtkInformation* request);
  virtual int ForwardUpstream(int i, int j, vtkInformation* request);

  // Copy information for the given request.
  virtual void CopyDefaultInformation(vtkInformation* request, int direction,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec);

  virtual void CopyFromDataToInformation(
    vtkDataObject* dobj, vtkInformation* inInfo);
  virtual void PushInformation(vtkInformation*);
  virtual void PopInformation (vtkInformation*);

  virtual int ExecuteDataObject(vtkInformation* request,
                                vtkInformationVector** inInfo,
                                vtkInformationVector* outInfo);

  virtual int ExecuteData(vtkInformation* request,
                          vtkInformationVector** inInfoVec,
                          vtkInformationVector* outInfoVec);

  virtual void ExecuteDataStart(vtkInformation* request,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec);

  // Override this check to account for update extent.
  virtual int NeedToExecuteData(int outputPort,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec);

  // Helper to check output information before propagating it to inputs.
  virtual int VerifyOutputInformation(int outputPort,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec);


  // Check whether the data object in the pipeline information for an
  // output port exists and has a valid type.
  virtual int CheckCompositeData(int port, vtkInformationVector* outInfoVec);

  // True when the pipeline is iterating over the current (simple) filter
  // to produce composite output. In this case, ExecuteDataStart() should
  // NOT Initialize() the composite output.
  int InLocalLoop;
  
  virtual void ExecuteSimpleAlgorithm(vtkInformation* request,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec,
                                      int compositePort);
  void CheckInputPorts(int& inputPortIsComposite,
                       int& inputIsComposite,
                       int& compositePort);
  virtual int InputTypeIsValid(int port, int index,vtkInformationVector **);

  vtkInformation* InformationCache;

  vtkInformation* GenericRequest;
  vtkInformation* DataObjectRequest;
  vtkInformation* InformationRequest;
  vtkInformation* UpdateExtentRequest;
  vtkInformation* DataRequest;

private:
  vtkCompositeDataPipeline(const vtkCompositeDataPipeline&);  // Not implemented.
  void operator=(const vtkCompositeDataPipeline&);  // Not implemented.
};

#endif
