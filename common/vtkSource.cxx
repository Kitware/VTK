/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSource.cxx
 Language:  C++
 Date:      $Date$
 Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
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
  int idx;
  vtkDataObject *input;
  vtkDataObject *output;

  // Watch out for loops in the pipeline
  if ( this->Updating )
    {
    // Since we are in a loop, we will want to update. But if
    // we don't modify this filter, then we will not execute
    // because our InformationTime will be more recent than
    // the MTime of our output.
    this->Modified();
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
      
      // What is the PipelineMTime of this input? Compare this against
      // our current computation to find the largest one.
      t2 = input->GetPipelineMTime();

      if (t2 > t1)
        {
        t1 = t2;
        }

      // Pipeline MTime of the input does not include the MTime of the 
      // data object itself. Factor these mtimes into the next PipelineMTime
      t2 = input->GetMTime();
      if (t2 > t1)
        {
        t1 = t2;
        }
      }
    }

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
        }  
      }
    
    this->ExecuteInformation();
    }
}

//----------------------------------------------------------------------------

void vtkSource::PropagateUpdateExtent(vtkDataObject *output)
{
  // check flag to avoid executing forever if there is a loop
  if (this->Updating)
    {
    return;
    }

  // Give the subclass a chance to indicate that it will provide
  // more data then require for the output. This can happen, for
  // example, when a source can only produce the whole output.
  // Although this is being called for a specific output, the source
  // may need to enlarge all outputs.
  this->EnlargeOutputUpdateExtents( output );

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
  for (int idx = 0; idx < this->NumberOfInputs; ++idx)
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

void vtkSource::UpdateData(vtkDataObject *vtkNotUsed(output))
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
    {
    for (idx = 0; idx < this->NumberOfInputs; ++idx)
      {
      if (this->Inputs[idx] != NULL)
	{
	this->Inputs[idx]->PropagateUpdateExtent();
	this->Inputs[idx]->UpdateData();
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
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }

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
    this->Execute();
    }

  // If we ended due to aborting, push the progress up to 1.0 (since
  // it probably didn't end there)
  if ( !this->AbortExecute )
    {
    this->UpdateProgress(1.0);
    }

  // Call the end method, if there is one
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }
    
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

void vtkSource::ComputeEstimatedPipelineMemorySize( vtkDataObject *output,
						    unsigned long size[3] )
{
  unsigned long outputSize[2];
  unsigned long inputPipelineSize[3];
  unsigned long mySize = 0;
  unsigned long maxSize = 0;
  unsigned long goingDownstreamSize = 0;
  unsigned long *inputSize = NULL;
  int idx;

  // We need some space to store the input sizes if there are any inputs
  if ( this->NumberOfInputs > 0 )
    {
    inputSize = new unsigned long[this->NumberOfInputs];
    }

  // Get the pipeline size propagated down each input. Keep track of max
  // pipeline size, how much memory will be required downstream from here,
  // the size of each input, and the memory required by this filter when
  // it executes.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {

      // Get the upstream size of the pipeline, the estimated size of this
      // input, and the maximum size seen upstream from here.
      this->Inputs[idx]->ComputeEstimatedPipelineMemorySize(inputPipelineSize);

      // Save this input size to possibly be used when estimating output size
      inputSize[idx] = inputPipelineSize[1];

      // Is the max returned bigger than the max we've seen so far?
      if ( inputPipelineSize[2] > maxSize )
	{
	maxSize = inputPipelineSize[2];
	}

      // If we are going to release this input, then its size won't matter
      // downstream from here.
      if ( this->Inputs[idx]->ShouldIReleaseData() )
	{
	goingDownstreamSize += inputPipelineSize[0] - inputPipelineSize[1];
	}
      else
	{
	goingDownstreamSize += inputPipelineSize[0];
	}

      // During execution this filter will need all the input data 
      mySize += inputPipelineSize[0];
      }

    // The input was null, so it has no size
    else
      {
      inputSize[idx] = 0;
      }
    }

  // Now the we know the size of all input, compute the output size
  this->ComputeEstimatedOutputMemorySize( output, inputSize, outputSize );

  // This filter will produce all output so it needs all that memory.
  // Also, all this data will flow downstream to the next source (if it is
  // the requested output) or will still exist with no chance of being
  // released (if it is the non-requested output)
  mySize += outputSize[1];
  goingDownstreamSize += outputSize[1];

  // Is the state of the pipeline during this filter's execution the
  // largest that it has been so far?
  if ( mySize > maxSize )
    {
    maxSize = mySize;
    }

  // The first size is the memory going downstream from here - which is all
  // the memory coming in minus any data realeased. The second size is the
  // size of the specified output (which can be used by the downstream 
  // filter when determining how much data it might release). The final size
  // is the maximum pipeline size encountered here and upstream from here.
  size[0] = goingDownstreamSize;
  size[1] = outputSize[0];
  size[2] = maxSize;

  // Delete the space we may have created
  if ( inputSize )
    {
    delete [] inputSize;
    }
}

//----------------------------------------------------------------------------

// This default implementation can be used by any source that will produce
// only structured output. This method should be overridden by anything
// that will produce vtkPolyData or vtkUnstructuredGrid data since the
// output itself cannot estimate its own size.
void vtkSource::ComputeEstimatedOutputMemorySize( vtkDataObject *output,
						  unsigned long *vtkNotUsed(inputSize),
						  unsigned long size[2] )
{
  int idx;
  int tmp;

  size[0] = 0;
  size[1] = 0;

  // loop through all the outputs asking them how big they are given the
  // information that they have on their update extent. Keep track of 
  // the size of the specified output in size[0], and the sum of all
  // output size in size[1]. Ignore input sizes in this default implementation.
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx])
      {
      tmp = this->Outputs[idx]->GetEstimatedMemorySize();
      if ( this->Outputs[idx] == output )
	{
	size[0] = tmp;
	}
      size[1] += tmp;
      }
    }
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
}

//----------------------------------------------------------------------------

void vtkSource::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass");
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
