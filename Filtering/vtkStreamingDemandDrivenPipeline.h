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
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
class vtkInformationObjectBaseKey;

class VTK_FILTERING_EXPORT vtkStreamingDemandDrivenPipeline : public vtkDemandDrivenPipeline
{
public:
  static vtkStreamingDemandDrivenPipeline* New();
  vtkTypeRevisionMacro(vtkStreamingDemandDrivenPipeline,vtkDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generalized interface for asking the executive to fullfill update
  // requests.
  virtual int ProcessRequest(vtkInformation* request);

  // Description:
  // Bring the algorithm's outputs up-to-date.
  virtual int Update();
  virtual int Update(int port);
  virtual int UpdateWholeExtent();

  static vtkInformationIntegerKey* CONTINUE_EXECUTING();
  static vtkInformationObjectBaseKey* EXTENT_TRANSLATOR();
  static vtkInformationIntegerKey* MAXIMUM_NUMBER_OF_PIECES();
  static vtkInformationIntegerKey* REQUEST_UPDATE_EXTENT();
  static vtkInformationIntegerKey* UPDATE_EXTENT_INITIALIZED();
  static vtkInformationIntegerVectorKey* UPDATE_EXTENT();
  static vtkInformationIntegerKey* UPDATE_PIECE_NUMBER();
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_PIECES();
  static vtkInformationIntegerKey* UPDATE_NUMBER_OF_GHOST_LEVELS();
  static vtkInformationIntegerVectorKey* WHOLE_EXTENT();
  static vtkInformationDoubleVectorKey* WHOLE_BOUNDING_BOX();
  static vtkInformationIntegerKey* EXACT_EXTENT();

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
  int SetRequestExactExtent(int port, int flag);
  int GetRequestExactExtent(int port);
  int SetExtentTranslator(int port, vtkExtentTranslator* translator);
  vtkExtentTranslator* GetExtentTranslator(int port);
  int SetWholeBoundingBox(int port, double bb[6]);
  void GetWholeBoundingBox(int port, double bb[6]);
  double* GetWholeBoundingBox(int port);

protected:
  vtkStreamingDemandDrivenPipeline();
  ~vtkStreamingDemandDrivenPipeline();

  virtual int ExecuteInformation();

  // Copy information for the given request.
  virtual void CopyDefaultInformation(vtkInformation* request, int direction);

  int VerifyOutputInformation(int outputPort);
  virtual int NeedToExecuteData(int outputPort);

  // Reset the pipeline update values in the given output information object.
  virtual void ResetPipelineInformation(int port, vtkInformation*);

private:
  vtkStreamingDemandDrivenPipeline(const vtkStreamingDemandDrivenPipeline&);  // Not implemented.
  void operator=(const vtkStreamingDemandDrivenPipeline&);  // Not implemented.
};

#endif
