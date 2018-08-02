/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingDemandDrivenPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStreamingDemandDrivenPipeline
 * @brief   Executive supporting partial updates.
 *
 * vtkStreamingDemandDrivenPipeline is an executive that supports
 * updating only a portion of the data set in the pipeline.  This is
 * the style of pipeline update that is provided by the old-style VTK
 * 4.x pipeline.  Instead of always updating an entire data set, this
 * executive supports asking for pieces or sub-extents.
*/

#ifndef vtkStreamingDemandDrivenPipeline_h
#define vtkStreamingDemandDrivenPipeline_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkDemandDrivenPipeline.h"

#define VTK_UPDATE_EXTENT_COMBINE 1
#define VTK_UPDATE_EXTENT_REPLACE 2

class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIdTypeKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
class vtkInformationIterator;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationStringKey;
class vtkInformationUnsignedLongKey;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkStreamingDemandDrivenPipeline : public vtkDemandDrivenPipeline
{
public:
  static vtkStreamingDemandDrivenPipeline* New();
  vtkTypeMacro(vtkStreamingDemandDrivenPipeline,vtkDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Generalized interface for asking the executive to fulfill update
   * requests.
   */
  int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo) override;

  //@{
  /**
   * Bring the outputs up-to-date.
   */
  int Update() override;
  int Update(int port) override;
  virtual int UpdateWholeExtent();
  //@}

  /**
   * This method enables the passing of data requests to the algorithm
   * to be used during execution (in addition to bringing a particular
   * port up-to-date). The requests argument should contain an information
   * object for each port that requests need to be passed. For each
   * of those, the pipeline will copy all keys to the output information
   * before execution. This is equivalent to:
   * \verbatim
   * executive->UpdateInformation();
   * for (int i=0; i<executive->GetNumberOfOutputPorts(); i++)
   * {
   * vtkInformation* portRequests = requests->GetInformationObject(i);
   * if (portRequests)
   * {
   * executive->GetOutputInformation(i)->Append(portRequests);
   * }
   * }
   * executive->Update();
   * \endverbatim
   * Available requests include UPDATE_PIECE_NUMBER(), UPDATE_NUMBER_OF_PIECES()
   * UPDATE_EXTENT() etc etc.
   */
  virtual int Update(int port, vtkInformationVector* requests);

  /**
   * Propagate the update request from the given output port back
   * through the pipeline.  Should be called only when information is
   * up to date.
   */
  int PropagateUpdateExtent(int outputPort);

  //@{
  /**
   * Propagate time through the pipeline. this is a special pass
   * only necessary if there is temporal meta data that must be updated
   */
  int PropagateTime(int outputPort);
  int UpdateTimeDependentInformation(int outputPort);
  //@}

  //@{
  /**
   * Set/Get the whole extent of an output port.  The whole extent is
   * meta data for structured data sets.  It gets set by the algorithm
   * during the update information pass.
   */
  static int SetWholeExtent(vtkInformation *, int extent[6]);
  static void GetWholeExtent(vtkInformation *, int extent[6]);
  static int* GetWholeExtent(vtkInformation *) VTK_SIZEHINT(6);
  //@}

  //@{
  /**
   * This request flag indicates whether the requester can handle more
   * data than requested for the given port.  Right now it is used in
   * vtkImageData.  Image filters can return more data than requested.
   * The consumer cannot handle this (i.e. DataSetToDataSetfilter)
   * the image will crop itself.  This functionality used to be in
   * ImageToStructuredPoints.
   */
  int SetRequestExactExtent(int port, int flag);
  int GetRequestExactExtent(int port);
  //@}

  /**
   * Key defining a request to propagate the update extent upstream.
   * \ingroup InformationKeys
   */
  static vtkInformationRequestKey* REQUEST_UPDATE_EXTENT();

  /**
   * Key defining a request to propagate the update extent upstream.
   * \ingroup InformationKeys
   */
  static vtkInformationRequestKey* REQUEST_UPDATE_TIME();
  /**
   * Key defining a request to make sure the meta information is up to date.
   * \ingroup InformationKeys
   */
  static vtkInformationRequestKey* REQUEST_TIME_DEPENDENT_INFORMATION();

