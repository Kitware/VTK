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

vtkCxxRevisionMacro(vtkStreamingDemandDrivenPipeline, "1.7");
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
vtkInformationIntegerKey*
vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()
{
  static vtkInformationIntegerKey instance("REQUEST_UPDATE_EXTENT",
                                           "vtkStreamingDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerVectorKey*
vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()
{
  static vtkInformationIntegerVectorKey instance("WHOLE_EXTENT",
                                                 "vtkStreamingDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerVectorKey*
vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()
{
  static vtkInformationIntegerVectorKey instance("UPDATE_EXTENT",
                                                 "vtkStreamingDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerKey*
vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED()
{
  static vtkInformationIntegerKey instance("UPDATE_EXTENT_INITIALIZED",
                                           "vtkStreamingDemandDrivenPipeline");
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerKey*
vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()
{
  static vtkInformationIntegerKey instance("MAXIMUM_NUMBER_OF_PIECES",
                                           "vtkStreamingDemandDrivenPipeline");
  return &instance;
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
    return this->PropagateUpdateExtent(port) && this->UpdateData(port);
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

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update(vtkAlgorithm* algorithm, int port)
{
  return this->Superclass::Update(algorithm, port);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::ExecuteInformation()
{
  if(this->Superclass::ExecuteInformation())
    {
    // For each port, if the update extent has not been set
    // explicitly, keep it set to the whole extent.
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = this->GetOutputInformation(i);
      if(info->Has(WHOLE_EXTENT()) && !info->Has(UPDATE_EXTENT_INITIALIZED()))
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

  // If we need to update data, propagate the update extent.
  int result = 1;
  if(this->PipelineMTime > this->DataTime.GetMTime())
    {
    // Make sure input types are valid before algorithm does anything.
    if(!this->InputCountIsValid() || !this->InputTypeIsValid())
      {
      return 0;
      }

    // Make sure the update extent is inside the whole extent.
    if(!this->VerifyUpdateExtent(outputPort))
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
int vtkStreamingDemandDrivenPipeline::VerifyUpdateExtent(int outputPort)
{
  // Check all ports if index is below 0.
  if(outputPort < 0)
    {
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
      {
      if(!this->VerifyUpdateExtent(i))
        {
        return 0;
        }
      }
    }

#if 0
  // TODO: Use EXTENT_TYPE to check structured or unstructured extents.
  vtkInformation* info = this->GetOutputInformation(outputPort);
  if(info->Has(WHOLE_EXTENT()) && info->Has(UPDATE_EXTENT()) &&
     info->Has(vtkInformation::EXTENT_TYPE()))
    {
    }
#endif
  return 1;
}
