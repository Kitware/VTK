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

vtkCxxRevisionMacro(vtkSource, "1.2");

#ifndef NULL
#define NULL 0
#endif

class vtkSourceToDataObjectFriendship
{
public:
  static int UpdateExtentInitialized(vtkDataObject* obj)
    {
    return obj->UpdateExtentInitialized;
    }
  static void CopyUpstreamIVarsToInformation(vtkDataObject* obj,
                                             vtkInformation* info)
    {
    obj->CopyUpstreamIVarsToInformation(info);
    }
  static void CopyUpstreamIVarsFromInformation(vtkDataObject* obj,
                                               vtkInformation* info)
    {
    obj->CopyUpstreamIVarsFromInformation(info);
    }
  static void CopyDownstreamIVarsToInformation(vtkDataObject* obj,
                                               vtkInformation* info)
    {
    obj->CopyDownstreamIVarsToInformation(info);
    }
  static void CopyDownstreamIVarsFromInformation(vtkDataObject* obj,
                                                 vtkInformation* info)
    {
    obj->CopyDownstreamIVarsFromInformation(info);
    }
  static void Crop(vtkDataObject* obj)
    {
    obj->Crop();
    }
};

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
#ifdef VTK_USE_EXECUTIVES
  for(int i=0; i < this->NumberOfOutputs; ++i)
    {
    this->SetNthOutput(i, 0);
    }
#else
  int idx;

  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->SetSource(NULL);
      this->Outputs[idx]->UnRegister(this);
      this->Outputs[idx] = NULL;
      }
    }
#endif
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
  int idx;
  
  for (idx = 0; idx < this->NumberOfOutputs; idx++)
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
#ifdef VTK_USE_EXECUTIVES
  if(vtkDemandDrivenPipeline* ddp =
     vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    if(vtkStreamingDemandDrivenPipeline* sddp =
       vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
      {
      // Synchronize ivars for compatibility layer.
      if(this->NumberOfOutputs > 0)
        {
        vtkSourceToDataObjectFriendship
          ::CopyUpstreamIVarsToInformation(this->Outputs[0],
                                           sddp->GetOutputInformation(0));
        }
      }
    ddp->Update(this, 0);
    }
  else
    {
    vtkErrorMacro("Executive is not a vtkDemandDrivenPipeline.");
    }
#else
  if (this->GetOutput(0))
    {
    this->GetOutput(0)->Update();
    if ( this->GetOutput(0)->GetSource() )
      {
      this->SetErrorCode( this->GetOutput(0)->GetSource()->GetErrorCode() );
      }
    }
#endif
}

//----------------------------------------------------------------------------
void vtkSource::UpdateInformation()
{
#ifdef VTK_USE_EXECUTIVES
  if(vtkDemandDrivenPipeline* ddp =
     vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    ddp->UpdateInformation();
    }
  else
    {
    vtkErrorMacro("Executive is not a vtkDemandDrivenPipeline.");
    }
#else
  unsigned long t1, t2;
  int           idx;
  vtkDataObject *input;
  vtkDataObject *output;
  float         maxLocality = 0.0;
  float         locality;

  // Watch out for loops in the pipeline
  if ( this->Updating )
    {
    // Since we are in a loop, we will want to execute on every call to update.
    // We set the pipline mtimes of our outputs 
    // to ensure the pipeline executes again. 
    this->Modified();
    for (idx = 0; idx < this->NumberOfOutputs; ++idx)
      {
      output = this->GetOutput(idx);
      if (output)
        {
        output->SetPipelineMTime(this->GetMTime());
        }  
      }
    return;
    }

  // The MTime of this source will be used in determine the PipelineMTime
  // for the outputs
  t1 = this->GetMTime();

  // Loop through the inputs
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      input = this->Inputs[idx];

      // Propagate the UpdateInformation call
      this->Updating = 1;
      input->UpdateInformation();
      this->Updating = 0;
      
      // Compute the max locality of the inputs.
      locality = input->GetLocality();
      if (locality > maxLocality)
        {
        maxLocality = locality;
        }

      // What is the PipelineMTime of this input? Compare this against
      // our current computation to find the largest one.
      t2 = input->GetPipelineMTime();

      if (t2 > t1)
        {
        t1 = t2;
        }
      }
    }
  locality = maxLocality * 0.5;


  // Call ExecuteInformation for subclass specific information.
  // Since UpdateInformation propagates all the way up the pipeline,
  // we need to be careful here to call ExecuteInformation only if necessary.
  if (t1 > this->InformationTime.GetMTime())
    {
    for (idx = 0; idx < this->NumberOfOutputs; ++idx)
      {
      output = this->GetOutput(idx);
      if (output)
        {
        output->SetPipelineMTime(t1);
        output->SetLocality(locality);
        }  
      }
    
    this->InvokeEvent(vtkCommand::ExecuteInformationEvent, NULL);
    this->ExecuteInformation();

    // Information gets invalidated as soon as Update is called,
    // so validate it again here.
    this->InformationTime.Modified();
    }
