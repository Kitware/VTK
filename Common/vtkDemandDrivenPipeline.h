/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDemandDrivenPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDemandDrivenPipeline -
// .SECTION Description
// vtkDemandDrivenPipeline

#ifndef __vtkDemandDrivenPipeline_h
#define __vtkDemandDrivenPipeline_h

#include "vtkDistributedExecutive.h"

class vtkDataArray;
class vtkDataSetAttributes;
class vtkDemandDrivenPipelineInternals;
class vtkFieldData;
class vtkInformation;
class vtkInformationIntegerKey;
class vtkInformationVector;

class VTK_COMMON_EXPORT vtkDemandDrivenPipeline : public vtkDistributedExecutive
{
public:
  static vtkDemandDrivenPipeline* New();
  vtkTypeRevisionMacro(vtkDemandDrivenPipeline,vtkDistributedExecutive);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bring the given algorithm's outputs up-to-date.  The algorithm
  // must already be managed by this executive.
  virtual int Update();
  virtual int Update(int port);
  virtual int Update(vtkAlgorithm* algorithm);
  virtual int Update(vtkAlgorithm* algorithm, int port);

  // Description:
  // Get the information object for an output port of an algorithm.
  virtual vtkInformation* GetOutputInformation(int port);
  virtual vtkInformation* GetOutputInformation(vtkAlgorithm* algorithm,
                                               int port);

  // Description:
  // Get the data object storing the output for the given port.
  virtual vtkDataObject* GetOutputData(int port);
  virtual vtkDataObject* GetOutputData(vtkAlgorithm* algorithm, int port);

  static vtkInformationIntegerKey* REQUEST_INFORMATION();
  static vtkInformationIntegerKey* REQUEST_DATA();
  static vtkInformationIntegerKey* FROM_OUTPUT_PORT();

  int UpdateInformation();
  int UpdateData(int outputPort);
protected:
  vtkDemandDrivenPipeline();
  ~vtkDemandDrivenPipeline();

  virtual int ExecuteInformation();
  virtual int ExecuteData(int outputPort);

  vtkDemandDrivenPipeline* GetConnectedInputExecutive(int port, int index);
  vtkInformation* GetConnectedInputInformation(int port, int index);

  vtkInformation* GetRequestInformation();
  vtkInformationVector* GetInputInformation();
  vtkInformation* GetInputInformation(int port);
  vtkInformationVector* GetOutputInformation();

  vtkDataObject* GetInputData(int port, int index);
  void PrepareDownstreamRequest(vtkInformationIntegerKey* rkey);
  void PrepareUpstreamRequest(vtkInformationIntegerKey* rkey);

  // Input connection validity checkers.
  int InputCountIsValid();
  int InputCountIsValid(int port);
  int InputTypeIsValid();
  int InputTypeIsValid(int port);
  int InputTypeIsValid(int port, int index);
  int InputFieldsAreValid();
  int InputFieldsAreValid(int port);
  int InputFieldsAreValid(int port, int index);

  // Field existence checkers.
  int DataSetAttributeExists(vtkDataSetAttributes* dsa, vtkInformation* field);
  int FieldArrayExists(vtkFieldData* data, vtkInformation* field);
  int ArrayIsValid(vtkDataArray* array, vtkInformation* field);

  // Input port information checkers.
  int InputIsOptional(int port);
  int InputIsRepeatable(int port);

  vtkDataObject* NewDataObject(const char* type);

  // Support garbage collection.
  virtual void ReportReferences(vtkGarbageCollector*);
  virtual void RemoveReferences();

  // Decide whether the output data need to be generated.
  virtual int NeedToExecuteData(int outputPort);

  // Largest MTime of any algorithm on this executive or preceding
  // executives.
  unsigned long PipelineMTime;

  // Time when information or data were last generated.
  vtkTimeStamp InformationTime;
  vtkTimeStamp DataTime;

  int InProcessDownstreamRequest;
  int InProcessUpstreamRequest;
private:
  vtkDemandDrivenPipelineInternals* DemandDrivenInternal;
private:
  vtkDemandDrivenPipeline(const vtkDemandDrivenPipeline&);  // Not implemented.
  void operator=(const vtkDemandDrivenPipeline&);  // Not implemented.
};

#endif
