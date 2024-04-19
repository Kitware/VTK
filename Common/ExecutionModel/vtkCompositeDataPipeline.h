// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeDataPipeline
 * @brief   Executive supporting composite datasets.
 *
 * vtkCompositeDataPipeline is an executive that supports the processing of
 * composite dataset. It supports algorithms that are aware of composite
 * dataset as well as those that are not. Type checking is performed at run
 * time. Algorithms that are not composite dataset-aware have to support
 * all dataset types contained in the composite dataset. The pipeline
 * execution can be summarized as follows:
 *
 * * REQUEST_INFORMATION: The producers have to provide information about
 * the contents of the composite dataset in this pass.
 * Sources that can produce more than one piece (note that a piece is
 * different than a block; each piece consists of 0 or more blocks) should
 * set CAN_HANDLE_PIECE_REQUEST.
 *
 * * REQUEST_UPDATE_EXTENT: This pass is identical to the one implemented
 * in vtkStreamingDemandDrivenPipeline
 *
 * * REQUEST_DATA: This is where the algorithms execute. If the
 * vtkCompositeDataPipeline is assigned to a simple filter,
 * it will invoke the  vtkStreamingDemandDrivenPipeline passes in a loop,
 * passing a different block each time and will collect the results in a
 * composite dataset.
 * @sa
 *  vtkCompositeDataSet
 */

#ifndef vtkCompositeDataPipeline_h
#define vtkCompositeDataPipeline_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO
#include <vtkSmartPointer.h>  // smart pointer

#include <vector> // for vector in return type

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataSet;
class vtkCompositeDataIterator;
class vtkInformationDoubleKey;
class vtkInformationIntegerVectorKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationDataObjectKey;
class vtkInformationIntegerKey;

class VTKCOMMONEXECUTIONMODEL_EXPORT VTK_MARSHALAUTO vtkCompositeDataPipeline
  : public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkCompositeDataPipeline* New();
  vtkTypeMacro(vtkCompositeDataPipeline, vtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the data object stored with the DATA_OBJECT() in the
   * output port
   */
  vtkDataObject* GetCompositeOutputData(int port);

  /**
   * Returns the data object stored with the DATA_OBJECT() in the
   * input port
   */
  vtkDataObject* GetCompositeInputData(int port, int index, vtkInformationVector** inInfoVec);

  /**
   * An integer key that indicates to the source to load all requested
   * blocks specified in UPDATE_COMPOSITE_INDICES.
   */
  static vtkInformationIntegerKey* LOAD_REQUESTED_BLOCKS();

  /**
   * COMPOSITE_DATA_META_DATA is a key placed in the output-port information by
   * readers/sources producing composite datasets. This meta-data provides
   * information about the structure of the composite dataset and things like
   * data-bounds etc.
   * *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
   */
  static vtkInformationObjectBaseKey* COMPOSITE_DATA_META_DATA();

  /**
   * UPDATE_COMPOSITE_INDICES is a key placed in the request to request a set of
   * composite indices from a reader/source producing composite dataset.
   * Typically, the reader publishes its structure using
   * COMPOSITE_DATA_META_DATA() and then the sink requests blocks of interest
   * using UPDATE_COMPOSITE_INDICES().
   * Note that UPDATE_COMPOSITE_INDICES has to be sorted vector with increasing
   * indices.
   * *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
   */
  static vtkInformationIntegerVectorKey* UPDATE_COMPOSITE_INDICES();

  /**
   * BLOCK_AMOUNT_OF_DETAIL is a key placed in the information about a multi-block
   * dataset that indicates how complex the block is.  It is intended to work with
   * multi-resolution streaming code.  For example in a multi-resolution dataset of
   * points, this key might store the number of points.
   * *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
   */
  static vtkInformationDoubleKey* BLOCK_AMOUNT_OF_DETAIL();

protected:
  vtkCompositeDataPipeline();
  ~vtkCompositeDataPipeline() override;

  int ForwardUpstream(vtkInformation* request) override;
  virtual int ForwardUpstream(int i, int j, vtkInformation* request);

  // Copy information for the given request.
  void CopyDefaultInformation(vtkInformation* request, int direction,
    vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec) override;

  virtual void PushInformation(vtkInformation*);
  virtual void PopInformation(vtkInformation*);

  int ExecuteDataObject(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  int ExecuteData(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) override;

  void ExecuteDataStart(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) override;

  // Override this check to account for update extent.
  int NeedToExecuteData(
    int outputPort, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec) override;

  // Check whether the data object in the pipeline information exists
  // and has a valid type.
  virtual int CheckCompositeData(
    vtkInformation* request, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec);

  // True when the pipeline is iterating over the current (simple) filter
  // to produce composite output. In this case, ExecuteDataStart() should
  // NOT Initialize() the composite output.
  int InLocalLoop;

  virtual void ExecuteSimpleAlgorithm(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec, int compositePort);

  virtual void ExecuteEach(vtkCompositeDataIterator* iter, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec, int compositePort, int connection, vtkInformation* request,
    std::vector<vtkSmartPointer<vtkCompositeDataSet>>& compositeOutput);

  std::vector<vtkDataObject*> ExecuteSimpleAlgorithmForBlock(vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec, vtkInformation* inInfo, vtkInformation* request,
    vtkDataObject* dobj);

  bool ShouldIterateOverInput(vtkInformationVector** inInfoVec, int& compositePort);

  int InputTypeIsValid(int port, int index, vtkInformationVector** inInfoVec) override;

  vtkInformation* InformationCache;

  vtkInformation* GenericRequest;
  vtkInformation* InformationRequest;

  void ResetPipelineInformation(int port, vtkInformation*) override;

  /**
   * Tries to create the best possible composite data output for the given input
   * and non-composite algorithm output. Returns a new instance on success.
   * It's main purpose is
   * to determine if vtkHierarchicalBoxDataSet can be propagated as
   * vtkHierarchicalBoxDataSet in the output (if the algorithm can produce
   * vtkUniformGrid given vtkUniformGrid inputs) or if it should be downgraded
   * to a vtkMultiBlockDataSet.
   */
  std::vector<vtkSmartPointer<vtkDataObject>> CreateOutputCompositeDataSet(
    vtkCompositeDataSet* input, int compositePort, int numOutputPorts);

  // Override this to handle UPDATE_COMPOSITE_INDICES().
  void MarkOutputsGenerated(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec) override;

  int NeedToExecuteBasedOnCompositeIndices(vtkInformation* outInfo);

  // Because we sometimes have to swap between "simple" data types and composite
  // data types, we sometimes want to skip resetting the pipeline information.
  static vtkInformationIntegerKey* SUPPRESS_RESET_PI();

  /**
   * COMPOSITE_INDICES() is put in the output information by the executive if
   * the request has UPDATE_COMPOSITE_INDICES() using the generated composite
   * dataset's structure.
   * Note that COMPOSITE_INDICES has to be sorted vector with increasing
   * indices.
   * *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
   */
  static vtkInformationIntegerVectorKey* DATA_COMPOSITE_INDICES();

private:
  vtkCompositeDataPipeline(const vtkCompositeDataPipeline&) = delete;
  void operator=(const vtkCompositeDataPipeline&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
