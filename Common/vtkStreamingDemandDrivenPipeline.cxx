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

vtkCxxRevisionMacro(vtkStreamingDemandDrivenPipeline, "1.3");
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
  static vtkInformationIntegerKey instance;
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerVectorKey*
vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()
{
  static vtkInformationIntegerVectorKey instance;
  return &instance;
}

//----------------------------------------------------------------------------
vtkInformationIntegerKey*
vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()
{
  static vtkInformationIntegerKey instance;
  return &instance;
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update(vtkAlgorithm* algorithm)
{
  return this->Superclass::Update(algorithm);
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::Update()
{
  if(!this->UpdateInformation())
    {
    return 0;
    }
  if(this->Algorithm->GetNumberOfOutputPorts() > 0)
    {
    return this->PropagateUpdateExtent(0) && this->UpdateData(0);
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkStreamingDemandDrivenPipeline::ExecuteInformation()
{
  if(this->Superclass::ExecuteInformation())
    {
    for(int i=0; i < this->Algorithm->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = this->GetOutputInformation(i);
      if(info->Has(WHOLE_EXTENT()) &&
         !info->Has(vtkInformation::UPDATE_EXTENT()))
        {
        int extent[6];
        info->Get(WHOLE_EXTENT(), extent);
        info->Set(vtkInformation::UPDATE_EXTENT(), extent, 6);
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
                  "Returning failure from the method.");
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

    // Request information from the algorithm.
    this->PrepareUpstreamRequest(REQUEST_UPDATE_EXTENT());
    this->GetRequestInformation()->Set(FROM_OUTPUT_PORT(), outputPort);
    this->InProcessUpstreamRequest = 1;
    result = this->Algorithm->ProcessUpstreamRequest(
      this->GetRequestInformation(), this->GetInputInformation(),
      this->GetOutputInformation());
    this->InProcessUpstreamRequest = 0;

    // Propagate the update extent to all inputs.
    if(result)
      {
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
    }
  return result;
}
