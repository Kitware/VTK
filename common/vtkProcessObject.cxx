/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkProcessObject.cxx
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
#include "vtkProcessObject.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkProcessObject* vtkProcessObject::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProcessObject");
  if(ret)
    {
    return (vtkProcessObject*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProcessObject;
}




// Instantiate object with no start, end, or progress methods.
vtkProcessObject::vtkProcessObject()
{
  this->StartMethod = NULL;
  this->StartMethodArgDelete = NULL;
  this->StartMethodArg = NULL;
  this->ProgressMethod = NULL;
  this->ProgressMethodArgDelete = NULL;
  this->ProgressMethodArg = NULL;
  this->EndMethod = NULL;
  this->EndMethodArgDelete = NULL;
  this->EndMethodArg = NULL;
  this->AbortExecute = 0;
  this->Progress = 0.0;
  this->NumberOfInputs = 0;
  this->NumberOfRequiredInputs = 0;
  this->Inputs = NULL;
}

// Destructor for the vtkProcessObject class
vtkProcessObject::~vtkProcessObject()
{
  if ((this->StartMethodArg)&&(this->StartMethodArgDelete))
    {
    (*this->StartMethodArgDelete)(this->StartMethodArg);
    }
  if ((this->ProgressMethodArg)&&(this->ProgressMethodArgDelete))
    {
    (*this->ProgressMethodArgDelete)(this->ProgressMethodArg);
    }
  if ((this->EndMethodArg)&&(this->EndMethodArgDelete))
    {
    (*this->EndMethodArgDelete)(this->EndMethodArg);
    }

  int idx;
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->UnRegister(this);
      this->Inputs[idx] = NULL;
      }
    }
  if (this->Inputs)
    {
    delete [] this->Inputs;
    this->Inputs = NULL;
    this->NumberOfInputs = 0;
    }
}

typedef vtkDataObject *vtkDataObjectPointer;
//----------------------------------------------------------------------------
// Called by constructor to set up input array.
void vtkProcessObject::SetNumberOfInputs(int num)
{
  int idx;
  vtkDataObjectPointer *inputs;

  // in case nothing has changed.
  if (num == this->NumberOfInputs)
    {
    return;
    }
  
  // Allocate new arrays.
  inputs = new vtkDataObjectPointer[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    inputs[idx] = NULL;
    }

  // Copy old inputs
  for (idx = 0; idx < num && idx < this->NumberOfInputs; ++idx)
    {
    inputs[idx] = this->Inputs[idx];
    }
  
  // delete the previous arrays
  if (this->Inputs)
    {
    delete [] this->Inputs;
    this->Inputs = NULL;
    this->NumberOfInputs = 0;
    }
  
  // Set the new arrays
  this->Inputs = inputs;
  
  this->NumberOfInputs = num;
  this->Modified();
}

//----------------------------------------------------------------------------
// Adds an input to the first null position in the input list.
// Expands the list memory if necessary
void vtkProcessObject::AddInput(vtkDataObject *input)
{
  int idx;
  
  if (input)
    {
    input->Register(this);
    }
  this->Modified();
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] == NULL)
      {
      this->Inputs[idx] = input;
      return;
      }
    }
  
  this->SetNumberOfInputs(this->NumberOfInputs + 1);
  this->Inputs[this->NumberOfInputs - 1] = input;
}

//----------------------------------------------------------------------------
// Adds an input to the first null position in the input list.
// Expands the list memory if necessary
void vtkProcessObject::RemoveInput(vtkDataObject *input)
{
  int idx, loc;
  
  if (!input)
    {
    return;
    }
  
  // find the input in the list of inputs
  loc = -1;
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] == input)
      {
      loc = idx;
      }
    }
  if (loc == -1)
    {
    vtkDebugMacro("tried to remove an input that was not in the list");
    return;
    }
  
  this->Inputs[loc]->UnRegister(this);
  this->Inputs[loc] = NULL;

  // if that was the last input, then shrink the list
  if (loc == this->NumberOfInputs - 1)
    {
    this->SetNumberOfInputs(this->NumberOfInputs - 1);
    }
  
  this->Modified();
}

