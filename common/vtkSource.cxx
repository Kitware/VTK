/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSource.cxx
 Language:  C++
 Date:      $Date$
 Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 

=========================================================================*/
#include "vtkSource.h"
#include "vtkDataObject.h"

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
void vtkSource::Update()
{
  if (this->GetOutput(0))
    {
    this->GetOutput(0)->Update();
    }
}

typedef vtkDataObject *vtkDataObjectPointer;
//----------------------------------------------------------------------------
// Called by constructor to set up output array.
void vtkSource::SetNumberOfOutputs(int num)
{
  int idx;
  vtkDataObjectPointer *outputs;

  // in case nothing has changed.
  if (num == this->NumberOfOutputs)
    {
    return;
    }
  
  // Allocate new arrays.
  outputs = new vtkDataObjectPointer[num];

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
void vtkSource::SetOutput(int idx, vtkDataObject *newOutput)
{
  vtkDataObject *oldOutput;
  
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetOutput: " << idx << ", cannot set output. ");
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
// Update input to this filter and the filter itself.
// This is a streaming version of the update method.
void vtkSource::InternalUpdate(vtkDataObject *output)
{
  int idx;
  int numDivisions, division;

  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }

  // Let the source initialize the data for streaming.
  // output->Initialize and CopyStructure ...
  this->StreamExecuteStart();
  
  // Determine how many pieces we are going to process.
  numDivisions = this->GetNumberOfStreamDivisions();
  for (division = 0; division < numDivisions; ++division)
    {
    // Compute the update extent for all of the inputs.
    if (this->ComputeDivisionExtents(output, division, numDivisions))
      {
      // Update the inputs
      this->Updating = 1;
      for (idx = 0; idx < this->NumberOfInputs; ++idx)
	{
	if (this->Inputs[idx] != NULL)
	  {
	  this->Inputs[idx]->InternalUpdate();
	  }
	}
      this->Updating = 0;
      
      // Execute
      if ( this->StartMethod )
	{
	(*this->StartMethod)(this->StartMethodArg);
	}
      // reset Abort flag
      this->AbortExecute = 0;
      this->Progress = 0.0;
      this->Execute();
      if ( !this->AbortExecute )
	{
	this->UpdateProgress(1.0);
	}
      if ( this->EndMethod )
	{
	(*this->EndMethod)(this->EndMethodArg);
	}
      }
    }
  
  // Let the source clean up after streaming.
  this->StreamExecuteEnd();
  
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
// To facilitate a single Update method, we are putting data initialization
// in this method.
void vtkSource::StreamExecuteStart()
{
  int idx;
  
  // clear output (why isn't this ReleaseData.  Does it allocate data too?)
  // Should it be done if StreamExecuteStart?
  for (idx = 0; idx < this->NumberOfOutputs; idx++)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->Initialize(); 
      }
    }
}



//----------------------------------------------------------------------------
int vtkSource::ComputeDivisionExtents(vtkDataObject *output,
				      int idx, int numDivisions)
{
  // If only one division is requested (filter is no initiating streaming),
  // then call the non-streaming convenience method.
  if (idx == 0 && numDivisions == 1)
    {
    return this->ComputeInputUpdateExtents(output);
    }
  
  
  if (this->NumberOfInputs > 0)
    {
    vtkErrorMacro("Source did not implement ComputeDivisionExtents");
    return 0;
    }
  
  return 1;
}


//----------------------------------------------------------------------------
int vtkSource::ComputeInputUpdateExtents(vtkDataObject *output)
{
  if (this->NumberOfInputs > 0)
    {
    vtkErrorMacro("Subclass did not implement ComputeInputUpdateExtents");
    }
  return 1;
}



//----------------------------------------------------------------------------
void vtkSource::UpdateInformation()
{
  unsigned long t1, t2, size;
  int locality, l2, idx;
  int maxPieces = 1;
  vtkDataObject *pd;
  vtkDataObject *output;

  if (this->Outputs[0] == NULL)
    {
    return;
    }
  
  if (this->Updating)
    {
    // We are in a pipeline loop.
    // Force an update
    this->GetOutput(0)->Modified();
    return;
    }
  
  // Update information on the input and
  // compute information that is general to vtkDataObject.
  t1 = this->GetMTime();
  size = 0;
  locality = 0;

  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      pd = this->Inputs[idx];

      this->Updating = 1;
      pd->UpdateInformation();
      this->Updating = 0;
      
      // for MPI port stuff
      l2 = pd->GetLocality();
      if (l2 > locality)
	{
	locality = l2;
	}
      
      // Pipeline MTime stuff
      t2 = pd->GetPipelineMTime();
      if (t2 > t1)
	{
	t1 = t2;
	}
      // Pipeline MTime does not include the MTime of the data object itself.
      // Factor these mtimes into the next PipelineMTime
      t2 = pd->GetMTime();
      if (t2 > t1)
	{
	t1 = t2;
	}
      
      // Default estimated size is just the sum of the sizes of the inputs.
      size += pd->GetEstimatedWholeMemorySize();
      }
    }

  // for copying information
  if (this->Inputs && this->Inputs[0])
    {
    pd = this->Inputs[0];
    }
  else
    {
    pd = NULL;
    }
  
  // Call ExecuteInformation for subclass specific information.
  // Some sources (readers) have an expensive ExecuteInformation method.
  if (t1 > this->InformationTime.GetMTime())
    {
    // Here is where we set up defaults.
    // I feel funny about setting the PipelineMTime inside the conditional,
    // but it should work.
    for (idx = 0; idx < this->NumberOfOutputs; ++idx)
      {
      output = this->GetOutput(idx);
      if (output)
	{
	output->SetPipelineMTime(t1);
	output->SetLocality(locality + 1);
	output->SetEstimatedWholeMemorySize(size);
	// By default, copy information from first input.
	if (pd)
	  {
	  output->CopyInformation(pd);
	  }
	}  
      }
    
    this->ExecuteInformation();
    // This call to modify is almost useless.  Update invalidates this time.
    // InformationTime is modified at the end of InternalUpdate too.
    // Keep this modified in case we have multiple calls to UpdateInformation.
    this->InformationTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSource::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass");
}

//----------------------------------------------------------------------------
void vtkSource::ExecuteInformation()
{
  //vtkErrorMacro(<< "Subclass did not implement ExecuteInformation");
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







