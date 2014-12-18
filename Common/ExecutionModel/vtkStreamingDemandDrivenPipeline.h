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
// .NAME vtkStreamingDemandDrivenPipeline - Executive supporting partial updates.
// .SECTION Description
// vtkStreamingDemandDrivenPipeline is an executive that supports
// updating only a portion of the data set in the pipeline.  This is
// the style of pipeline update that is provided by the old-style VTK
// 4.x pipeline.  Instead of always updating an entire data set, this
// executive supports asking for pieces or sub-extents.

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
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationStringKey;
class vtkInformationUnsignedLongKey;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkStreamingDemandDrivenPipeline : public vtkDemandDrivenPipeline
{
public:
  static vtkStreamingDemandDrivenPipeline* New();
  vtkTypeMacro(vtkStreamingDemandDrivenPipeline,vtkDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generalized interface for asking the executive to fulfill update
  // requests.
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

  // Description:
  // Bring the outputs up-to-date.
  virtual int Update();
  virtual int Update(int port);
  virtual int UpdateWholeExtent();

  // Description:
  // Propagate the update request from the given output port back
  // through the pipeline.  Should be called only when information is
  // up to date.
  int PropagateUpdateExtent(int outputPort);


  // Description:
  // Propagate time through the pipeline. this is a special pass
  // only necessary if there is temporal meta data that must be updated
  int PropagateTime(int outputPort);
  int UpdateTimeDependentInformation(int outputPort);

  // Description:
  // Set/Get the whole extent of an output port.  The whole extent is
  // meta data for structured data sets.  It gets set by the algorithm
  // during the update information pass.
  static int SetWholeExtent(vtkInformation *, int extent[6]);
  static void GetWholeExtent(vtkInformation *, int extent[6]);
  static int* GetWholeExtent(vtkInformation *);

  // Description:
  // If the whole input extent is required to generate the requested output
  // extent, this method can be called to set the input update extent to the
  // whole input extent. This method assumes that the whole extent is known
  // (that UpdateInformation has been called)
  int SetUpdateExtentToWholeExtent(int port);
  static int SetUpdateExtentToWholeExtent(vtkInformation *);

  // Description:
  // Get/Set the update extent for output ports that use 3D extents.
  int SetUpdateExtent(int port, int extent[6]);
  int SetUpdateExtent(int port, int x0, int x1, int y0, int y1, int z0, int z1);
  static int SetUpdateExtent(vtkInformation *, int extent[6]);
  static void GetUpdateExtent(vtkInformation *, int extent[6]);
  static int* GetUpdateExtent(vtkInformation *);

  // Description:
  // Set/Get the update piece, update number of pieces, and update
  // number of ghost levels for an output port.  Similar to update
  // extent in 3D.
  int SetUpdateExtent(int port,
                      int piece, int numPieces, int ghostLevel);
  static int SetUpdateExtent(vtkInformation *,
                             int piece, int numPieces, int ghostLevel);
  static int SetUpdatePiece(vtkInformation *, int piece);
  static int GetUpdatePiece(vtkInformation *);
  static int SetUpdateNumberOfPieces(vtkInformation *, int n);
  static int GetUpdateNumberOfPieces(vtkInformation *);
  static int SetUpdateGhostLevel(vtkInformation *, int n);
  static int GetUpdateGhostLevel(vtkInformation *);

  // Description:
  // Get/Set the update extent for output ports that use Temporal Extents
  int SetUpdateTimeStep(int port, double time);
  static int SetUpdateTimeStep(vtkInformation *, double time);

  // Description:
  // This request flag indicates whether the requester can handle more
  // data than requested for the given port.  Right now it is used in
  // vtkImageData.  Image filters can return more data than requested.
  // The the consumer cannot handle this (i.e. DataSetToDataSetFitler)
  // the image will crop itself.  This functionality used to be in
  // ImageToStructuredPoints.
  int SetRequestExactExtent(int port, int flag);
  int GetRequestExactExtent(int port);

  // Description:
  // Key defining a request to propagate the update extent upstream.
  // \ingroup InformationKeys
  static vtkInformationRequestKey* REQUEST_UPDATE_EXTENT();

  // Description:
  // Key defining a request to propagate the update extent upstream.
  // \ingroup InformationKeys
  static vtkInformationRequestKey* REQUEST_UPDATE_TIME();
  // Description:
  // Key defining a request to make sure the meta information is up to date.
  // \ingroup InformationKeys
  static vtkInformationRequestKey* REQUEST_TIME_DEPENDENT_INFORMATION();

  // Description:
  // Key for an algorithm to store in a request to tell this executive
  // to keep executing it.
  // \ingroup InformationKeys
  static vtkInformationIntegerKey* CONTINUE_EXECUTING();

  // Description:
  // Keys to store an update request in pipeline information.
  // \ingroup InformationKeys
  static vtkInformationIntegerKey* UPDATE_EXTENT_INITIALIZED();
  // Description:
  // \ingroup InformationKeys
  static vtkInformationIntegerVectorKey* UPDATE_EXTENT();
  // Description:
  // \ingroup InformationKeys
  static vtkInformationIntegerKey* UPDATE_PIECE_NUMBER();
  // Description:
  // \ingroup InformationKeys
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_PIECES();
  // Description:
  // \ingroup InformationKeys
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_GHOST_LEVELS();

  // Description:
  // Key for combining the update extents requested by all consumers,
  // so that the final extent that is produced satisfies all consumers.
  // \ingroup InformationKeys
  static vtkInformationIntegerVectorKey* COMBINED_UPDATE_EXTENT();

  // Description:
  // Key to store the whole extent provided in pipeline information.
  // \ingroup InformationKeys
  static vtkInformationIntegerVectorKey* WHOLE_EXTENT();

  // Description:
  // This is set if the update extent is not restricted to the
  // whole extent, for sources that can generate an extent of
  // any requested size.
  // \ingroup InformationKeys
  static vtkInformationIntegerKey* UNRESTRICTED_UPDATE_EXTENT();

  // Description:
  // Key to specify the request for exact extent in pipeline information.
  // \ingroup InformationKeys
  static vtkInformationIntegerKey* EXACT_EXTENT();

  // Description:
  // Key to store available time steps.
  // \ingroup InformationKeys
  static vtkInformationDoubleVectorKey* TIME_STEPS();

  // Description:
  // Key to store available time range for continuous sources.
  // \ingroup InformationKeys
  static vtkInformationDoubleVectorKey* TIME_RANGE();

  // Description:
  // Update time steps requested by the pipeline.
  // \ingroup InformationKeys
  static vtkInformationDoubleKey* UPDATE_TIME_STEP();

  // Description:
  // Whether there are time dependent meta information
  // if there is, the pipeline will perform two extra passes
  // to gather the time dependent information
  // \ingroup InformationKeys
  static vtkInformationIntegerKey* TIME_DEPENDENT_INFORMATION();

  // Description:
  // key to record the bounds of a dataset.
  // \ingroup InformationKeys
  static vtkInformationDoubleVectorKey *BOUNDS();

protected:
  vtkStreamingDemandDrivenPipeline();
  ~vtkStreamingDemandDrivenPipeline();

  // Description:
  // Keep track of the update time request corresponding to the
  // previous executing. If the previous update request did not
  // correspond to an existing time step and the reader chose
  // a time step with it's own logic, the data time step will
  // be different than the request. If the same time step is
  // requested again, there is no need to re-execute the algorithm.
  // We know that it does not have this time step.
  // \ingroup InformationKeys
  static vtkInformationDoubleKey* PREVIOUS_UPDATE_TIME_STEP();

  // Does the time request correspond to what is in the data?
  // Returns 0 if yes, 1 otherwise.
  virtual int NeedToExecuteBasedOnTime(vtkInformation* outInfo,
                                       vtkDataObject* dataObject);

  // Setup default information on the output after the algorithm
  // executes information.
  virtual int ExecuteInformation(vtkInformation* request,
                                 vtkInformationVector** inInfoVec,
                                 vtkInformationVector* outInfoVec);

  // Copy information for the given request.
  virtual void CopyDefaultInformation(vtkInformation* request, int direction,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec);

  // Helper to check output information before propagating it to inputs.
  virtual int VerifyOutputInformation(int outputPort,
                                      vtkInformationVector** inInfoVec,
                                      vtkInformationVector* outInfoVec);


  // Override this check to account for update extent.
  virtual int NeedToExecuteData(int outputPort,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec);

  // Override these to handle the continue-executing option.
  virtual void ExecuteDataStart(vtkInformation* request,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec);
  virtual void ExecuteDataEnd(vtkInformation* request,
                              vtkInformationVector** inInfoVec,
                              vtkInformationVector* outInfoVec);

  // Override this to handle cropping and ghost levels.
  virtual void MarkOutputsGenerated(vtkInformation* request,
                                    vtkInformationVector** inInfoVec,
                                    vtkInformationVector* outInfoVec);


  // Remove update/whole extent when resetting pipeline information.
  virtual void ResetPipelineInformation(int port, vtkInformation*);

  // Flag for when an algorithm returns with CONTINUE_EXECUTING in the
  // request.
  int ContinueExecuting;

  vtkInformation *UpdateExtentRequest;

  // did the most recent PUE do anything ?
  int LastPropogateUpdateExtentShortCircuited;

private:
  vtkStreamingDemandDrivenPipeline(const vtkStreamingDemandDrivenPipeline&);  // Not implemented.
  void operator=(const vtkStreamingDemandDrivenPipeline&);  // Not implemented.
};

#endif
