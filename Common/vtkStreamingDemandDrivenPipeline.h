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

class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
class vtkStreamingDemandDrivenPipelineInternals;

class VTK_COMMON_EXPORT vtkStreamingDemandDrivenPipeline : public vtkDemandDrivenPipeline
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

  int PropagateUpdateExtent(int outputPort);

protected:
  vtkStreamingDemandDrivenPipeline();
  ~vtkStreamingDemandDrivenPipeline();

  virtual int ExecuteInformation();
  int VerifyUpdateExtent(int outputPort);
  virtual int NeedToExecuteData(int outputPort);
private:
  vtkStreamingDemandDrivenPipelineInternals* StreamingDemandDrivenInternal;
private:
  vtkStreamingDemandDrivenPipeline(const vtkStreamingDemandDrivenPipeline&);  // Not implemented.
  void operator=(const vtkStreamingDemandDrivenPipeline&);  // Not implemented.
};

#endif