//----------------------------------------------------------------------------
// Adds an input to the first null position in the input list.
// Expands the list memory if necessary
void vtkProcessObject::SqueezeInputArray()
{
  int idx, loc;
  
  // move NULL entries to the end
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] == NULL)
      {
      for (loc = idx+1; loc < this->NumberOfInputs; loc++)
        {
        this->Inputs[loc-1] = this->Inputs[loc];
        }
      this->Inputs[this->NumberOfInputs -1] = NULL;
      }
    }

  // adjust the size of the array
  loc = -1;
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (loc == -1 && this->Inputs[idx] == NULL)
      {
      loc = idx;
      }
    }
  if (loc > 0)
    {
    this->SetNumberOfInputs(loc);
    }
}

//----------------------------------------------------------------------------
// Set an Input of this filter. 
void vtkProcessObject::SetNthInput(int idx, vtkDataObject *input)
{
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthInput: " << idx << ", cannot set input. ");
    return;
    }
  // Expand array if necessary.
  if (idx >= this->NumberOfInputs)
    {
    this->SetNumberOfInputs(idx + 1);
    }
  
  // does this change anything?
  if (input == this->Inputs[idx])
    {
    return;
    }
  
  if (this->Inputs[idx])
    {
    this->Inputs[idx]->UnRegister(this);
    this->Inputs[idx] = NULL;
    }
  
  if (input)
    {
    input->Register(this);
    }

  this->Inputs[idx] = input;
  this->Modified();
}

// Update the progress of the process object. If a ProgressMethod exists, 
// executes it. Then set the Progress ivar to amount. The parameter amount 
// should range between (0,1).
void vtkProcessObject::UpdateProgress(float amount)
{
  this->Progress = amount;
  if ( this->ProgressMethod )
    {
    (*this->ProgressMethod)(this->ProgressMethodArg);
    }
}

// Specify function to be called before object executes.
void vtkProcessObject::SetStartMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartMethod || arg != this->StartMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->StartMethodArg)&&(this->StartMethodArgDelete))
      {
      (*this->StartMethodArgDelete)(this->StartMethodArg);
      }
    this->StartMethod = f;
    this->StartMethodArg = arg;
    this->Modified();
    }
}

// Specify function to be called to show progress of filter
void vtkProcessObject::SetProgressMethod(void (*f)(void *), void *arg)
{
  if ( f != this->ProgressMethod || arg != this->ProgressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ProgressMethodArg)&&(this->ProgressMethodArgDelete))
      {
      (*this->ProgressMethodArgDelete)(this->ProgressMethodArg);
      }
    this->ProgressMethod = f;
    this->ProgressMethodArg = arg;
    this->Modified();
    }
}

// Specify function to be called after object executes.
void vtkProcessObject::SetEndMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndMethod || arg != this->EndMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EndMethodArg)&&(this->EndMethodArgDelete))
      {
      (*this->EndMethodArgDelete)(this->EndMethodArg);
      }
    this->EndMethod = f;
    this->EndMethodArg = arg;
    this->Modified();
    }
}


// Set the arg delete method. This is used to free user memory.
void vtkProcessObject::SetStartMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartMethodArgDelete)
    {
    this->StartMethodArgDelete = f;
    this->Modified();
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkProcessObject::SetProgressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ProgressMethodArgDelete)
    {
    this->ProgressMethodArgDelete = f;
    this->Modified();
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkProcessObject::SetEndMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndMethodArgDelete)
    {
    this->EndMethodArgDelete = f;
    this->Modified();
    }
}

void vtkProcessObject::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Required Inputs: "
     << this->NumberOfRequiredInputs << vtkEndl;

  if ( this->NumberOfInputs)
    {
    int idx;
    for (idx = 0; idx < this->NumberOfInputs; ++idx)
      {
      os << indent << "Input " << idx << ": (" << this->Inputs[idx] << ")\n";
      }
    }
  else
    {
    os << indent <<"No Inputs\n";
    }

  if ( this->StartMethod )
    {
    os << indent << "Start Method defined\n";
    }
  else
    {
    os << indent <<"No Start Method\n";
    }

  if ( this->ProgressMethod )
    {
    os << indent << "Progress Method defined\n";
    }
  else
    {
    os << indent << "No Progress Method\n";
    }

  if ( this->EndMethod )
    {
    os << indent << "End Method defined\n";
    }
  else
    {
    os << indent << "No End Method\n";
    }

  os << indent << "AbortExecute: " << (this->AbortExecute ? "On\n" : "Off\n");
  os << indent << "Progress: " << this->Progress << "\n";
}



