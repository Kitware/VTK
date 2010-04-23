/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSource.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include "vtkImageData.h"


class vtkSourceToDataSetFriendship
{
public:
  static void GenerateGhostLevelArray(vtkDataSet* ds)
    {
    ds->GenerateGhostLevelArray();
    }
};

//----------------------------------------------------------------------------
vtkSource::vtkSource()
{
  this->NumberOfOutputs = 0;
  this->Outputs = NULL;
  this->Updating = 0;
}

//----------------------------------------------------------------------------
vtkSource::~vtkSource()
{
  this->UnRegisterAllOutputs();
  if(this->Outputs)
    {
    delete [] this->Outputs;
    this->Outputs = NULL;
    this->NumberOfOutputs = 0;
    }
}

//----------------------------------------------------------------------------
int vtkSource::GetOutputIndex(vtkDataObject *out)
{
  int i;
  
  for (i = 0; i < this->NumberOfOutputs; i++)
    {
    if (this->Outputs[i] == out)
      {
      return i;
      }
    }
  return -1;
}


//----------------------------------------------------------------------------
vtkDataObject* vtkSource::GetOutput(int i)
{
  if(i >= 0 && i < this->NumberOfOutputs)
    {
    return this->Outputs[i];
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSource::UnRegisterAllOutputs()
{
  for(int i=0; i < this->NumberOfOutputs; ++i)
    {
    this->SetNthOutput(i, 0);
    }
}

//----------------------------------------------------------------------------
int vtkSource::GetReleaseDataFlag()
{
  if (this->GetOutput(0))
    {
    return this->GetOutput(0)->GetReleaseDataFlag();
    }
  vtkWarningMacro(<<"Output doesn't exist!");
  return 1;
}

//----------------------------------------------------------------------------
void vtkSource::SetReleaseDataFlag(int i)
{
  for (int idx = 0; idx < this->NumberOfOutputs; idx++)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->SetReleaseDataFlag(i);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSource::UpdateWholeExtent()
{
  this->UpdateInformation();

  if (this->GetOutput(0))
    {
    this->GetOutput(0)->SetUpdateExtentToWholeExtent();
    this->GetOutput(0)->Update();
    }
}

//----------------------------------------------------------------------------
void vtkSource::Update()
{
  if(vtkDemandDrivenPipeline* ddp =
     vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    ddp->Update(0);
    }
  else
    {
    vtkErrorMacro("Executive is not a vtkDemandDrivenPipeline.");
    }
}

//----------------------------------------------------------------------------
void vtkSource::UpdateInformation()
{
  if(vtkDemandDrivenPipeline* ddp =
     vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    ddp->UpdateInformation();
    }
  else
    {
    vtkErrorMacro("Executive is not a vtkDemandDrivenPipeline.");
    }
}

//----------------------------------------------------------------------------
void vtkSource::PropagateUpdateExtent(vtkDataObject* output)
{
  if(vtkStreamingDemandDrivenPipeline* sddp =
     vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    if(output)
      {
      for(int i=0; i < this->NumberOfOutputs; ++i)
        {
        if(this->Outputs[i] == output)
          {
          sddp->PropagateUpdateExtent(i);
          }
        }
      }
    else
      {
      sddp->PropagateUpdateExtent(-1);
      }
    }
}

//----------------------------------------------------------------------------
// By default we require all the input to produce the output. This is
// overridden in the subclasses since we can often produce the output with
// just a portion of the input data.
void vtkSource::ComputeInputUpdateExtents( vtkDataObject *vtkNotUsed(output) )
{
  for (int idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      this->Inputs[idx]->RequestExactExtentOn();
      this->Inputs[idx]->SetUpdateExtentToWholeExtent();
      }
    }  
}

//----------------------------------------------------------------------------

void vtkSource::TriggerAsynchronousUpdate()
{
  // check flag to avoid executing forever if there is a loop
  if (this->Updating)
    {
    return;
    }

  // Propagate the trigger to all the inputs
  this->Updating = 1;
  for (int idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      this->Inputs[idx]->TriggerAsynchronousUpdate();
      }
    }
  this->Updating = 0;
}

//----------------------------------------------------------------------------
void vtkSource::UpdateData(vtkDataObject* output)
{
  if(vtkDemandDrivenPipeline* ddp =
     vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    if(output)
      {
      for(int i=0; i < this->NumberOfOutputs; ++i)
        {
        if(this->Outputs[i] == output)
          {
          ddp->UpdateData(i);
          }
        }
      }
    else
      {
      ddp->UpdateData(-1);
      }
    }
  else
    {
    vtkErrorMacro("Executive is not a vtkDemandDrivenPipeline.");
    }
}

//----------------------------------------------------------------------------
// Assume that any source that implements ExecuteData 
// can handle an empty extent.
void vtkSource::ExecuteData(vtkDataObject *output)
{
  // I want to find out if the requested extent is empty.
  if (this->UpdateExtentIsEmpty(output) && output)
    {
    output->Initialize();
    return;
    }

  this->Execute();
}


//----------------------------------------------------------------------------
void vtkSource::SetNumberOfOutputs(int newNumberOfOutputs)
{
  if(newNumberOfOutputs < 0)
    {
    vtkErrorMacro("Cannot set number of outputs to " << newNumberOfOutputs);
    newNumberOfOutputs = 0;
    }

  if(newNumberOfOutputs != this->NumberOfOutputs)
    {
    // Copy outputs that will still exist.
    vtkDataObject** newOutputs = new vtkDataObject*[newNumberOfOutputs];
    int i;
    for(i=0; i < newNumberOfOutputs; ++i)
      {
      newOutputs[i] = (i < this->NumberOfOutputs)? this->Outputs[i] : 0;
      }

    // Delete outputs if number is decreasing.
    for(;i < this->NumberOfOutputs; ++i)
      {
      this->SetNthOutput(i, 0);
      }

    // Free old outputs array.
    if(this->Outputs)
      {
      delete [] this->Outputs;
      this->Outputs = 0;
      this->NumberOfOutputs = 0;
      }

    // Setup new outputs array.
    this->Outputs = newOutputs;
    this->NumberOfOutputs = newNumberOfOutputs;
    this->SetNumberOfOutputPorts(this->NumberOfOutputs);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSource::AddOutput(vtkDataObject* output)
{
  if(output)
    {
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      if(!this->Outputs[i])
        {
        this->SetNthOutput(i, output);
        return;
        }
      }
    this->SetNthOutput(this->GetNumberOfOutputPorts(), output);
    }
}

//----------------------------------------------------------------------------
void vtkSource::RemoveOutput(vtkDataObject* output)
{
  if(output)
    {
    for(int i=0; i < this->NumberOfOutputs; ++i)
      {
      if(this->Outputs[i] == output)
        {
        this->SetNthOutput(i, 0);
        return;
        }
      }
    vtkErrorMacro("Could not remove " << output->GetClassName()
                  << "(" << output << ") because it is not an output.");
    }
}

//----------------------------------------------------------------------------
void vtkSource::SetNthOutput(int index, vtkDataObject* newOutput)
{
  if(index < 0)
    {
    vtkErrorMacro("SetNthOutput: " << index << ", cannot set output. ");
    return;
    }

  if(index >= this->NumberOfOutputs)
    {
    this->SetNumberOfOutputs(index+1);
    }

  vtkDataObject* oldOutput = this->Outputs[index];
  if(newOutput == oldOutput)
    {
    return;
    }

  // Ask the executive to setup the new output.
  this->GetExecutive()->SetOutputData(index, newOutput);

  this->InvokeEvent(vtkCommand::SetOutputEvent,NULL);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSource::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass"
    " and you should really use ExecuteData(vtkDataObject *) instead");
}

//----------------------------------------------------------------------------
vtkDataObject** vtkSource::GetOutputs()
{
  return this->Outputs;
}


//----------------------------------------------------------------------------

// Default implementation - copy information from first input to all outputs
void vtkSource::ExecuteInformation()
{
  vtkDataObject *input, *output;

  if (this->Inputs && this->Inputs[0])
    {
    input = this->Inputs[0];

    for (int idx = 0; idx < this->NumberOfOutputs; ++idx)
      {
      output = this->GetOutput(idx);
      if (output)
        {
        output->CopyInformation(input);
        }  
      }
    }
  else
    {
    for (int idx = 0; idx < this->NumberOfOutputs; ++idx)
      {
      output = this->GetOutput(idx);
      if (output)
        {
        // Since most unstructured filters in VTK generate all their data once,
        // make it the default.
        // protected: if ( output->GetExtentType() == VTK_PIECES_EXTENT )
        if (output->IsA("vtkPolyData") || output->IsA("vtkUnstructuredGrid"))
          {
          output->SetMaximumNumberOfPieces(1);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->NumberOfOutputs)
    {
    int idx;
    for (idx = 0; idx < this->NumberOfOutputs; ++idx)
      {
      os << indent << "Output " << idx << ": (" << this->Outputs[idx] << ")\n";
      }
    }
  else
    {
    os << indent <<"No Outputs\n";
    }
}

//----------------------------------------------------------------------------
int vtkSource::FillOutputPortInformation(int port, vtkInformation* info)
{
  return this->Superclass::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
void vtkSource::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  for(int i=0; i < this->NumberOfOutputs; ++i)
    {
    vtkGarbageCollectorReport(collector, this->Outputs[i], "Outputs");
    }
}

//----------------------------------------------------------------------------
void vtkSource::SetExecutive(vtkExecutive* executive)
{
  // Set the executive normally.
  this->Superclass::SetExecutive(executive);
  // Copy our set of outputs to the information objects in the
  // executive.
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    this->GetExecutive()->SetOutputData(i, this->Outputs[i]);
    }
}

//----------------------------------------------------------------------------
void vtkSource::SetNumberOfOutputPorts(int n)
{
  if(n != this->GetNumberOfOutputPorts())
    {
    this->Superclass::SetNumberOfOutputPorts(n);
    this->SetNumberOfOutputs(n);
    }
}

//----------------------------------------------------------------------------
int vtkSource::ProcessRequest(vtkInformation* request,
                              vtkInformationVector** inputVector,
                              vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    // The compatibility layer always keeps output data objects around
    // because they are needed for connections.
    return 1;
    }
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    // Make sure the outputs are synchronized between the old and new
    // style pipelines.
    int i;
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      vtkInformation* info = this->GetExecutive()->GetOutputInformation(i);
      this->SetNthOutput(i, info->Get(vtkDataObject::DATA_OBJECT()));
      }

    vtkDebugMacro("ProcessRequest(REQUEST_INFORMATION) "
                  "calling ExecuteInformation.");

    // Old-style filters will get origin and spacing from the input
    // data objects themselves.  We handle this here by copying the
    // pipeline information version of these values into the data
    // object's version.
    for(i=0; i < this->NumberOfInputs; ++i)
      {
      vtkInformation* info = inputVector[0]->GetInformationObject(i);
      if(vtkImageData* id = vtkImageData::SafeDownCast(
           info->Get(vtkDataObject::DATA_OBJECT())))
        {
        if(info->Has(vtkDataObject::ORIGIN()))
          {
          id->SetOrigin(info->Get(vtkDataObject::ORIGIN()));
          }
        if(info->Has(vtkDataObject::SPACING()))
          {
          id->SetSpacing(info->Get(vtkDataObject::SPACING()));
          }
        }
      }

    // Ask the subclass to fill in the information for the outputs.
    this->InvokeEvent(vtkCommand::ExecuteInformationEvent, NULL);
    this->ExecuteInformation();

    // The number of outputs may have been changed by execute information.
    outputVector->SetNumberOfInformationObjects(this->NumberOfOutputs);

    // Old-style filters will set the origin and spacing on the output
    // data objects themselves.  We handle this here by copying the
    // information back to the pipeline information.
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      if(vtkImageData* id = vtkImageData::SafeDownCast(
           info->Get(vtkDataObject::DATA_OBJECT())))
        {
        info->Set(vtkDataObject::ORIGIN(), id->GetOrigin(), 3);
        info->Set(vtkDataObject::SPACING(), id->GetSpacing(), 3);
        }
      }

    return 1;
    }
  else if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    int i;
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      vtkInformation* info = this->GetExecutive()->GetOutputInformation(i);
      this->SetNthOutput(i, info->Get(vtkDataObject::DATA_OBJECT()));
      }

    // If the user defines a ComputeInputUpdateExtent method, we want
    // RequestExactUpdateExtent to be off by default (User does
    // nothing else).  Otherwise, the ComputeInputUpdateExtent in this
    // superclass sets RequestExactExtent to on.  The reason for this
    // initialization here is if this sources shares an input with
    // another, we do not want the input's RequestExactExtent "state"
    // to interfere with each other.
    for(i=0; i < this->NumberOfInputs; ++i)
      {
      if(this->Inputs[i])
        {
        this->Inputs[i]->RequestExactExtentOff();
        }
      }

    // Check inputs.
    if(this->NumberOfRequiredInputs > 0 &&
       this->GetNumberOfInputPorts() < 1)
      {
      vtkErrorMacro("This filter requires " << this->NumberOfRequiredInputs
                    << " input(s) but has no input ports.  A call to "
                    << "SetNumberOfInputPorts and an implementation of "
                    << "FillInputPortInformation may need to be added to "
                    << "this class.");
      return 0;
      }

    int outputPort =
      request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());
    vtkDataObject* fromOutput = 0;
    if(outputPort >= 0)
      {
      fromOutput = this->Outputs[outputPort];
      }

    // Give the subclass a chance to request a larger extent on the
    // inputs. This is necessary when, for example, a filter requires
    // more data at the "internal" boundaries to produce the boundary
    // values - such as an image filter that derives a new pixel value
    // by applying some operation to a neighborhood of surrounding
    // original values.
    vtkDebugMacro("ProcessRequest(REQUEST_UPDATE_EXTENT) "
                  "calling ComputeInputUpdateExtents using output port "
                  << outputPort);
    this->ComputeInputUpdateExtents(fromOutput);

    return 1;
    }
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_NOT_GENERATED()))
    {
    // Mark all outputs as not generated so that the executive does
    // not try to handle initialization/finalization of the outputs.
    // We will do it here.
    int i;
    for(i=0; i < outputVector->GetNumberOfInformationObjects(); ++i)
      {
      vtkInformation* outInfo = outputVector->GetInformationObject(i);
      outInfo->Set(vtkDemandDrivenPipeline::DATA_NOT_GENERATED(), 1);
      }
    }
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    // Make sure the outputs are synchronized between the old and new
    // style pipelines.
    int i;
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      vtkInformation* info = this->GetExecutive()->GetOutputInformation(i);
      this->SetNthOutput(i, info->Get(vtkDataObject::DATA_OBJECT()));
      }
    int outputPort = request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

    // Check inputs.
    if(this->NumberOfRequiredInputs > 0 &&
       this->GetNumberOfInputPorts() < 1)
      {
      vtkErrorMacro("This filter requires " << this->NumberOfRequiredInputs
                    << " input(s) but has no input ports.  A call to "
                    << "SetNumberOfInputPorts and an implementation of "
                    << "FillInputPortInformation may need to be added to "
                    << "this class.");
      return 0;
      }

    vtkDebugMacro("ProcessRequest(REQUEST_DATA) "
                  "calling ExecuteData for output port " << outputPort);

    // Prepare to execute the filter.
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      if(this->Outputs[i])
        {
        this->Outputs[i]->PrepareForNewData();
        }
      }
    
    // Pass the vtkDataObject's field data from the first input to all
    // outputs.
    if(this->NumberOfInputs > 0 && this->Inputs[0] &&
       this->Inputs[0]->GetFieldData())
      {
      for(i=0; i < this->NumberOfOutputs; ++i)
        {
        if(this->Outputs[i] && this->Outputs[i]->GetFieldData())
          {
          this->Outputs[i]->GetFieldData()->PassData(
            this->Inputs[0]->GetFieldData());
          }
        }
      }

    // Execute the filter.
    vtkDataObject* output = (outputPort >= 0)? this->Outputs[outputPort] : 0;
    this->ExecuteData(output);

    // Mark the data as up-to-date.
    this->MarkGeneratedOutputs(output);

    // Cleanup the outputs.
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);

      // Old-style filters will set the origin and spacing on the output
      // data objects themselves.  We handle this here by copying the
      // information back to the pipeline information.
      if(vtkImageData* id = vtkImageData::SafeDownCast(
           info->Get(vtkDataObject::DATA_OBJECT())))
        {
        info->Set(vtkDataObject::ORIGIN(), id->GetOrigin(), 3);
        info->Set(vtkDataObject::SPACING(), id->GetSpacing(), 3);
        }

      // Compute the ghost level array for the output if necessary.
      if(vtkDataSet* ds = vtkDataSet::SafeDownCast(this->Outputs[i]))
        {
        vtkSourceToDataSetFriendship::GenerateGhostLevelArray(ds);
        }
      }
    return 1;
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkSource::MarkGeneratedOutputs(vtkDataObject*)
{
  // Mark the data as up-to-date.  Mark all outputs by default.
  for(int i=0; i < this->NumberOfOutputs; ++i)
    {
    if(this->Outputs[i])
      {
      this->Outputs[i]->DataHasBeenGenerated();
      // Assume that the algorithm produced the required data unless the
      // algorithm sets otherwise.
      vtkInformation* dataInfo = this->Outputs[i]->GetInformation();
      if (!dataInfo->Has(vtkDataObject::DATA_PIECE_NUMBER()) ||
          dataInfo->Get(vtkDataObject::DATA_PIECE_NUMBER()) == - 1)
        {
        dataInfo->Set(vtkDataObject::DATA_PIECE_NUMBER(), 
                      this->Outputs[i]->GetUpdatePiece());
        dataInfo->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 
                      this->Outputs[i]->GetUpdateNumberOfPieces());
        dataInfo->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 
                      this->Outputs[i]->GetUpdateGhostLevel());
        }

      }
    }
}
