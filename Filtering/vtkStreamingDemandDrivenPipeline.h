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

#ifndef __vtkStreamingDemandDrivenPipeline_h
#define __vtkStreamingDemandDrivenPipeline_h

#include "vtkDemandDrivenPipeline.h"

#define VTK_UPDATE_EXTENT_COMBINE 1
#define VTK_UPDATE_EXTENT_REPLACE 2

class vtkExtentTranslator;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationIdTypeKey;

class VTK_FILTERING_EXPORT vtkStreamingDemandDrivenPipeline : public vtkDemandDrivenPipeline
{
public:
  static vtkStreamingDemandDrivenPipeline* New();
  vtkTypeMacro(vtkStreamingDemandDrivenPipeline,vtkDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generalized interface for asking the executive to fullfill update
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
  // Set/Get the maximum number of pieces that can be requested from
  // the given port.  The maximum number of pieces is meta data for
  // unstructured data sets.  It gets set by the source during the
  // update information call.  A value of -1 indicates that there is
  // no maximum.
  int SetMaximumNumberOfPieces(int port, int n);
  int SetMaximumNumberOfPieces(vtkInformation *, int n);
  int GetMaximumNumberOfPieces(int port);
  int GetMaximumNumberOfPieces(vtkInformation *);

  // Description:
  // Set/Get the whole extent of an output port.  The whole extent is
  // meta data for structured data sets.  It gets set by the algorithm
  // during the update information pass.
  int SetWholeExtent(vtkInformation *, int extent[6]);
  void GetWholeExtent(vtkInformation *, int extent[6]);
  int* GetWholeExtent(vtkInformation *);

  // Description:
  // If the whole input extent is required to generate the requested output
  // extent, this method can be called to set the input update extent to the
  // whole input extent. This method assumes that the whole extent is known
  // (that UpdateInformation has been called)
  int SetUpdateExtentToWholeExtent(int port);
  int SetUpdateExtentToWholeExtent(vtkInformation *);
  
  // Description:
  // Get/Set the update extent for output ports that use 3D extents.
  int SetUpdateExtent(int port, int extent[6]);
  int SetUpdateExtent(vtkInformation *, int extent[6]);
  void GetUpdateExtent(vtkInformation *, int extent[6]);
  int* GetUpdateExtent(vtkInformation *);

  // Description:
  // Set/Get the update piece, update number of pieces, and update
  // number of ghost levels for an output port.  Similar to update
  // extent in 3D.
  int SetUpdateExtent(int port, 
                      int piece, int numPieces, int ghostLevel);
  int SetUpdateExtent(vtkInformation *, 
                      int piece, int numPieces, int ghostLevel);
  int SetUpdatePiece(vtkInformation *, int piece);
  int GetUpdatePiece(vtkInformation *);
  int SetUpdateNumberOfPieces(vtkInformation *, int n);
  int GetUpdateNumberOfPieces(vtkInformation *);
  int SetUpdateGhostLevel(vtkInformation *, int n);
  int GetUpdateGhostLevel(vtkInformation *);
  int SetUpdateResolution(int port, double r);
  int SetUpdateResolution(vtkInformation *, double r);
  double GetUpdateResolution(vtkInformation *);

  // Description:
  // Get/Set the update extent for output ports that use Temporal Extents
  int SetUpdateTimeSteps(int port, double *times, int length);
  int SetUpdateTimeSteps(vtkInformation *, double *times, int length);
  int SetUpdateTimeStep(int port, double time);
  //void GetUpdateTimeSteps(vtkInformation *, int extent[6]);

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
  // Get/Set the object that will translate pieces into structured
  // extents for an output port.
  int SetExtentTranslator(int port, vtkExtentTranslator* translator);
  int SetExtentTranslator(vtkInformation *, vtkExtentTranslator* translator);
  vtkExtentTranslator* GetExtentTranslator(int port);
  vtkExtentTranslator* GetExtentTranslator(vtkInformation *info);

  // Description:
  // Set/Get the whole bounding box of an output port data object.
  // The whole whole bounding box is meta data for data sets.  It gets
  // set by the algorithm during the update information pass.
  int SetWholeBoundingBox(int port, double bb[6]);
  void GetWholeBoundingBox(int port, double bb[6]);
  double* GetWholeBoundingBox(int port);

  // Description:
  // Set/Get the piece bounding box of an output port data object.
  // The piece bounding box is meta data for data sets.  It gets
  // set by the algorithm during the update extent information pass.
  int SetPieceBoundingBox(int port, double bb[6]);
  void GetPieceBoundingBox(int port, double bb[6]);
  double* GetPieceBoundingBox(int port);

  // Description:
  // Key defining a request to propagate the update extent upstream.
  static vtkInformationRequestKey* REQUEST_UPDATE_EXTENT();

  // Description:
  // Key defining a request to propagate information about the update
  // extent downstream.
  static vtkInformationRequestKey* REQUEST_UPDATE_EXTENT_INFORMATION();

  // Description:
  // Key defining to propagate resolution changes up the pipeline.
  static vtkInformationRequestKey* REQUEST_RESOLUTION_PROPAGATE();

  // Description:
  // Key for an algorithm to store in a request to tell this executive
  // to keep executing it.
  static vtkInformationIntegerKey* CONTINUE_EXECUTING();

  // Description:
  // Key to store an extent translator in pipeline information.
  static vtkInformationObjectBaseKey* EXTENT_TRANSLATOR();

  // Description:
  // Keys to store an update request in pipeline information.
  static vtkInformationIntegerKey* UPDATE_EXTENT_INITIALIZED();
  static vtkInformationIntegerVectorKey* UPDATE_EXTENT();
  static vtkInformationIntegerKey* UPDATE_PIECE_NUMBER();
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_PIECES();
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_GHOST_LEVELS();

  // Description:
  // Key for combining the update extents requested by all consumers,
  // so that the final extent that is produced satisfies all consumers.
  static vtkInformationIntegerVectorKey* COMBINED_UPDATE_EXTENT();

  // Description:
  // This is set if the extent was set through extent translation.
  // GenerateGhostLevelArray() is called only when this is set.
  static vtkInformationIntegerKey* UPDATE_EXTENT_TRANSLATED();

  // Description:
  // Key to store the whole extent provided in pipeline information.
  static vtkInformationIntegerVectorKey* WHOLE_EXTENT();

  // Description:
  // This is set if the update extent is not restricted to the
  // whole extent, for sources that can generate an extent of
  // any requested size.
  static vtkInformationIntegerKey* UNRESTRICTED_UPDATE_EXTENT();

  // Description:
  // Key to store the maximum number of pieces provided in pipeline
  // information.
  static vtkInformationIntegerKey* MAXIMUM_NUMBER_OF_PIECES();

  // Description:
  // Key to store the bounding box of the entire data set in pipeline
  // information.
  static vtkInformationDoubleVectorKey* WHOLE_BOUNDING_BOX();

  // Description:
  // Key to store the bounding box of a portion of the data set in 
  // pipeline information.
  static vtkInformationDoubleVectorKey* PIECE_BOUNDING_BOX();

  // Description:
  // Key to specify the request for exact extent in pipeline information.
  static vtkInformationIntegerKey* EXACT_EXTENT();

  // Description:
  // Key to store available time steps.
  static vtkInformationDoubleVectorKey* TIME_STEPS();

  // Description:
  // Key to store available time range for continuous sources.
  static vtkInformationDoubleVectorKey* TIME_RANGE();

  // Description:
  // Update time steps requested by the pipeline.
  static vtkInformationDoubleVectorKey* UPDATE_TIME_STEPS();

  // Description:
  // Key that specifies from 0.0 to 1.0 the pipeline computed priority 
  // of this update extent. 0.0 means does not contribute and can 
  // be skipped.
  static vtkInformationDoubleKey* PRIORITY();

  // Description:
  // Key that specifies a requested resolution level for this update
  // extent. 0.0 is very low and 1.0 is full resolution.
  static vtkInformationDoubleKey* UPDATE_RESOLUTION();

  // Description:
  // Used internally to validate meta information as it flows through pipeline
  static vtkInformationIntegerKey* REMOVE_ATTRIBUTE_INFORMATION();

  // Description:
  // The following keys are meant to be used by an algorithm that 
  // works with temporal data. Rather than re-executing the pipeline
  // for each timestep, if the reader, as part of its API, contains
  // a faster way to read temporal data, algorithms may use these
  // keys to request temporal data from the reader.
  // See also: vtkExtractArraysOverTime. 

  // Key to allow a reader to advertise that it supports a fast-path
  // for reading data over time.
  static vtkInformationIntegerKey* FAST_PATH_FOR_TEMPORAL_DATA();
  // The type of data being requested.
  // Possible values: POINT, CELL, EDGE, FACE
  static vtkInformationStringKey* FAST_PATH_OBJECT_TYPE();
  // Possible values: INDEX, GLOBAL
  static vtkInformationStringKey* FAST_PATH_ID_TYPE();
  // The id (either index or global id) being requested
  static vtkInformationIdTypeKey* FAST_PATH_OBJECT_ID();

  // Description:
  // Issues pipeline request to determine and return the priority of the 
  // piece described by the current update extent. The priority is a 
  // number between 0.0 and 1.0 with 0 meaning skippable (REQUEST_DATA 
  // not needed) and 1.0 meaning important. 
  double ComputePriority()
    {
      return this->ComputePriority(0);
    }
  virtual double ComputePriority(int port);

protected:
  vtkStreamingDemandDrivenPipeline();
  ~vtkStreamingDemandDrivenPipeline();

  // Description:
  // Called before RequestUpdateExtent() pass on the algorithm. Here we remove
  // all update-related keys from the input information.
  // Currently this only removes the fast-path related keys.
  virtual void ResetUpdateInformation(vtkInformation* request,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec);

  // Keep track of the update time request corresponding to the
  // previous executing. If the previous update request did not
  // correspond to an existing time step and the reader chose 
  // a time step with it's own logic, the data time step will
  // be different than the request. If the same time step is
  // requested again, there is no need to re-execute the algorithm.
  // We know that it does not have this time step.
  static vtkInformationDoubleVectorKey* PREVIOUS_UPDATE_TIME_STEPS();

  // Keep track of the fast path keys corresponding to the 
  // previous executing. If all key values are the same as their
  // counterparts in the previous request, we do not need to re-execute.
  static vtkInformationIdTypeKey* PREVIOUS_FAST_PATH_OBJECT_ID();
  static vtkInformationStringKey* PREVIOUS_FAST_PATH_OBJECT_TYPE();
  static vtkInformationStringKey* PREVIOUS_FAST_PATH_ID_TYPE();

  // Does the time request correspond to what is in the data?
  // Returns 0 if yes, 1 otherwise.
  virtual int NeedToExecuteBasedOnTime(vtkInformation* outInfo,
                                       vtkDataObject* dataObject);

  // If the request contains a fast path key for temporal data, always execute
  virtual int NeedToExecuteBasedOnFastPathData(vtkInformation* outInfo);

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
