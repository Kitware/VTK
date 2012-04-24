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
// .NAME vtkDemandDrivenPipeline - Executive supporting on-demand execution.
// .SECTION Description
// vtkDemandDrivenPipeline is an executive that will execute an
// algorithm only when its outputs are out-of-date with respect to its
// inputs.

#ifndef __vtkDemandDrivenPipeline_h
#define __vtkDemandDrivenPipeline_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkExecutive.h"

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

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkDemandDrivenPipeline : public vtkExecutive
{
public:
  static vtkDemandDrivenPipeline* New();
  vtkTypeMacro(vtkDemandDrivenPipeline,vtkExecutive);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generalized interface for asking the executive to fulfill update
  // requests.
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

  // Description:
  // Implement the pipeline modified time request.
  virtual int
  ComputePipelineMTime(vtkInformation* request,
                       vtkInformationVector** inInfoVec,
                       vtkInformationVector* outInfoVec,
                       int requestFromOutputPort,
                       unsigned long* mtime);

  // Description:
  // Bring the algorithm's outputs up-to-date.  Returns 1 for success
  // and 0 for failure.
  virtual int Update();
  virtual int Update(int port);

  // Description:
  // Get the PipelineMTime for this exective.
  vtkGetMacro(PipelineMTime, unsigned long);

  // Description:
  // Set whether the given output port releases data when it is
  // consumed.  Returns 1 if the the value changes and 0 otherwise.
  virtual int SetReleaseDataFlag(int port, int n);

  // Description:
  // Get whether the given output port releases data when it is consumed.
  virtual int GetReleaseDataFlag(int port);

  // Description:
  // Bring the PipelineMTime up to date.
  virtual int UpdatePipelineMTime();

  // Description:
  // Bring the output data object's existence up to date.  This does
  // not actually produce data, but does create the data object that
  // will store data produced during the UpdateData step.
  virtual int UpdateDataObject();

  // Description:
  // Bring the output information up to date.
  virtual int UpdateInformation();

  // Description:
  // Bring the output data up to date.  This should be called only
  // when information is up to date.  Use the Update method if it is
  // not known that the information is up to date.
  virtual int UpdateData(int outputPort);

  // Description:
  // Key defining a request to make sure the output data objects exist.
  static vtkInformationRequestKey* REQUEST_DATA_OBJECT();

  // Description:
  // Key defining a request to make sure the output information is up to date.
  static vtkInformationRequestKey* REQUEST_INFORMATION();

  // Description:
  // Key defining a request to make sure the output data are up to date.
  static vtkInformationRequestKey* REQUEST_DATA();

  // Description:
  // Key defining a request to mark outputs that will NOT be generated
  // during a REQUEST_DATA.
  static vtkInformationRequestKey* REQUEST_DATA_NOT_GENERATED();

  // Description:
  // Key to specify in pipeline information the request that data be
  // released after it is used.
  static vtkInformationIntegerKey* RELEASE_DATA();

  // Description:
  // Key to store a mark for an output that will not be generated.
  // Algorithms use this to tell the executive that they will not
  // generate certain outputs for a REQUEST_DATA.
  static vtkInformationIntegerKey* DATA_NOT_GENERATED();

  // Description:
  // Create (New) and return a data object of the given type.
  // This is here for backwards compatibility. Use
  // vtkDataObjectTypes::NewDataObject() instead.
  static vtkDataObject* NewDataObject(const char* type);

  // Description:
  // Key to be used for REQUEST_INFORMATION and REQUEST_DATA_OBJECT
  // passes when you modification time should not be taken into account.
  static vtkInformationIntegerKey* REQUEST_REGENERATE_INFORMATION();

protected:
  vtkDemandDrivenPipeline();
  ~vtkDemandDrivenPipeline();

  // Helper methods to send requests to the algorithm.
  virtual int ExecuteDataObject(vtkInformation* request,
                                vtkInformationVector** inInfo,
                                vtkInformationVector* outInfo);
  virtual int ExecuteInformation(vtkInformation* request,
                                 vtkInformationVector** inInfo,
                                 vtkInformationVector* outInfo);
  virtual int ExecuteData(vtkInformation* request,
                          vtkInformationVector** inInfo,
                          vtkInformationVector* outInfo);


  // Reset the pipeline update values in the given output information object.
  virtual void ResetPipelineInformation(int port, vtkInformation*);

  // Check whether the data object in the pipeline information for an
  // output port exists and has a valid type.
  virtual int CheckDataObject(int port, vtkInformationVector* outInfo);


  // Input connection validity checkers.
  int InputCountIsValid(vtkInformationVector **);
  int InputCountIsValid(int port,vtkInformationVector **);
  int InputTypeIsValid(vtkInformationVector **);
  int InputTypeIsValid(int port,vtkInformationVector **);
  virtual int InputTypeIsValid(int port, int index,vtkInformationVector **);
  int InputFieldsAreValid(vtkInformationVector **);
  int InputFieldsAreValid(int port,vtkInformationVector **);
  virtual int InputFieldsAreValid(int port, int index,vtkInformationVector **);

  // Field existence checkers.
  int DataSetAttributeExists(vtkDataSetAttributes* dsa, vtkInformation* field);
  int FieldArrayExists(vtkFieldData* data, vtkInformation* field);
  int ArrayIsValid(vtkAbstractArray* array, vtkInformation* field);

  // Input port information checkers.
  int InputIsOptional(int port);
  int InputIsRepeatable(int port);

  // Decide whether the output data need to be generated.
  virtual int NeedToExecuteData(int outputPort,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec);

  // Handle before/after operations for ExecuteData method.
  virtual void ExecuteDataStart(vtkInformation* request,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec);
  virtual void ExecuteDataEnd(vtkInformation* request,
                              vtkInformationVector** inInfoVec,
                              vtkInformationVector* outInfoVec);
  virtual void MarkOutputsGenerated(vtkInformation* request,
                                    vtkInformationVector** inInfoVec,
                                    vtkInformationVector* outInfoVec);

  // Largest MTime of any algorithm on this executive or preceding
  // executives.
  unsigned long PipelineMTime;

  // Time when information or data were last generated.
  vtkTimeStamp DataObjectTime;
  vtkTimeStamp InformationTime;
  vtkTimeStamp DataTime;

//BTX
  friend class vtkCompositeDataPipeline;
//ETX


  vtkInformation *InfoRequest;
  vtkInformation *DataObjectRequest;
  vtkInformation *DataRequest;

private:
  vtkDemandDrivenPipeline(const vtkDemandDrivenPipeline&);  // Not implemented.
  void operator=(const vtkDemandDrivenPipeline&);  // Not implemented.
};

#endif
