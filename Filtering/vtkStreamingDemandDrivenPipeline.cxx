/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingDemandDrivenPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

vtkCxxRevisionMacro(vtkStreamingDemandDrivenPipeline, "1.1");
vtkStandardNewMacro(vtkStreamingDemandDrivenPipeline);

//----------------------------------------------------------------------------
class vtkStreamingDemandDrivenPipelineInternals
{
public:
};

//----------------------------------------------------------------------------
vtkStreamingDemandDrivenPipeline::vtkStreamingDemandDrivenPipeline()
{
  this->StreamingDemandDrivenInternal = new vtkStreamingDemandDrivenPipelineInternals;
}

//----------------------------------------------------------------------------
vtkStreamingDemandDrivenPipeline::~vtkStreamingDemandDrivenPipeline()
{
  delete this->StreamingDemandDrivenInternal;
}

//----------------------------------------------------------------------------
void vtkStreamingDemandDrivenPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update()
{
  return this->Superclass::Update();
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update(int port)
{
  if(!this->UpdateInformation())
    {
    return 0;
    }
  if(port >= 0 && port < this->Algorithm->GetNumberOfOutputPorts())
    {
    int retval = 1;
    // some streaming filters can request that the pipeline execute multiple
    // times for a single update
    do 
      {
      retval =  
        this->PropagateUpdateExtent(port) && this->UpdateData(port) && retval;
      }
    while (this->Algorithm->GetInformation()->Get(CONTINUE_EXECUTING()));
    return retval;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update(vtkAlgorithm* algorithm)
{
  return this->Superclass::Update(algorithm);
}

int vtkStreamingDemandDrivenPipeline::UpdateWholeExtent(
  vtkAlgorithm* algorithm)
{
  return this->Update(algorithm);

#if 0
  if(algorithm != this->GetAlgorithm())
    {
    vtkErrorMacro("Request to update algorithm not managed by this "
                  "executive: " << algorithm);
    return 0;
    }

  // update the info
  if(!this->UpdateInformation())
    {
    return 0;
    }
  
  // set the output UpdateExtent to the WholeExtent
#endif
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update(vtkAlgorithm* algorithm, int port)
{
  return this->Superclass::Update(algorithm, port);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::ExecuteInformation()
{
  // Let the superclass make the request to the algorithm.
  if(this->Superclass::ExecuteInformation())
    {
    // For each port, if the update extent has not been set
    // explicitly, keep it set to the whole extent.
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = this->GetOutputInformation(i);
      if(info->Has(WHOLE_EXTENT()) &&
         (!info->Has(UPDATE_EXTENT_INITIALIZED()) ||
          !info->Get(UPDATE_EXTENT_INITIALIZED())))
        {
        int extent[6];
        info->Get(WHOLE_EXTENT(), extent);
        info->Set(UPDATE_EXTENT(), extent, 6);
        }
      }
    return 1;
    }
  else
    {
    return 0;
    }
}

void vtkStreamingDemandDrivenPipeline::
FillDownstreamKeysToCopy(vtkInformation *info)
{
  this->Superclass::FillDownstreamKeysToCopy(info);
  info->Append(vtkDemandDrivenPipeline::DOWNSTREAM_KEYS_TO_COPY(),
               WHOLE_EXTENT());
  info->Append(vtkDemandDrivenPipeline::DOWNSTREAM_KEYS_TO_COPY(),
               MAXIMUM_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
void vtkStreamingDemandDrivenPipeline::CopyDefaultInformation()
{ 
  this->Superclass::CopyDefaultInformation();
  
  // Setup default information.
  for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* outPort = this->Algorithm->GetOutputPortInformation(i);
    if(outPort->Has(vtkDataObject::DATA_EXTENT_TYPE()))
      {
      vtkInformation* outInfo = this->GetOutputInformation(i);
      if(outPort->Get(vtkDataObject::DATA_EXTENT_TYPE()) ==
         VTK_PIECES_EXTENT)
        {
        if (!outInfo->Has(MAXIMUM_NUMBER_OF_PIECES()))
          {
          // Since most unstructured filters in VTK generate all their
          // data once, set the default maximum number of pieces to 1.
          outInfo->Set(MAXIMUM_NUMBER_OF_PIECES(), 1);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::PropagateUpdateExtent(int outputPort)
{
  // Avoid infinite recursion.
  if(this->InProcessUpstreamRequest)
    {
    vtkErrorMacro("PropagateUpdateExtent invoked during an upstream request.  "
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
    vtkErrorMacro("PropagateUpdateExtent given output port index "
                  << outputPort << " on an algorithm with "
                  << this->Algorithm->GetNumberOfOutputPorts()
                  << " output ports.");
    return 0;
    }

  // Make sure the information on the output port is valid.
  if(!this->VerifyOutputInformation(outputPort))
    {
    return 0;
    }
  
  // If we need to update data, propagate the update extent.
  int result = 1;
  if(this->NeedToExecuteData(outputPort))
    {
    // Make sure input types are valid before algorithm does anything.
    if(!this->InputCountIsValid() || !this->InputTypeIsValid())
      {
      return 0;
      }

    // Request information from the algorithm.
    this->PrepareUpstreamRequest(REQUEST_UPDATE_EXTENT());
    this->GetRequestInformation()->Set(FROM_OUTPUT_PORT(), outputPort);
    this->InProcessUpstreamRequest = 1;
    result = this->Algorithm->ProcessUpstreamRequest(
      this->GetRequestInformation(), this->GetInputInformation(),
      this->GetOutputInformation());
    this->InProcessUpstreamRequest = 0;

    if(!result)
      {
      return 0;
      }

    // Propagate the update extent to all inputs.
    for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
      {
      for(int j=0; j < this->Algorithm->GetNumberOfInputConnections(i); ++j)
        {
        vtkDemandDrivenPipeline* ddp = this->GetConnectedInputExecutive(i, j);
        if(vtkStreamingDemandDrivenPipeline* sddp =
           vtkStreamingDemandDrivenPipeline::SafeDownCast(ddp))
          {
          if(!sddp->PropagateUpdateExtent(this->Algorithm->GetInputConnection(i, j)->GetIndex()))
            {
            return 0;
            }
          }
        }
      }
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::VerifyOutputInformation(int outputPort)
{
  // If no port is specified, check all ports.
  if(outputPort < 0)
    {
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
      {
      if(!this->VerifyOutputInformation(i))
        {
        return 0;
        }
      }
    }

  // Get the information object to check.
  vtkInformation* outInfo = this->GetOutputInformation(outputPort);

  // Make sure there is a data object.  It is supposed to be created
  // by the ExecuteInformation step.
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if(!dataObject)
    {
    vtkErrorMacro("No data object has been set in the information for "
                  "output port " << outputPort << ".");
    return 0;
    }

  // if it is dummy then return ok
  if(dataObject->IsA("vtkProcessObjectDummyData"))
    {
    return 1;
    }
  
  // Check extents.
  vtkInformation* dataInfo = dataObject->GetInformation();
  if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_PIECES_EXTENT)
    {
    // For an unstructured extent, make sure the update request
    // exists.  We do not need to check if it is valid because
    // out-of-range requests produce empty data.
    if(!outInfo->Has(MAXIMUM_NUMBER_OF_PIECES()))
      {
      vtkErrorMacro("No maximum number of pieces has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    if(!outInfo->Has(UPDATE_PIECE_NUMBER()))
      {
      vtkErrorMacro("No update piece number has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    if(!outInfo->Has(UPDATE_NUMBER_OF_PIECES()))
      {
      vtkErrorMacro("No update number of pieces has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    if(!outInfo->Has(UPDATE_NUMBER_OF_GHOST_LEVELS()))
      {
      vtkErrorMacro("No update number of ghost levels has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    }
  else if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
    {
    // For a structured extent, make sure the update request
    // exists.
    if(!outInfo->Has(WHOLE_EXTENT()))
      {
      vtkErrorMacro("No whole extent has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    if(!outInfo->Has(UPDATE_EXTENT()))
      {
      vtkErrorMacro("No update extent has been set in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ").");
      return 0;
      }
    // Make sure the update request is inside the whole extent.
    int wholeExtent[6];
    int updateExtent[6];
    outInfo->Get(WHOLE_EXTENT(), wholeExtent);
    outInfo->Get(UPDATE_EXTENT(), updateExtent);
    if((updateExtent[0] < wholeExtent[0] ||
        updateExtent[1] > wholeExtent[1] ||
        updateExtent[2] < wholeExtent[2] ||
        updateExtent[3] > wholeExtent[3] ||
        updateExtent[4] < wholeExtent[4] ||
        updateExtent[5] > wholeExtent[5]) &&
       (updateExtent[0] <= updateExtent[1] &&
        updateExtent[2] <= updateExtent[3] &&
        updateExtent[4] <= updateExtent[5]))
      {
      // Update extent is outside the whole extent and is not empty.
      vtkErrorMacro("The update extent specified in the "
                    "information for output port " << outputPort
                    << " on algorithm " << this->Algorithm->GetClassName()
                    << "(" << this->Algorithm << ") is "
                    << updateExtent[0] << " " << updateExtent[1] << " "
                    << updateExtent[2] << " " << updateExtent[3] << " "
                    << updateExtent[4] << " " << updateExtent[5]
                    << ", which is outside the whole extent "
                    << wholeExtent[0] << " " << wholeExtent[1] << " "
                    << wholeExtent[2] << " " << wholeExtent[3] << " "
                    << wholeExtent[4] << " " << wholeExtent[5] << ".");
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::NeedToExecuteData(int outputPort)
{
  // If no port is specified, check all ports.  This behavior is
  // implemented by the superclass.
  if(outputPort < 0)
    {
    return this->Superclass::NeedToExecuteData(outputPort);
    }

  // Does the superclass want to execute?
  if(this->Superclass::NeedToExecuteData(outputPort))
    {
    return 1;
    }

  // Has the algorithm asked to be executed again?
  if(this->Algorithm->GetInformation()->Get(CONTINUE_EXECUTING()))
    {
    return 1;
    }

  // We need to check the requested update extent.  Get the output
  // port information and data information.  We do not need to check
  // existence of values because it has already been verified by
  // VerifyOutputInformation.
  vtkInformation* outInfo = this->GetOutputInformation(outputPort);
  vtkDataObject* dataObject = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation* dataInfo = dataObject->GetInformation();
  if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_PIECES_EXTENT)
    {
    // Check the unstructured extent.  If we do not have the requested
    // piece, we need to execute.
    int dataPiece = dataInfo->Get(vtkDataObject::DATA_PIECE_NUMBER());
    int dataNumberOfPieces = dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
    int dataGhostLevel = dataInfo->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
    int updatePiece = outInfo->Get(UPDATE_PIECE_NUMBER());
    int updateNumberOfPieces = outInfo->Get(UPDATE_NUMBER_OF_PIECES());
    int updateGhostLevel = outInfo->Get(UPDATE_NUMBER_OF_GHOST_LEVELS());
    if(dataPiece != updatePiece ||
       dataNumberOfPieces != updateNumberOfPieces ||
       dataGhostLevel != updateGhostLevel)
      {
      return 1;
      }
    }
  else if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
    {
    // Check the structured extent.  If the update extent is outside
    // of the extent and not empty, we need to execute.
    int dataExtent[6];
    int updateExtent[6];
    outInfo->Get(UPDATE_EXTENT(), updateExtent);
    dataInfo->Get(vtkDataObject::DATA_EXTENT(), dataExtent);
    if((updateExtent[0] < dataExtent[0] ||
        updateExtent[1] > dataExtent[1] ||
        updateExtent[2] < dataExtent[2] ||
        updateExtent[3] > dataExtent[3] ||
        updateExtent[4] < dataExtent[4] ||
        updateExtent[5] > dataExtent[5]) &&
       (updateExtent[0] <= updateExtent[1] &&
        updateExtent[2] <= updateExtent[3] &&
        updateExtent[4] <= updateExtent[5]))
      {
      return 1;
      }
    }

  // We do not need to execute.
  return 0;
}

//----------------------------------------------------------------------------
// Define information keys for this pipeline object.
#define VTK_SDDP_DEFINE_KEY_METHOD(NAME, type)                              \
  vtkInformation##type##Key* vtkStreamingDemandDrivenPipeline::NAME()       \
    {                                                                       \
    static vtkInformation##type##Key instance(                              \
           #NAME, "vtkStreamingDemandDrivenPipeline");                      \
    return &instance;                                                       \
    }
VTK_SDDP_DEFINE_KEY_METHOD(CONTINUE_EXECUTING, Integer);
VTK_SDDP_DEFINE_KEY_METHOD(REQUEST_UPDATE_EXTENT, Integer);
VTK_SDDP_DEFINE_KEY_METHOD(WHOLE_EXTENT, IntegerVector);
VTK_SDDP_DEFINE_KEY_METHOD(MAXIMUM_NUMBER_OF_PIECES, Integer);
VTK_SDDP_DEFINE_KEY_METHOD(UPDATE_EXTENT_INITIALIZED, Integer);
VTK_SDDP_DEFINE_KEY_METHOD(UPDATE_EXTENT, IntegerVector);
VTK_SDDP_DEFINE_KEY_METHOD(UPDATE_PIECE_NUMBER, Integer);
VTK_SDDP_DEFINE_KEY_METHOD(UPDATE_NUMBER_OF_PIECES, Integer);
VTK_SDDP_DEFINE_KEY_METHOD(UPDATE_NUMBER_OF_GHOST_LEVELS, Integer);
#undef VTK_SDDP_DEFINE_KEY_METHOD