#endif
}

//----------------------------------------------------------------------------
void vtkSource::PropagateUpdateExtent(vtkDataObject* output)
{
#ifdef VTK_USE_EXECUTIVES
  if(vtkStreamingDemandDrivenPipeline* sddp =
     vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
    {
    if(output)
      {
      for(int i=0; i < this->NumberOfOutputs; ++i)
        {
        if(this->Outputs[i] == output)
          {
          // Synchronize ivars for compatibility layer.
          vtkSourceToDataObjectFriendship
            ::CopyUpstreamIVarsToInformation(this->Outputs[i],
                                             sddp->GetOutputInformation(i));
          sddp->PropagateUpdateExtent(i);
          }
        }
      }
    else
      {
      // Synchronize ivars for compatibility layer.
      for(int i=0; i < this->NumberOfOutputs; ++i)
        {
        if(this->Outputs[i])
          {
          vtkSourceToDataObjectFriendship
            ::CopyUpstreamIVarsToInformation(this->Outputs[i],
                                             sddp->GetOutputInformation(i));
          }
        }
      sddp->PropagateUpdateExtent(-1);
      }
    }
#else
  int idx;

  // Check flag to avoid executing forever if there is a loop.
  if (this->Updating || this->NumberOfInputs == 0)
    {
    return;
    }

  // If the user defines a ComputeInputUpdateExtent method,
  // I want RequestExactUpdateExtent to be off by default (User does nothing else).
  // Otherwise, the ComputeInputUpdateExtent in this superclass sets
  // RequestExactExtent to on.  The reason for this initialization here is 
  // if this sources shares an input with another, we do not want the input's
  // RequestExactExtent "state" to interfere with each other.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      this->Inputs[idx]->RequestExactExtentOff();
      }
    }  
      
  // Give the subclass a chance to request a larger extent on 
  // the inputs. This is necessary when, for example, a filter
  // requires more data at the "internal" boundaries to 
  // produce the boundary values - such as an image filter that
  // derives a new pixel value by applying some operation to a 
  // neighborhood of surrounding original values. 
  this->ComputeInputUpdateExtents( output );

  // Now that we know the input update extent, propogate this
  // through all the inputs.
  this->Updating = 1;
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      this->Inputs[idx]->PropagateUpdateExtent();
      }
    }
  this->Updating = 0;
