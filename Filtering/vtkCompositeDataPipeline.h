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
// the contents of the composite dataset in this pass. 
// Sources that can produce more than one piece (note that a piece is
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
// composite dataset. 
// .SECTION See also
//  vtkCompositeDataSet

#ifndef __vtkCompositeDataPipeline_h
#define __vtkCompositeDataPipeline_h

#include "vtkStreamingDemandDrivenPipeline.h"

class vtkCompositeDataSet;
class vtkInformationDoubleKey;
class vtkInformationIntegerVectorKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationDataObjectKey;
class vtkInformationIntegerKey;

class VTK_FILTERING_EXPORT vtkCompositeDataPipeline : public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkCompositeDataPipeline* New();
  vtkTypeMacro(vtkCompositeDataPipeline,vtkStreamingDemandDrivenPipeline);
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
  static vtkInformationIntegerKey*       REQUIRES_TIME_DOWNSTREAM();

  // Description:
  // COMPOSITE_DATA_META_DATA is a key placed in the output-port information by
  // readers/sources producing composite datasets. This meta-data provides
  // information about the structure of the composite dataset and things like
  // data-bounds etc.
  // *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
  static vtkInformationObjectBaseKey* COMPOSITE_DATA_META_DATA();

  // Description:
  // UPDATE_COMPOSITE_INDICES is a key placed in the request to request a set of
  // composite indices from a reader/source producing composite dataset.
  // Typically, the reader publishes its structure using
  // COMPOSITE_DATA_META_DATA() and then the sink requests blocks of interest
  // using UPDATE_COMPOSITE_INDICES().
  // Note that UPDATE_COMPOSITE_INDICES has to be sorted vector with increasing
  // indices.
  // *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
  static vtkInformationIntegerVectorKey* UPDATE_COMPOSITE_INDICES();

  // Description:
  // COMPOSITE_INDICES() is put in the output information by the executive if
  // the request has UPDATE_COMPOSITE_INDICES() using the generated composite
  // dataset's structure.
  // Note that COMPOSITE_INDICES has to be sorted vector with increasing
  // indices.
  // *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
  static vtkInformationIntegerVectorKey* COMPOSITE_INDICES();

  // Description:
  // COMPOSITE_INDEX() is added to the leaf nodes of the meta-data composite
  // dataset (COMPOSITE_DATA_META_DATA) during REQUEST_INFORMATION(). Filters
  // downstream can use this index to request specific datasets when
  // creating UPDATE_COMPOSITE_INDICES().
  // *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
  static vtkInformationIntegerKey* COMPOSITE_INDEX();

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

  // Override this check to account for iterating over temporal data.
  virtual int NeedToExecuteBasedOnTime(vtkInformation *outInfo,
                                       vtkDataObject *dataObject);

  // Check whether the data object in the pipeline information for an
  // output port exists and has a valid type.
  virtual int CheckCompositeData(vtkInformation *request,
                                 int port, 
                                 vtkInformationVector** inInfoVec,
                                 vtkInformationVector* outInfoVec);

  // True when the pipeline is iterating over the current (simple) filter
  // to produce composite output. In this case, ExecuteDataStart() should
  // NOT Initialize() the composite output.
  int InLocalLoop;
  
  virtual void ExecuteSimpleAlgorithm(vtkInformation* request,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec,
                                      int compositePort);
  virtual void ExecuteSimpleAlgorithmTime(vtkInformation* request,
                                          vtkInformationVector** inInfoVec,
                                          vtkInformationVector* outInfoVec);
  vtkDataObject* ExecuteSimpleAlgorithmForBlock(
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec,
    vtkInformation* inInfo,
    vtkInformation* outInfo,
    vtkInformation* request,  
    vtkDataObject* dobj);

  bool ShouldIterateOverInput(int& compositePort);
  bool ShouldIterateTemporalData(vtkInformation *request,
                                 vtkInformationVector** inInfoVec, 
                                 vtkInformationVector *outInfoVec);
  virtual int InputTypeIsValid(int port, int index, 
                                vtkInformationVector **inInfoVec);

  vtkInformation* InformationCache;

  vtkInformation* GenericRequest;
  vtkInformation* DataObjectRequest;
  vtkInformation* InformationRequest;
  vtkInformation* UpdateExtentRequest;
  vtkInformation* DataRequest;

  // Because we sometimes have to swap between "simple" data types and composite
  // data types, we sometimes want to skip resetting the pipeline information.
  int SuppressResetPipelineInformation;

  virtual void ResetPipelineInformation(int port, vtkInformation*);

  // Description:
  // Tries to create the best possible composite data output for the given input
  // and non-composite algorithm output. Returns a new instance on success.
  // Don't use this method for creating vtkTemporalDataSet. It's main purpose is
  // to determine if vtkHierarchicalBoxDataSet can be propagated as
  // vtkHierarchicalBoxDataSet in the output (if the algorithm can produce
  // vtkUniformGrid given vtkUniformGrid inputs) or if it should be downgraded
  // to a vtkMultiBlockDataSet.
  vtkCompositeDataSet* CreateOutputCompositeDataSet(
    vtkCompositeDataSet* input, int compositePort);

  // Override this to handle UPDATE_COMPOSITE_INDICES().
  virtual void MarkOutputsGenerated(vtkInformation* request,
                                    vtkInformationVector** inInfoVec,
                                    vtkInformationVector* outInfoVec);

  int NeedToExecuteBasedOnCompositeIndices(vtkInformation* outInfo);

private:
  vtkCompositeDataPipeline(const vtkCompositeDataPipeline&);  // Not implemented.
  void operator=(const vtkCompositeDataPipeline&);  // Not implemented.
};

#endif
