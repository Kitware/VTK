/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDemandDrivenPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDemandDrivenPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationKeyVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkDemandDrivenPipeline, "1.1.2.2");
vtkStandardNewMacro(vtkDemandDrivenPipeline);

//----------------------------------------------------------------------------
class vtkDemandDrivenPipelineInternals
{
public:
  vtkSmartPointer<vtkInformationVector> OutputInformation;
  vtkSmartPointer<vtkInformationVector> InputInformation;
  vtkSmartPointer<vtkInformation> RequestInformation;

  vtkDemandDrivenPipelineInternals()
    {
    this->OutputInformation = vtkSmartPointer<vtkInformationVector>::New();
    this->InputInformation = vtkSmartPointer<vtkInformationVector>::New();
    }
};

//----------------------------------------------------------------------------
vtkDemandDrivenPipeline::vtkDemandDrivenPipeline()
{
  this->DemandDrivenInternal = new vtkDemandDrivenPipelineInternals;
  this->InProcessDownstreamRequest = 0;
  this->InProcessUpstreamRequest = 0;
}

//----------------------------------------------------------------------------
vtkDemandDrivenPipeline::~vtkDemandDrivenPipeline()
{
  delete this->DemandDrivenInternal;
}

//----------------------------------------------------------------------------
void vtkDemandDrivenPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PipelineMTime: " << this->PipelineMTime << "\n";
}