#endif
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
#ifdef VTK_USE_EXECUTIVES
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
#else
  int idx;

  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }

  // Propagate the update call - make sure everything we
  // might rely on is up-to-date
  // Must call PropagateUpdateExtent before UpdateData if multiple 
  // inputs since they may lead back to the same data object.
  this->Updating = 1;
  if ( this->NumberOfInputs == 1 )
    {
    if (this->Inputs[0] != NULL)
      {
      this->Inputs[0]->UpdateData();
      }
    }
  else
    { // To avoid serlializing execution of pipelines with ports
    // we need to sort the inputs by locality (ascending).
    this->SortInputsByLocality();
    for (idx = 0; idx < this->NumberOfInputs; ++idx)
      {
      if (this->SortedInputs[idx] != NULL)
        {
        this->SortedInputs[idx]->PropagateUpdateExtent();
        this->SortedInputs[idx]->UpdateData();
        }
      }
    }

    
  // Initialize all the outputs
  for (idx = 0; idx < this->NumberOfOutputs; idx++)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->PrepareForNewData(); 
      }
    }
 
  // If there is a start method, call it
  this->InvokeEvent(vtkCommand::StartEvent,NULL);

  // Execute this object - we have not aborted yet, and our progress
  // before we start to execute is 0.0.
  this->AbortExecute = 0;
  this->Progress = 0.0;

  int skipExecute = 0;
  if (this->NumberOfInputs < this->NumberOfRequiredInputs)
    {
    vtkErrorMacro(<< "At least " << this->NumberOfRequiredInputs 
                  << " inputs are required but only " << this->NumberOfInputs 
                  << " are specified. Skipping execution.");
    skipExecute = 1;
    }
  else
    {
    for (idx = 0; idx < this->NumberOfRequiredInputs; ++idx)
      {
      if (!this->Inputs[idx])
        {
        vtkErrorMacro(<< "Required input " << idx 
                      << " is not assigned. Skipping execution.");
        skipExecute = 1;
        }
      }
    }

  // This condition gives the default behavior if the user asks
  // for a piece that cannot be generated by the source.
  // Just ignore the request and return empty.
  if (output && output->GetMaximumNumberOfPieces() > 0 &&
      output->GetUpdatePiece() >= output->GetMaximumNumberOfPieces())
    {
    skipExecute = 1;
    }

  if (!skipExecute)
    {
    // Pass the vtkDataObject's field data from the first input
    // to all outputs
    vtkFieldData* fd;
    if ((this->NumberOfInputs > 0) && (this->Inputs[0]) && 
        (fd = this->Inputs[0]->GetFieldData()))
      {
      vtkFieldData* outputFd;
      for (idx = 0; idx < this->NumberOfOutputs; idx++)
        {
        if (this->Outputs[idx] && 
            (outputFd=this->Outputs[idx]->GetFieldData()))
          {
          outputFd->PassData(fd);
          }
        }
      }
    this->ExecuteData(output);
    }

  // If we ended due to aborting, push the progress up to 1.0 (since
  // it probably didn't end there)
  if ( !this->AbortExecute )
    {
    this->UpdateProgress(1.0);
    }

  // Call the end method, if there is one
  this->InvokeEvent(vtkCommand::EndEvent,NULL);
    
  // Now we have to mark the data as up to data.
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->DataHasBeenGenerated();
      }
    }
  
  // Release any inputs if marked for release
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      if ( this->Inputs[idx]->ShouldIReleaseData() )
        {
        this->Inputs[idx]->ReleaseData();
        }
      }  
    }

  this->Updating = 0;

  // Cleanup the outputs.
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if(this->Outputs[idx] && this->Outputs[idx]->GetRequestExactExtent())
      {
      vtkSourceToDataObjectFriendship::Crop(this->Outputs[idx]);
      }
    if(vtkDataSet* ds = vtkDataSet::SafeDownCast(this->Outputs[idx]))
      {
      vtkSourceToDataSetFriendship::GenerateGhostLevelArray(ds);
      }
    }
#endif
}

