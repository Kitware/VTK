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
// .NAME vtkStreamingDemandDrivenPipeline -
// .SECTION Description
// vtkStreamingDemandDrivenPipeline

#ifndef __vtkStreamingDemandDrivenPipeline_h
#define __vtkStreamingDemandDrivenPipeline_h

#include "vtkDemandDrivenPipeline.h"

class vtkExtentTranslator;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
class vtkInformationObjectBaseKey;
class vtkStreamingDemandDrivenPipelineInternals;

class VTK_FILTERING_EXPORT vtkStreamingDemandDrivenPipeline : public vtkDemandDrivenPipeline
{
public:
  static vtkStreamingDemandDrivenPipeline* New();
  vtkTypeRevisionMacro(vtkStreamingDemandDrivenPipeline,vtkDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bring the given algorithm's outputs up-to-date.  The algorithm
  // must already be managed by this executive.
  virtual int Update();
  virtual int Update(int port);
  virtual int Update(vtkAlgorithm* algorithm);
  virtual int UpdateWholeExtent(vtkAlgorithm* algorithm);
  virtual int Update(vtkAlgorithm* algorithm, int port);

  static vtkInformationIntegerKey* CONTINUE_EXECUTING();
  static vtkInformationIntegerKey* REQUEST_UPDATE_EXTENT();
  static vtkInformationIntegerVectorKey* WHOLE_EXTENT();
  static vtkInformationIntegerKey* MAXIMUM_NUMBER_OF_PIECES();
  static vtkInformationIntegerKey* UPDATE_EXTENT_INITIALIZED();
  static vtkInformationIntegerVectorKey* UPDATE_EXTENT();
  static vtkInformationIntegerKey* UPDATE_PIECE_NUMBER();
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_PIECES();
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_GHOST_LEVELS();
  static vtkInformationObjectBaseKey* EXTENT_TRANSLATOR();

  int PropagateUpdateExtent(int outputPort);

  int SetMaximumNumberOfPieces(int port, int n);
  int GetMaximumNumberOfPieces(int port);
  int SetWholeExtent(int port, int extent[6]);
  void GetWholeExtent(int port, int extent[6]);
  int* GetWholeExtent(int port);
  int SetUpdateExtentToWholeExtent(int port);
  int SetUpdateExtent(int port, int extent[6]);
  void GetUpdateExtent(int port, int extent[6]);
  int* GetUpdateExtent(int port);
  int SetUpdateExtent(int port, int piece, int numPieces, int ghostLevel);
  int SetUpdatePiece(int port, int piece);
  int GetUpdatePiece(int port);
  int SetUpdateNumberOfPieces(int port, int n);
  int GetUpdateNumberOfPieces(int port);
  int SetUpdateGhostLevel(int port, int n);
  int GetUpdateGhostLevel(int port);
  int SetExtentTranslator(int port, vtkExtentTranslator* translator);
  vtkExtentTranslator* GetExtentTranslator(int port);

protected:
  vtkStreamingDemandDrivenPipeline();
  ~vtkStreamingDemandDrivenPipeline();

  virtual int ExecuteInformation();
  virtual void CopyDefaultDownstreamInformation();
  virtual void CopyDefaultUpstreamInformation(int outputPort);
  int VerifyOutputInformation(int outputPort);
  virtual int NeedToExecuteData(int outputPort);

  // Put default information in output information objects.
  virtual void FillDefaultOutputInformation(int port, vtkInformation*);

  // Reset the pipeline update values in the given output information object.
  virtual void ResetPipelineInformation(int port, vtkInformation*);

private:
  vtkStreamingDemandDrivenPipelineInternals* StreamingDemandDrivenInternal;
private:
  vtkStreamingDemandDrivenPipeline(const vtkStreamingDemandDrivenPipeline&);  // Not implemented.
  void operator=(const vtkStreamingDemandDrivenPipeline&);  // Not implemented.
};

#endif
