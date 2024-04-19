// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDemandDrivenPipeline
 * @brief   Executive supporting on-demand execution.
 *
 * vtkDemandDrivenPipeline is an executive that will execute an
 * algorithm only when its outputs are out-of-date with respect to its
 * inputs.
 */

#ifndef vtkDemandDrivenPipeline_h
#define vtkDemandDrivenPipeline_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkExecutive.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataArray;
class vtkDataSetAttributes;
class vtkDemandDrivenPipelineInternals;
class vtkFieldData;
class vtkInformation;
class vtkInformationIntegerKey;
class vtkInformationVector;
class vtkInformationKeyVectorKey;
class vtkInformationUnsignedLongKey;

///\defgroup InformationKeys Information Keys
/// The VTK pipeline relies on algorithms providing information about their
/// input and output and responding to requests.  The information objects used
/// to perform these actions map known keys to values.  This is a list of keys
/// that information objects use and what each key should be used for.
///

class VTKCOMMONEXECUTIONMODEL_EXPORT VTK_MARSHALAUTO vtkDemandDrivenPipeline : public vtkExecutive
{
public:
  static vtkDemandDrivenPipeline* New();
  vtkTypeMacro(vtkDemandDrivenPipeline, vtkExecutive);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Generalized interface for asking the executive to fulfill update
   * requests.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  /**
   * Implement the pipeline modified time request.
   */
  int ComputePipelineMTime(vtkInformation* request, vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec, int requestFromOutputPort, vtkMTimeType* mtime) override;

  ///@{
  /**
   * Bring the algorithm's outputs up-to-date.  Returns 1 for success
   * and 0 for failure.
   */
  vtkTypeBool Update() override;
  vtkTypeBool Update(int port) override;
  ///@}

  ///@{
  /**
   * Get the PipelineMTime for this exective.
   */
  vtkGetMacro(PipelineMTime, vtkMTimeType);
  ///@}

  /**
   * Set whether the given output port releases data when it is
   * consumed.  Returns 1 if the value changes and 0 otherwise.
   */
  virtual int SetReleaseDataFlag(int port, vtkTypeBool n);

  /**
   * Get whether the given output port releases data when it is consumed.
   */
  virtual vtkTypeBool GetReleaseDataFlag(int port);

  /**
   * Bring the PipelineMTime up to date.
   */
  virtual int UpdatePipelineMTime();

  /**
   * Bring the output data object's existence up to date.  This does
   * not actually produce data, but does create the data object that
   * will store data produced during the UpdateData step.
   */
  int UpdateDataObject() override;

  /**
   * Bring the output information up to date.
   */
  int UpdateInformation() override;

  /**
   * Bring the output data up to date.  This should be called only
   * when information is up to date.  Use the Update method if it is
   * not known that the information is up to date.
   */
  virtual int UpdateData(int outputPort);

  /**
   * Key defining a request to make sure the output data objects exist.
   * @ingroup InformationKeys
   */
  static vtkInformationRequestKey* REQUEST_DATA_OBJECT();

  /**
   * Key defining a request to make sure the output information is up to date.
   * @ingroup InformationKeys
   */
  static vtkInformationRequestKey* REQUEST_INFORMATION();

  /**
   * Key defining a request to make sure the output data are up to date.
   * @ingroup InformationKeys
   */
  static vtkInformationRequestKey* REQUEST_DATA();

  /**
   * Key defining a request to mark outputs that will NOT be generated
   * during a REQUEST_DATA.
   * @ingroup InformationKeys
   */
  static vtkInformationRequestKey* REQUEST_DATA_NOT_GENERATED();

  /**
   * Key to specify in pipeline information the request that data be
   * released after it is used.
   * @ingroup InformationKeys
   */
  static vtkInformationIntegerKey* RELEASE_DATA();

  /**
   * Key to store a mark for an output that will not be generated.
   * Algorithms use this to tell the executive that they will not
   * generate certain outputs for a REQUEST_DATA.
   * @ingroup InformationKeys
   */
  static vtkInformationIntegerKey* DATA_NOT_GENERATED();

  /**
   * Create (New) and return a data object of the given type.
   * This is here for backwards compatibility. Use
   * vtkDataObjectTypes::NewDataObject() instead.
   */
  static vtkDataObject* NewDataObject(const char* type);

protected:
  vtkDemandDrivenPipeline();
  ~vtkDemandDrivenPipeline() override;

  // Helper methods to send requests to the algorithm.
  virtual int ExecuteDataObject(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo);
  virtual int ExecuteInformation(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo);
  virtual int ExecuteData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo);

  // Reset the pipeline update values in the given output information object.
  void ResetPipelineInformation(int, vtkInformation*) override;

  // Check whether the data object in the pipeline information for an
  // output port exists and has a valid type.
  virtual int CheckDataObject(int port, vtkInformationVector* outInfo);

  // Input connection validity checkers.
  int InputCountIsValid(vtkInformationVector**);
  int InputCountIsValid(int port, vtkInformationVector**);
  int InputTypeIsValid(vtkInformationVector**);
  int InputTypeIsValid(int port, vtkInformationVector**);
  virtual int InputTypeIsValid(int port, int index, vtkInformationVector**);
  int InputFieldsAreValid(vtkInformationVector**);
  int InputFieldsAreValid(int port, vtkInformationVector**);
  virtual int InputFieldsAreValid(int port, int index, vtkInformationVector**);

  // Field existence checkers.
  int DataSetAttributeExists(vtkDataSetAttributes* dsa, vtkInformation* field);
  int FieldArrayExists(vtkFieldData* data, vtkInformation* field);
  int ArrayIsValid(vtkAbstractArray* array, vtkInformation* field);

  // Input port information checkers.
  int InputIsOptional(int port);
  int InputIsRepeatable(int port);

  // Decide whether the output data need to be generated.
  virtual int NeedToExecuteData(
    int outputPort, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec);

  // Handle before/after operations for ExecuteData method.
  virtual void ExecuteDataStart(
    vtkInformation* request, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec);
  virtual void ExecuteDataEnd(
    vtkInformation* request, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec);
  virtual void MarkOutputsGenerated(
    vtkInformation* request, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec);

  // Largest MTime of any algorithm on this executive or preceding
  // executives.
  vtkMTimeType PipelineMTime;

  // Time when information or data were last generated.
  vtkTimeStamp DataObjectTime;
  vtkTimeStamp InformationTime;
  vtkTimeStamp DataTime;

  friend class vtkCompositeDataPipeline;

  vtkInformation* InfoRequest;
  vtkInformation* DataObjectRequest;
  vtkInformation* DataRequest;

private:
  vtkDemandDrivenPipeline(const vtkDemandDrivenPipeline&) = delete;
  void operator=(const vtkDemandDrivenPipeline&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