//----------------------------------------------------------------------------
int vtkSource::UpdateExtentIsEmpty(vtkDataObject *output)
{
  if (output == NULL)
    {
    return 1;
    }

  int *ext = output->GetUpdateExtent();
  switch ( output->GetExtentType() )
    {
    case VTK_PIECES_EXTENT:
      // Special way of asking for no input.
      if ( output->GetUpdateNumberOfPieces() == 0 )
        {
        return 1;
        }
      break;

    case VTK_3D_EXTENT:
      // Special way of asking for no input. (zero volume)
      if (ext[0] == (ext[1] + 1) ||
          ext[2] == (ext[3] + 1) ||
          ext[4] == (ext[5] + 1))
      {
      return 1;
      }
      break;

    // We should never have this case occur
    default:
      vtkErrorMacro( << "Internal error - invalid extent type!" );
      break;
    }

  return 0;
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
#ifdef VTK_USE_EXECUTIVES
void vtkSource::SetNumberOfOutputs(int newNumberOfOutputs)
{
  if(newNumberOfOutputs < 0)
    {
    vtkErrorMacro("Cannot set number of outputs to " << newNumberOfOutputs);
    newNumberOfOutputs = 0;
    }

  if(newNumberOfOutputs != this->NumberOfOutputs)
    {
    vtkDataObject** newOutputs = new vtkDataObject*[newNumberOfOutputs];
    for(int i=0; i < newNumberOfOutputs; ++i)
      {
      newOutputs[i] = (i < this->NumberOfOutputs)? this->Outputs[i] : 0;
      }

    if(this->Outputs)
      {
      delete [] this->Outputs;
      this->Outputs = 0;
      this->NumberOfOutputs = 0;
      }

    this->Outputs = newOutputs;
    this->NumberOfOutputs = newNumberOfOutputs;
    this->SetNumberOfOutputPorts(this->NumberOfOutputs);
    this->Modified();
    }
}
#else
void vtkSource::SetNumberOfOutputs(int num)
{
  int idx;
  vtkDataObject **outputs;

  // in case nothing has changed.
  if (num == this->NumberOfOutputs)
    {
    return;
    }

  // Destroy extra outputs if decreasing number of outputs.
  if(num < this->NumberOfOutputs)
    {
    for(idx=num; idx < this->NumberOfOutputs; ++idx)
      {
      this->SetNthOutput(idx, 0);
      }
    }
  
  // Allocate new arrays.
  outputs = new vtkDataObject *[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    outputs[idx] = NULL;
    }

  // Copy old outputs
  for (idx = 0; idx < num && idx < this->NumberOfOutputs; ++idx)
    {
    outputs[idx] = this->Outputs[idx];
    }
  
  // delete the previous arrays
  if (this->Outputs)
    {
    delete [] this->Outputs;
    this->Outputs = NULL;
    this->NumberOfOutputs = 0;
    }
  
  // Set the new arrays
  this->Outputs = outputs;
  
  this->NumberOfOutputs = num;
  this->Modified();
}
#endif

//----------------------------------------------------------------------------
void vtkSource::AddOutput(vtkDataObject* output)
{
#ifdef VTK_USE_EXECUTIVES
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
#else
  int idx;
  
  if (output)
    {
    output->SetSource(this);
    output->Register(this);
    }
  this->Modified();
  
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx] == NULL)
      {
      this->Outputs[idx] = output;
      return;
      }
    }
  
  this->SetNumberOfOutputs(this->NumberOfOutputs + 1);
  this->Outputs[this->NumberOfOutputs - 1] = output;
#endif
}

//----------------------------------------------------------------------------
void vtkSource::RemoveOutput(vtkDataObject* output)
{
#ifdef VTK_USE_EXECUTIVES
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
#else
  int idx, loc;
  
  if (!output)
    {
    return;
    }
  
  // find the output in the list of outputs
  loc = -1;
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx] == output)
      {
      loc = idx;
      }
    }
  if (loc == -1)
    {
    vtkDebugMacro("tried to remove an output that was not in the list");
    return;
    }
  
  this->Outputs[loc]->SetSource(NULL);
  this->Outputs[loc]->UnRegister(this);
  this->Outputs[loc] = NULL;

  // if that was the last output, then shrink the list
  if (loc == this->NumberOfOutputs - 1)
    {
    this->SetNumberOfOutputs(this->NumberOfOutputs - 1);
    }
  
  this->Modified();
