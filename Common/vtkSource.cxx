/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSource.cxx
 Language:  C++
 Date:      $Date$
 Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkSource.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"


//----------------------------------------------------------------------------
vtkSource* vtkSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSource");
  if(ret)
    {
    return (vtkSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSource;
}




#ifndef NULL
#define NULL 0
#endif

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
  UnRegisterAllOutputs();
  if (this->Outputs)
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
vtkDataObject *vtkSource::GetOutput(int i)
{
  if (this->NumberOfOutputs < i+1)
    {
    return NULL;
    }

  return this->Outputs[i];
}

//----------------------------------------------------------------------------
void vtkSource::UnRegisterAllOutputs(void)
{
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
  if (this->GetOutput(0))
    {
    this->GetOutput(0)->Update();
    }
}

//----------------------------------------------------------------------------

void vtkSource::UpdateInformation()
{
  unsigned long t1, t2;
  int           idx;
  vtkDataObject *input;
  vtkDataObject *output;
  float         maxLocality = 0.0;
  float         locality;

  // Watch out for loops in the pipeline
  if ( this->Updating )
    {
    // Since we are in a loop, we will want to update. But if
    // we don't modify this filter, then we will not execute
    // because our InformationTime will be more recent than
    // the MTime of our output.
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
  // Otherwise, we may cause this source to be modified which will cause it
  // to execute again on the next update.
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
    
    this->ExecuteInformation();
    }
}

//----------------------------------------------------------------------------

void vtkSource::PropagateUpdateExtent(vtkDataObject *output)
{
  int idx;

  // check flag to avoid executing forever if there is a loop
  if (this->Updating)
    {
    return;
    }

  // Make sure the fitler does not implement this legacy method.
  this->LegacyHack = 1;
  this->EnlargeOutputUpdateExtents(output);
  if (this->LegacyHack)
    {
    vtkErrorMacro("EnlargeOutputUpdateExtent is no longer being supported."
	<< " This method was used by inmaging filters to change the"
	<< "UpdateExtent of their input so that the vtkImmageToImageFilter superclass"
	<< " would allocate a larger volume.  Changing the UpdateExtent of your input is"
	<< " no longer allowed.  The alternative method is to write your own 'ExecuteData(vtkDataObject *)'"
	<< " method and allocate your own data.");  
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

void vtkSource::UpdateData(vtkDataObject *output)
{
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
  this->Updating = 0;	  
    
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
  if (this->NumberOfInputs < this->NumberOfRequiredInputs)
    {
    vtkErrorMacro(<< "At least " << this->NumberOfRequiredInputs << " inputs are required but only " << this->NumberOfInputs << " are specified");
    }
  else
    {
    this->ExecuteData(output);
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
  
  // Information gets invalidated as soon as Update is called,
  // so validate it again here.
  this->InformationTime.Modified();
}

//----------------------------------------------------------------------------

// Called by constructor to set up output array.
void vtkSource::SetNumberOfOutputs(int num)
{
  int idx;
  vtkDataObject **outputs;

  // in case nothing has changed.
  if (num == this->NumberOfOutputs)
    {
    return;
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

//----------------------------------------------------------------------------
// Adds an output to the first null position in the output list.
// Expands the list memory if necessary
void vtkSource::AddOutput(vtkDataObject *output)
{
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
}

//----------------------------------------------------------------------------
// Adds an output to the first null position in the output list.
// Expands the list memory if necessary
void vtkSource::RemoveOutput(vtkDataObject *output)
{
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
}

//----------------------------------------------------------------------------
// Set an Output of this filter. 
// tricky because we have to manage the double pointers and keep
// them consistent.
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

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSource::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass and you should really use ExecuteData(vtkDataObject *) instead");
}

//----------------------------------------------------------------------------
vtkDataObject **vtkSource::GetOutputs()
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
  vtkProcessObject::PrintSelf(os,indent);

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

int vtkSource::InRegisterLoop(vtkObject *o)
{
  int idx;
  int num = 0;
  int cnum = 0;
  int match = 0;
  
  for (idx = 0; idx < this->NumberOfOutputs; idx++)
    {
    if (this->Outputs[idx])
      {
      if (this->Outputs[idx] == o)
        {
        match = 1;
        }
      if (this->Outputs[idx]->GetSource() == this)
        {
        num++;
        cnum += this->Outputs[idx]->GetNetReferenceCount();
        }
      }
    }
  
  // if no one outside is using us
  // and our data objects are down to one net reference
  // and we are being asked by one of our data objects
  if (this->ReferenceCount == num && cnum == (num + 1) && match)
    {
    return 1;
    }
  return 0;
}
                           
void vtkSource::UnRegister(vtkObject *o)
{
  int idx;
  int done = 0;

  // detect the circular loop source <-> data
  // If we have two references and one of them is my data
  // and I am not being unregistered by my data, break the loop.
  if (this->ReferenceCount == (this->NumberOfOutputs+1))
    {
    done = 1;
    for (idx = 0; idx < this->NumberOfOutputs; idx++)
      {
      if (this->Outputs[idx])
        {
        if (this->Outputs[idx] == o)
          {
          done = 0;
          }
        if (this->Outputs[idx]->GetNetReferenceCount() != 1)
          {
          done = 0;
          }
        }
      }
    }
  
  if (this->ReferenceCount == this->NumberOfOutputs)
    {
    int match = 0;
    int total = 0;
    for (idx = 0; idx < this->NumberOfOutputs; idx++)
      {
      if (this->Outputs[idx])
        {
        if (this->Outputs[idx] == o)
          {
          match = 1;
          }
        total += this->Outputs[idx]->GetNetReferenceCount();
        }
      }
    if (total == (this->NumberOfOutputs + 1) && match)
      {
      done = 1;
      }
    }
    
  if (done)
    {
    for (idx = 0; idx < this->NumberOfOutputs; idx++)
      {
      if (this->Outputs[idx])
        {
        this->Outputs[idx]->SetSource(NULL);
        }
      }
    }
  
  this->vtkObject::UnRegister(o);
}


void vtkSource::EnlargeOutputUpdateExtents(vtkDataObject *vtkNotUsed(output))
{
  this->LegacyHack = 0;
}