//----------------------------------------------------------------------------
vtkInformationKeyVectorKey* vtkDemandDrivenPipeline::DOWNSTREAM_KEYS_TO_COPY()
{
  static vtkInformationKeyVectorKey instance("DOWNSTREAM_KEYS_TO_COPY",
                                             "vtkDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerKey* vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()
{
  static vtkInformationIntegerKey instance("REQUEST_DATA_OBJECT",
                                           "vtkDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerKey* vtkDemandDrivenPipeline::REQUEST_INFORMATION()
{
  static vtkInformationIntegerKey instance("REQUEST_INFORMATION",
                                           "vtkDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerKey* vtkDemandDrivenPipeline::REQUEST_DATA()
{
  static vtkInformationIntegerKey instance("REQUEST_DATA",
                                           "vtkDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerKey* vtkDemandDrivenPipeline::FROM_OUTPUT_PORT()
{
  static vtkInformationIntegerKey instance("FROM_OUTPUT_PORT",
                                           "vtkDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkDemandDrivenPipeline*
vtkDemandDrivenPipeline::GetConnectedInputExecutive(int port, int index)
{
  if(vtkAlgorithmOutput* input =
     this->Algorithm->GetInputConnection(port, index))
    {
    if(vtkDemandDrivenPipeline* executive =
       vtkDemandDrivenPipeline::SafeDownCast(
         input->GetProducer()->GetExecutive()))
      {
      return executive;
      }
    else
      {
      vtkErrorMacro("Algorithm "
                    << input->GetProducer()->GetClassName()
                    << "(" << input->GetProducer()
                    << ") producing input for connection index " << index
                    << " on port index " << port
                    << " to algorithm "
                    << this->Algorithm->GetClassName() << "("
                    << this->Algorithm << ") is not managed by a"
                    << " vtkDemandDrivenPipeline.");
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkInformation*
vtkDemandDrivenPipeline::GetConnectedInputInformation(int port, int index)
{
  if(vtkDemandDrivenPipeline* executive =
     this->GetConnectedInputExecutive(port, index))
    {
    vtkAlgorithmOutput* input =
      this->Algorithm->GetInputConnection(port, index);
    return executive->GetOutputInformation(input->GetIndex());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkInformation* vtkDemandDrivenPipeline::GetRequestInformation()
{
  if(!this->DemandDrivenInternal->RequestInformation)
    {
    this->DemandDrivenInternal->RequestInformation =
      vtkSmartPointer<vtkInformation>::New();
    }
  return this->DemandDrivenInternal->RequestInformation.GetPointer();
}

//----------------------------------------------------------------------------
vtkInformationVector* vtkDemandDrivenPipeline::GetInputInformation()
{
  if(!this->DemandDrivenInternal->InputInformation)
    {
    this->DemandDrivenInternal->InputInformation =
      vtkSmartPointer<vtkInformationVector>::New();
    }
  this->DemandDrivenInternal->InputInformation
    ->SetNumberOfInformationObjects(this->Algorithm->GetNumberOfInputPorts());
  return this->DemandDrivenInternal->InputInformation.GetPointer();
}

//----------------------------------------------------------------------------
vtkInformation* vtkDemandDrivenPipeline::GetInputInformation(int port)
{
  return this->GetInputInformation()->GetInformationObject(port);
}

void vtkDemandDrivenPipeline::FillDownstreamKeysToCopy(vtkInformation *info)
{
  info->Append(vtkDemandDrivenPipeline::DOWNSTREAM_KEYS_TO_COPY(),
               vtkDataObject::SCALAR_TYPE());
  info->Append(vtkDemandDrivenPipeline::DOWNSTREAM_KEYS_TO_COPY(),
               vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS());
}


//----------------------------------------------------------------------------
vtkInformationVector* vtkDemandDrivenPipeline::GetOutputInformation()
{
  if(!this->DemandDrivenInternal->OutputInformation)
    {
    this->DemandDrivenInternal->OutputInformation =
      vtkSmartPointer<vtkInformationVector>::New();
    }
  int numberOfInfoObjs = 
    this->DemandDrivenInternal->OutputInformation
    ->GetNumberOfInformationObjects();
  this->DemandDrivenInternal->OutputInformation
    ->SetNumberOfInformationObjects(this->Algorithm->GetNumberOfOutputPorts());

  // then set the keys to always copy the default values for any new
  // informaiton objects
  for (int i = numberOfInfoObjs; 
       i < this->Algorithm->GetNumberOfOutputPorts();++i)
    {
    this->FillDownstreamKeysToCopy(
      this->DemandDrivenInternal->OutputInformation->GetInformationObject(i));
    }
  
  return this->DemandDrivenInternal->OutputInformation.GetPointer();
}

//----------------------------------------------------------------------------
vtkInformation* vtkDemandDrivenPipeline::GetOutputInformation(int port)
{
  return this->GetOutputInformation()->GetInformationObject(port);
}

//----------------------------------------------------------------------------
vtkInformation*
vtkDemandDrivenPipeline::GetOutputInformation(vtkAlgorithm* algorithm,
                                              int port)
{
  return this->Superclass::GetOutputInformation(algorithm, port);
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::Update()
{
  return this->Superclass::Update();
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::Update(int port)
{
  if(!this->UpdateDataObject() || !this->UpdateInformation())
    {
    return 0;
    }
  if(port >= 0 && port < this->Algorithm->GetNumberOfOutputPorts())
    {
    return this->UpdateData(port);
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::Update(vtkAlgorithm* algorithm)
{
  return this->Superclass::Update(algorithm);
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::Update(vtkAlgorithm* algorithm, int port)
{
  return this->Superclass::Update(algorithm, port);
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::UpdateDataObject()
{
  // Avoid infinite recursion.
  if(this->InProcessDownstreamRequest)
    {
    vtkErrorMacro("UpdateDataObject invoked during a downstream request.  "
                  "Returning failure to algorithm "
                  << this->Algorithm->GetClassName() << "("
                  << this->Algorithm << ").");

    // Tests should fail when this happens because there is a bug in
    // the code.
    if(getenv("DASHBOARD_TEST_FROM_CTEST") || getenv("DART_TEST_FROM_DART"))
      {
      abort();
      }
    return 0;
    }

  // The pipeline's MTime starts with this algorithm's MTime.
  this->PipelineMTime = this->Algorithm->GetMTime();

  // Get the pipeline MTime for all the inputs.
  for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
    {
    for(int j=0; j < this->Algorithm->GetNumberOfInputConnections(i); ++j)
      {
      if(vtkDemandDrivenPipeline* e = this->GetConnectedInputExecutive(i, j))
        {
        // Propagate the UpdateDataObject call
        if(!e->UpdateDataObject())
          {
          return 0;
          }

        // We want the maximum PipelineMTime of all inputs.
        if(e->PipelineMTime > this->PipelineMTime)
          {
          this->PipelineMTime = e->PipelineMTime;
          }
        }
      }
    }

  // Make sure our output data type is up-to-date.
  int result = 1;
  if(this->PipelineMTime > this->DataObjectTime.GetMTime())
    {
    // Make sure input types are valid before algorithm does anything.
    if(!this->InputCountIsValid() || !this->InputTypeIsValid())
      {
      return 0;
      }

    // Request data type from the algorithm.
    result = this->ExecuteDataObject();

    // Data type is now up to date.
    this->DataObjectTime.Modified();
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::UpdateInformation()
{
  // Avoid infinite recursion.
  if(this->InProcessDownstreamRequest)
    {
    vtkErrorMacro("UpdateInformation invoked during a downstream request.  "
                  "Returning failure to algorithm "
                  << this->Algorithm->GetClassName() << "("
                  << this->Algorithm << ").");

    // Tests should fail when this happens because there is a bug in
    // the code.
    if(getenv("DASHBOARD_TEST_FROM_CTEST") || getenv("DART_TEST_FROM_DART"))
      {
      abort();
      }
    return 0;
    }

  // The pipeline's MTime starts with this algorithm's MTime.
  this->PipelineMTime = this->Algorithm->GetMTime();

  // Get the pipeline MTime for all the inputs.
  for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
    {
    for(int j=0; j < this->Algorithm->GetNumberOfInputConnections(i); ++j)
      {
      if(vtkDemandDrivenPipeline* e = this->GetConnectedInputExecutive(i, j))
        {
        // Propagate the UpdateInformation call
        if(!e->UpdateInformation())
          {
          return 0;
          }

        // We want the maximum PipelineMTime of all inputs.
        if(e->PipelineMTime > this->PipelineMTime)
          {
          this->PipelineMTime = e->PipelineMTime;
          }
        }
      }
    }

  // Make sure our output information is up-to-date.
  int result = 1;
  if(this->PipelineMTime > this->InformationTime.GetMTime())
    {
    // Make sure input types are valid before algorithm does anything.
    if(!this->InputCountIsValid() || !this->InputTypeIsValid())
      {
      return 0;
      }

    // Request information from the algorithm.
    result = this->ExecuteInformation();

    // Information is now up to date.
    this->InformationTime.Modified();
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::UpdateData(int outputPort)
{
  // Avoid infinite recursion.
  if(this->InProcessDownstreamRequest)
    {
    vtkErrorMacro("UpdateData invoked during a downstream request.  "
                  "Returning failure to algorithm "
                  << this->Algorithm->GetClassName() << "("
                  << this->Algorithm << ").");

    // Tests should fail when this happens because there is a bug in
    // the code.
    if(getenv("DASHBOARD_TEST_FROM_CTEST") || getenv("DART_TEST_FROM_DART"))
      {
      abort();
      }
    return 0;
    }

  // Range check.
  if(outputPort < -1 ||
     outputPort >= this->Algorithm->GetNumberOfOutputPorts())
    {
    vtkErrorMacro("UpdateData given output port index "
                  << outputPort << " on an algorithm with "
                  << this->Algorithm->GetNumberOfOutputPorts()
                  << " output ports.");
    return 0;
    }

  // Make sure our outputs are up-to-date.
  int result = 1;
  if(this->NeedToExecuteData(outputPort))
    {
    // Make sure everything on which we might rely is up-to-date.
    for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
      {
      for(int j=0; j < this->Algorithm->GetNumberOfInputConnections(i); ++j)
        {
        if(vtkDemandDrivenPipeline* e = this->GetConnectedInputExecutive(i, j))
          {
          if(!e->UpdateData(this->Algorithm->GetInputConnection(i, j)->GetIndex()))
            {
            return 0;
            }
          }
        }
      }
    
    // Make sure inputs are valid before algorithm does anything.
    if(!this->InputCountIsValid() || !this->InputTypeIsValid() ||
       !this->InputFieldsAreValid())
      {
      return 0;
      }

#if 0
    // TODO: This condition gives the default behavior if the user
    // asks for a piece that cannot be generated by the source.  Just
    // ignore the request and return empty.
    if (output && output->GetMaximumNumberOfPieces() > 0 &&
        output->GetUpdatePiece() >= output->GetMaximumNumberOfPieces())
      {
      skipExecute = 1;
      }
#endif

    // Request data from the algorithm.
    result = this->ExecuteData(outputPort);

    // Data are now up to date.
    this->DataTime.Modified();
    }
  return result;
}

//----------------------------------------------------------------------------
void vtkDemandDrivenPipeline::CopyDefaultInformation()
{ 
  // Setup default information for the outputs.
  if(this->Algorithm->GetNumberOfInputPorts() > 0)
    {
    // Copy information from the first input.
    vtkInformation* inInfo = 
      this->GetInputInformation(0)
      ->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION())
      ->GetInformationObject(0);
    if (inInfo)
      {
      for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
        {
        vtkInformation* outInfo = this->GetOutputInformation(i);
        outInfo->CopyEntries(
          inInfo, vtkDemandDrivenPipeline::DOWNSTREAM_KEYS_TO_COPY());
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::ExecuteDataObject()
{
  this->PrepareDownstreamRequest(REQUEST_DATA_OBJECT());

  this->InProcessDownstreamRequest = 1;
  int result = this->Algorithm->ProcessDownstreamRequest(
    this->GetRequestInformation(), this->GetInputInformation(),
    this->GetOutputInformation());
  this->InProcessDownstreamRequest = 0;

  // Make sure a valid data object exists for all output ports.
  for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
    if(!this->CheckDataObject(i))
      {
      return 0;
      }
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::ExecuteInformation()
{
  this->PrepareDownstreamRequest(REQUEST_INFORMATION());

  // Setup default information for the outputs.
  this->CopyDefaultInformation();

  this->InProcessDownstreamRequest = 1;
  int result = this->Algorithm->ProcessDownstreamRequest(
    this->GetRequestInformation(), this->GetInputInformation(),
    this->GetOutputInformation());

  this->InProcessDownstreamRequest = 0;
  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::ExecuteData(int outputPort)
{
  this->PrepareDownstreamRequest(REQUEST_DATA());
  this->GetRequestInformation()->Set(FROM_OUTPUT_PORT(), outputPort);
  this->InProcessDownstreamRequest = 1;
  int result = this->Algorithm->ProcessDownstreamRequest(
    this->GetRequestInformation(), this->GetInputInformation(),
    this->GetOutputInformation());
  this->InProcessDownstreamRequest = 0;
  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::CheckDataObject(int port)
{
  // Check that the given output port has a valid data object.
  vtkInformation* outInfo =
    this->GetOutputInformation()->GetInformationObject(port);
  vtkDataObject* data = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation* portInfo = this->Algorithm->GetOutputPortInformation(port);
  if(const char* dt = portInfo->Get(vtkDataObject::DATA_TYPE_NAME()))
    {
    // The output port specifies a data type.  Make sure the data
    // object exists and is of the right type.
    if(!data || !data->IsA(dt))
      {
      // Try to create an instance of the correct type.
      data = this->NewDataObject(dt);
      this->SetOutputDataInternal(this->Algorithm, port, data);
      if(data)
        {
        data->SetProducerPort(this->Algorithm->GetOutputPort(port));
        data->Delete();
        }
      }
    if(!data)
      {
      // The algorithm has a bug and did not create the data object.
      vtkErrorMacro("Algorithm " << this->Algorithm->GetClassName() << "("
                    << this->Algorithm
                    << ") did not create output for port " << port
                    << " when asked by REQUEST_DATA_OBJECT and does not"
                    << " specify a concrete DATA_TYPE_NAME.");
      return 0;
      }
    return 1;
    }
  else if(data)
    {
    // The algorithm did not specify its output data type.  Just assume
    // the data object is of the correct type.
    return 1;
    }
  else
    {
    // The algorithm did not specify its output data type and no
    // object exists.
    vtkErrorMacro("Algorithm " << this->Algorithm->GetClassName() << "("
                  << this->Algorithm
                  << ") did not create output for port " << port
                  << " when asked by REQUEST_DATA_OBJECT and does not"
                  << " specify any DATA_TYPE_NAME.");
    return 0;
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDemandDrivenPipeline::GetOutputData(int port)
{
  if(!this->OutputPortIndexInRange(port, "get data for"))
    {
    return 0;
    }

  // Bring the data object up to date.
  this->UpdateDataObject();

  // Return the data object.
  return this->GetOutputDataInternal(this->Algorithm, port);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDemandDrivenPipeline::GetOutputData(vtkAlgorithm* algorithm,
                                                      int port)
{
  return this->Superclass::GetOutputData(algorithm, port);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDemandDrivenPipeline::GetInputData(int port, int index)
{
  if(vtkDemandDrivenPipeline* e = this->GetConnectedInputExecutive(port, index))
    {
    vtkAlgorithmOutput* input =
      this->Algorithm->GetInputConnection(port, index);
    return e->GetOutputData(input->GetIndex());
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void
vtkDemandDrivenPipeline
::PrepareDownstreamRequest(vtkInformationIntegerKey* rkey)
{
  // Setup request information for this request.
  vtkInformation* request = this->GetRequestInformation();
  request->Clear();
  request->Set(rkey, 1);

  // Put all the input data objects into the input information.
  vtkInformationVector* inputVector = this->GetInputInformation();
  for(int ip=0; ip < this->Algorithm->GetNumberOfInputPorts(); ++ip)
    {
    vtkInformation* info = inputVector->GetInformationObject(ip);
    int numConnections = this->Algorithm->GetNumberOfInputConnections(ip);
    vtkInformationVector* connInfo =
      info->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION());
    if(!connInfo)
      {
      connInfo = vtkInformationVector::New();
      info->Set(vtkAlgorithm::INPUT_CONNECTION_INFORMATION(), connInfo);
      connInfo->Delete();
      }
    connInfo->SetNumberOfInformationObjects(numConnections);
    if(numConnections > 0)
      {
      for(int i=0; i < numConnections; ++i)
        {
        connInfo->SetInformationObject(
          i, this->GetConnectedInputInformation(ip, i));
        }
      }
    }
}

//----------------------------------------------------------------------------
void
vtkDemandDrivenPipeline
::PrepareUpstreamRequest(vtkInformationIntegerKey* rkey)
{
  this->PrepareDownstreamRequest(rkey);
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputCountIsValid()
{
  // Check the number of connections for each port.
  int result = 1;
  for(int p=0; p < this->Algorithm->GetNumberOfInputPorts(); ++p)
    {
    if(!this->InputCountIsValid(p))
      {
      result = 0;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputCountIsValid(int port)
{
  // Get the number of connections for this port.
  int connections = this->Algorithm->GetNumberOfInputConnections(port);

  // If the input port is optional, there may be less than one connection.
  if(!this->InputIsOptional(port) && (connections < 1))
    {
    vtkErrorMacro("Input port " << port << " of algorithm "
                  << this->Algorithm->GetClassName()
                  << "(" << this->Algorithm << ") has " << connections
                  << " connections but is not optional.");
    return 0;
    }

  // If the input port is repeatable, there may be more than one connection.
  if(!this->InputIsRepeatable(port) && (connections > 1))
    {
    vtkErrorMacro("Input port " << port << " of algorithm "
                  << this->Algorithm->GetClassName()
                  << "(" << this->Algorithm << ") has " << connections
                  << " connections but is not repeatable.");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputTypeIsValid()
{
  // Check the connection types for each port.
  int result = 1;
  for(int p=0; p < this->Algorithm->GetNumberOfInputPorts(); ++p)
    {
    if(!this->InputTypeIsValid(p))
      {
      result = 0;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputTypeIsValid(int port)
{
  // Check the type of each connection on this port.
  int result = 1;
  for(int i=0; i < this->Algorithm->GetNumberOfInputConnections(port); ++i)
    {
    if(!this->InputTypeIsValid(port, i))
      {
      result = 0;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputTypeIsValid(int port, int index)
{
  vtkInformation* info = this->Algorithm->GetInputPortInformation(port);
  vtkDataObject* input = this->GetInputData(port, index);

  // Special case for compatibility layer to support NULL inputs.
  if(this->Algorithm->IsA("vtkProcessObject") &&
     input->IsA("vtkProcessObjectDummyData"))
    {
    return 1;
    }

  // Enforce required type, if any.
  if(const char* dt = info->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))
    {
    // The input cannot be NULL.
    if(!input)
      {
      vtkErrorMacro("Input for connection index " << index
                    << " on input port index " << port
                    << " for algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ") is NULL, but a " << dt
                    << " is required.");
      return 0;
      }

    // The input must be of required type.
    if(!input->IsA(dt))
      {
      vtkErrorMacro("Input for connection index " << index
                    << " on input port index " << port
                    << " for algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ") is of type "
                    << input->GetClassName() << ", but a " << dt
                    << " is required.");
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputFieldsAreValid()
{
  // Check the fields for each port.
  int result = 1;
  for(int p=0; p < this->Algorithm->GetNumberOfInputPorts(); ++p)
    {
    if(!this->InputFieldsAreValid(p))
      {
      result = 0;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputFieldsAreValid(int port)
{
  // Check the fields for each connection on this port.
  int result = 1;
  for(int i=0; i < this->Algorithm->GetNumberOfInputConnections(port); ++i)
    {
    if(!this->InputFieldsAreValid(port, i))
      {
      result = 0;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputFieldsAreValid(int port, int index)
{
  vtkInformation* info = this->Algorithm->GetInputPortInformation(port);
  vtkInformationVector* fields =
    info->Get(vtkAlgorithm::INPUT_REQUIRED_FIELDS());

  // If there are no required fields, there is nothing to check.
  if(!fields)
    {
    return 1;
    }
  vtkDataObject* input = this->GetInputData(port, index);

  // Special case for compatibility layer to support NULL inputs.
  if(this->Algorithm->IsA("vtkProcessObject") &&
     input->IsA("vtkProcessObjectDummyData"))
    {
    return 1;
    }

  // Check availability of each required field.
  int result = 1;
  for(int i=0; i < fields->GetNumberOfInformationObjects(); ++i)
    {
    vtkInformation* field = fields->GetInformationObject(i);

    // Decide which kinds of fields to check.
    int checkPoints = 1;
    int checkCells = 1;
    int checkFields = 1;
    if(field->Has(vtkDataObject::FIELD_ASSOCIATION()))
      {
      switch(field->Get(vtkDataObject::FIELD_ASSOCIATION()))
        {
        case vtkDataObject::FIELD_ASSOCIATION_POINTS:
          checkCells = 0; checkFields = 0; break;
        case vtkDataObject::FIELD_ASSOCIATION_CELLS:
          checkPoints = 0; checkFields = 0; break;
        case vtkDataObject::FIELD_ASSOCIATION_NONE:
          checkPoints = 0; checkCells = 0; break;
        }
      }

    // Point and cell data arrays only exist in vtkDataSet instances.
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(input);

    // Look for a point data, cell data, or field data array matching
    // the requirements.
    if(!(checkPoints && dataSet && dataSet->GetPointData() &&
         this->DataSetAttributeExists(dataSet->GetPointData(), field)) &&
       !(checkCells && dataSet && dataSet->GetCellData() &&
         this->DataSetAttributeExists(dataSet->GetCellData(), field)) &&
       !(checkFields && input && input->GetFieldData() &&
         this->FieldArrayExists(input->GetFieldData(), field)))
      {
      /* TODO: Construct more descriptive error message from field
         requirements. */
      vtkErrorMacro("Required field not found in input.");
      result = 0;
      }
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::DataSetAttributeExists(vtkDataSetAttributes* dsa,
                                                    vtkInformation* field)
{
  if(field->Has(vtkDataObject::FIELD_ATTRIBUTE_TYPE()))
    {
    // A specific attribute must match the requirements.
    int attrType = field->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
    return this->ArrayIsValid(dsa->GetAttribute(attrType), field);
    }
  else
    {
    // Search for an array matching the requirements.
    return this->FieldArrayExists(dsa, field);
    }
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::FieldArrayExists(vtkFieldData* data,
                                              vtkInformation* field)
{
  // Search the field data instance for an array matching the requirements.
  for(int a=0; a < data->GetNumberOfArrays(); ++a)
    {
    if(this->ArrayIsValid(data->GetArray(a), field))
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::ArrayIsValid(vtkDataArray* array,
                                          vtkInformation* field)
{
  // Enforce existence of the array.
  if(!array)
    {
    return 0;
    }

  // Enforce name of the array.  This should really only be used for
  // field data (not point or cell data).
  if(const char* name = field->Get(vtkDataObject::FIELD_NAME()))
    {
    if(!array->GetName() || (strcmp(name, array->GetName()) != 0))
      {
      return 0;
      }
    }

  // Enforce component type for the array.
  if(field->Has(vtkDataObject::FIELD_ARRAY_TYPE()))
    {
    int arrayType = field->Get(vtkDataObject::FIELD_ARRAY_TYPE());
    if(array->GetDataType() != arrayType)
      {
      return 0;
      }
    }

  // Enforce number of components for the array.
  if(field->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
    {
    int arrayNumComponents =
      field->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
    if(array->GetNumberOfComponents() != arrayNumComponents)
      {
      return 0;
      }
    }

  // Enforce number of tuples.  This should really only be used for
  // field data (not point or cell data).
  if(field->Has(vtkDataObject::FIELD_NUMBER_OF_TUPLES()))
    {
    int arrayNumTuples = field->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES());
    if(array->GetNumberOfTuples() != arrayNumTuples)
      {
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputIsOptional(int port)
{
  if(vtkInformation* info = this->Algorithm->GetInputPortInformation(port))
    {
    return info->Get(vtkAlgorithm::INPUT_IS_OPTIONAL());
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::InputIsRepeatable(int port)
{
  if(vtkInformation* info = this->Algorithm->GetInputPortInformation(port))
    {
    return info->Get(vtkAlgorithm::INPUT_IS_REPEATABLE());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDemandDrivenPipeline::NewDataObject(const char* type)
{
  // Check for some standard types and then try the instantiator.
  if(strcmp(type, "vtkImageData") == 0)
    {
    return vtkImageData::New();
    }
  else if(strcmp(type, "vtkPolyData") == 0)
    {
    return vtkPolyData::New();
    }
  else if(strcmp(type, "vtkRectilinearGrid") == 0)
    {
    return vtkRectilinearGrid::New();
    }
  else if(strcmp(type, "vtkStructuredGrid") == 0)
    {
    return vtkStructuredGrid::New();
    }
  else if(strcmp(type, "vtkUnstructuredGrid") == 0)
    {
    return vtkUnstructuredGrid::New();
    }
  else if(vtkObject* obj = vtkInstantiator::CreateInstance(type))
    {
    vtkDataObject* data = vtkDataObject::SafeDownCast(obj);
    if(!data)
      {
      obj->Delete();
      }
    return data;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkDemandDrivenPipeline::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  
  // if we have an algorithm then report our references to its data objects 
  if (this->Algorithm)
    {
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
      {
      collector->ReportReference(
      this->GetOutputInformation(i)->Get(vtkDataObject::DATA_OBJECT()),
      "AlgorithmOutput");
      }
    }
}

//----------------------------------------------------------------------------
void vtkDemandDrivenPipeline::RemoveReferences()
{
  for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
    this->GetOutputInformation(i)->Remove(vtkDataObject::DATA_OBJECT());
    }
  this->Superclass::RemoveReferences();
}

//----------------------------------------------------------------------------
int vtkDemandDrivenPipeline::NeedToExecuteData(int outputPort)
{
  // If the data are out of date, we need to execute.
  if(this->PipelineMTime > this->DataTime.GetMTime())
    {
    return 1;
    }

  // If no port is specified, check all ports.  Subclass
  // implementations might use the port number.
  if(outputPort < 0)
    {
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
      {
      if(this->NeedToExecuteData(i))
        {
        return 1;
        }
      }
    }

  // We do not need to execute.
  return 0;
}