#endif
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_EXECUTIVES
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

  vtkInformation* info =
    this->GetExecutive()->GetOutputInformation(this, index);

  if(oldOutput)
    {
    oldOutput->SetSource(0);
    oldOutput->UnRegister(this);
    this->Outputs[index] = 0;
    info->Set(vtkDataObject::DATA_OBJECT(), 0);
    }

  if(newOutput)
    {
    newOutput->Register(this);
    if(vtkSource* oldSource = newOutput->GetSource())
      {
      oldSource->RemoveOutput(newOutput);
      }
    info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    this->Outputs[index] = newOutput;
    newOutput->SetSource(this);
    }

  this->InvokeEvent(vtkCommand::SetOutputEvent,NULL);

  this->Modified();
}
#else
void vtkSource::SetNthOutput(int idx, vtkDataObject *newOutput)
{
  vtkDataObject *oldOutput;
  
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthOutput: " << idx << ", cannot set output. ");
    return;
    }
  // Expand array if necessary.
  if (idx >= this->NumberOfOutputs)
    {
    this->SetNumberOfOutputs(idx + 1);
    }
  
  // does this change anything?
  oldOutput = this->Outputs[idx];
  if (newOutput == oldOutput)
    {
    return;
    }
  
  // disconnect first existing source-output relationship.
  if (oldOutput)
    {
    oldOutput->SetSource(NULL);
    oldOutput->UnRegister(this);
    this->Outputs[idx] = NULL;
    }
  
  if (newOutput)
    {
    vtkSource *newOutputOldSource = newOutput->GetSource();

    // Register the newOutput so it does not get deleted.
    // Don't set the link yet until previous links is disconnected.
    newOutput->Register(this);
    
    // disconnect second existing source-output relationship
    if (newOutputOldSource)
      {
      newOutputOldSource->RemoveOutput(newOutput);
      }
    newOutput->SetSource(this);
    }
  // now actually make the link that was registered previously.
  this->Outputs[idx] = newOutput;

  this->InvokeEvent(vtkCommand::SetOutputEvent,NULL);

  this->Modified();
}
#endif

//----------------------------------------------------------------------------
void vtkSource::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass and you should really use ExecuteData(vtkDataObject *) instead");
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
    collector->ReportReference(this->Outputs[i], "Outputs");
    }
}

//----------------------------------------------------------------------------
void vtkSource::RemoveReferences()
{
  for(int i=0; i < this->NumberOfOutputs; ++i)
    {
    if(this->Outputs[i])
      {
      this->Outputs[i]->UnRegister(this);
      this->Outputs[i] = 0;
      }
    }
  this->Superclass::RemoveReferences();
}

//----------------------------------------------------------------------------
void vtkSource::SetExecutive(vtkExecutive* executive)
{
  // Set the executive normally.
  this->Superclass::SetExecutive(executive);
#ifdef VTK_USE_EXECUTIVES
  // Copy our set of outputs to the information objects in the
  // executive.
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = this->GetExecutive()->GetOutputInformation(this, i);
    info->Set(vtkDataObject::DATA_OBJECT(), this->Outputs[i]);
    }
#endif
}

//----------------------------------------------------------------------------
void vtkSource::SetNumberOfOutputPorts(int n)
{
  if(n != this->GetNumberOfOutputPorts())
    {
    this->Superclass::SetNumberOfOutputPorts(n);
#ifdef VTK_USE_EXECUTIVES
    this->SetNumberOfOutputs(n);
#endif
    }
}

//----------------------------------------------------------------------------
int vtkSource::ProcessUpstreamRequest(vtkInformation* request,
                                      vtkInformationVector* inputVector,
                                      vtkInformationVector* outputVector)
{
#ifdef VTK_USE_EXECUTIVES
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    int i;
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      vtkInformation* info =
        this->GetExecutive()->GetOutputInformation(this, i);
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

    // Copy the information from the the information objects to
    // synchronize ivars for compatibility layer.
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      if(this->Outputs[i])
        {
        vtkInformation* info = outputVector->GetInformationObject(i);
        vtkSourceToDataObjectFriendship::
          CopyUpstreamIVarsFromInformation(this->Outputs[i], info);
        }
      }

    // Give the subclass a chance to request a larger extent on the
    // inputs. This is necessary when, for example, a filter requires
    // more data at the "internal" boundaries to produce the boundary
    // values - such as an image filter that derives a new pixel value
    // by applying some operation to a neighborhood of surrounding
    // original values.
    vtkDebugMacro("ProcessUpstreamRequest(REQUEST_UPDATE_EXTENT) "
                  "calling ComputeInputUpdateExtents using output port "
                  << outputPort);
    this->ComputeInputUpdateExtents(fromOutput);

    // Copy the resulting information back into the information
    // objects to synchronize ivars for compatibility layer.
    for(i=0; i < this->NumberOfInputs; ++i)
      {
      if(this->Inputs[i])
        {
        vtkInformation* info =
          inputVector->GetInformationObject(0)
          ->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION())
          ->GetInformationObject(i);
        vtkSourceToDataObjectFriendship::
          CopyUpstreamIVarsToInformation(this->Inputs[i], info);
        }
      }
    return 1;
    }
  return 0;