  /**
   * Key for an algorithm to store in a request to tell this executive
   * to keep executing it.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* CONTINUE_EXECUTING();

  /**
   * Keys to store an update request in pipeline information.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* UPDATE_EXTENT_INITIALIZED();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerVectorKey* UPDATE_EXTENT();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* UPDATE_PIECE_NUMBER();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_PIECES();
  /**
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_GHOST_LEVELS();

  /**
   * Key for combining the update extents requested by all consumers,
   * so that the final extent that is produced satisfies all consumers.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerVectorKey* COMBINED_UPDATE_EXTENT();

  /**
   * Key to store the whole extent provided in pipeline information.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerVectorKey* WHOLE_EXTENT();

  /**
   * This is set if the update extent is not restricted to the
   * whole extent, for sources that can generate an extent of
   * any requested size.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* UNRESTRICTED_UPDATE_EXTENT();

  /**
   * Key to specify the request for exact extent in pipeline information.
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* EXACT_EXTENT();

  /**
   * Key to store available time steps.
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleVectorKey* TIME_STEPS();

  /**
   * Key to store available time range for continuous sources.
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleVectorKey* TIME_RANGE();

  /**
   * Update time steps requested by the pipeline.
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleKey* UPDATE_TIME_STEP();

  /**
   * Whether there are time dependent meta information
   * if there is, the pipeline will perform two extra passes
   * to gather the time dependent information
   * \ingroup InformationKeys
   */
  static vtkInformationIntegerKey* TIME_DEPENDENT_INFORMATION();

  /**
   * key to record the bounds of a dataset.
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleVectorKey *BOUNDS();

  //@{
  /**
   * Get/Set the update extent for output ports that use 3D extents.
   */
  static void GetUpdateExtent(vtkInformation *, int extent[6]);
  static int* GetUpdateExtent(vtkInformation *);
  //@}
  //@{
  /**
   * Set/Get the update piece, update number of pieces, and update
   * number of ghost levels for an output port.  Similar to update
   * extent in 3D.
   */
  static int GetUpdatePiece(vtkInformation *);
  static int GetUpdateNumberOfPieces(vtkInformation *);
  static int GetUpdateGhostLevel(vtkInformation *);
  //@}

protected:
  vtkStreamingDemandDrivenPipeline();
  ~vtkStreamingDemandDrivenPipeline() override;

  /**
   * Keep track of the update time request corresponding to the
   * previous executing. If the previous update request did not
   * correspond to an existing time step and the reader chose
   * a time step with it's own logic, the data time step will
   * be different than the request. If the same time step is
   * requested again, there is no need to re-execute the algorithm.
   * We know that it does not have this time step.
   * \ingroup InformationKeys
   */
  static vtkInformationDoubleKey* PREVIOUS_UPDATE_TIME_STEP();

  // Does the time request correspond to what is in the data?
  // Returns 0 if yes, 1 otherwise.
  virtual int NeedToExecuteBasedOnTime(vtkInformation* outInfo,
                                       vtkDataObject* dataObject);

  // Setup default information on the output after the algorithm
  // executes information.
  int ExecuteInformation(vtkInformation* request,
                                 vtkInformationVector** inInfoVec,
                                 vtkInformationVector* outInfoVec) override;

  // Copy information for the given request.
  void CopyDefaultInformation(vtkInformation* request, int direction,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec) override;

  // Helper to check output information before propagating it to inputs.
  virtual int VerifyOutputInformation(int outputPort,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec);


  // Override this check to account for update extent.
  int NeedToExecuteData(int outputPort,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec) override;

  // Override these to handle the continue-executing option.
  void ExecuteDataStart(vtkInformation* request,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec) override;
  void ExecuteDataEnd(vtkInformation* request,
                              vtkInformationVector** inInfoVec,
                              vtkInformationVector* outInfoVec) override;

  // Override this to handle cropping and ghost levels.
  void MarkOutputsGenerated(vtkInformation* request,
                                    vtkInformationVector** inInfoVec,
                                    vtkInformationVector* outInfoVec) override;


  // Remove update/whole extent when resetting pipeline information.
  void ResetPipelineInformation(int port, vtkInformation*) override;

  // Flag for when an algorithm returns with CONTINUE_EXECUTING in the
  // request.
  int ContinueExecuting;

  vtkInformation *UpdateExtentRequest;
  vtkInformation *UpdateTimeRequest;
  vtkInformation *TimeDependentInformationRequest;
  vtkInformationIterator *InformationIterator;

  // did the most recent PUE do anything ?
  int LastPropogateUpdateExtentShortCircuited;

private:
  vtkStreamingDemandDrivenPipeline(const vtkStreamingDemandDrivenPipeline&) = delete;
  void operator=(const vtkStreamingDemandDrivenPipeline&) = delete;
};

#endif