#else
  return this->Superclass::ProcessUpstreamRequest(request, inputVector,
                                                  outputVector);
#endif
}

//----------------------------------------------------------------------------
int vtkSource::ProcessDownstreamRequest(vtkInformation* request,
                                        vtkInformationVector* inputVector,
                                        vtkInformationVector* outputVector)
{
#ifdef VTK_USE_EXECUTIVES
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    // Make sure the outputs are synchronized between the old and new
    // style pipelines.
    int i;
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      vtkInformation* info =
        this->GetExecutive()->GetOutputInformation(this, i);
      this->SetNthOutput(i, info->Get(vtkDataObject::DATA_OBJECT()));
      }

    // Copy whole extent from information objects into data objects
    // for backward compatibility.  This is necessary when the
    // producer of the input is not part of the backward compatibility
    // layer and therefore does not actually set this information on
    // the data object.
    for(i=0; i < this->NumberOfInputs; ++i)
      {
      if(this->Inputs[i])
        {
        vtkInformation* info = inputVector->GetInformationObject(0)
          ->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION())
          ->GetInformationObject(i);
        vtkSourceToDataObjectFriendship::
          CopyDownstreamIVarsFromInformation(this->Inputs[i], info);
        }
      }

    vtkDebugMacro("ProcessDownstreamRequest(REQUEST_INFORMATION) "
                  "calling ExecuteInformation.");

    // Ask the subclass to fill in the information for the outputs.
    this->InvokeEvent(vtkCommand::ExecuteInformationEvent, NULL);
    this->ExecuteInformation();

    // Copy the resulting information back into the information
    // objects to synchronize ivars for compatibility layer.
    outputVector->SetNumberOfInformationObjects(this->NumberOfOutputs);
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      if(this->Outputs[i])
        {
        vtkInformation* info = outputVector->GetInformationObject(i);
        vtkSourceToDataObjectFriendship
          ::CopyDownstreamIVarsToInformation(this->Outputs[i], info);
        }
      }
    return 1;
    }
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    // Make sure the outputs are synchronized between the old and new
    // style pipelines.
    int i;
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      vtkInformation* info =
        this->GetExecutive()->GetOutputInformation(this, i);
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

    vtkDebugMacro("ProcessDownstreamRequest(REQUEST_DATA) "
                  "calling ExecuteData for output port " << outputPort);

    // Prepare to execute the filter.
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      if(this->Outputs[i])
        {
        this->Outputs[i]->PrepareForNewData();
        }
      }
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    this->AbortExecute = 0;
    this->Progress = 0.0;

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
    this->ExecuteData((outputPort >= 0)? this->Outputs[outputPort] : 0);
    if(!this->AbortExecute)
      {
      this->UpdateProgress(1.0);
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    // Mark the data as up-to-date.
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      if(this->Outputs[i])
        {
        this->Outputs[i]->DataHasBeenGenerated();
        }
      }

    // Release any inputs if marked for release.
    for(i=0; i < this->NumberOfInputs; ++i)
      {
      if(this->Inputs[i] && this->Inputs[i]->ShouldIReleaseData())
        {
        this->Inputs[i]->ReleaseData();
        }
      }

    // Cleanup the outputs.
    for(i=0; i < this->NumberOfOutputs; ++i)
      {
      if(this->Outputs[i] && this->Outputs[i]->GetRequestExactExtent())
        {
        vtkSourceToDataObjectFriendship::Crop(this->Outputs[i]);
        }
      if(vtkDataSet* ds = vtkDataSet::SafeDownCast(this->Outputs[i]))
        {
        vtkSourceToDataSetFriendship::GenerateGhostLevelArray(ds);
        }
      }
    return 1;
    }
  return 0;
#else
  return this->Superclass::ProcessDownstreamRequest(request, inputVector,
                                                    outputVector);
#endif
}
