/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessObject.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessObject.h"
#include "vtkObjectFactory.h"
#include "vtkOldStyleCallbackCommand.h"

vtkCxxRevisionMacro(vtkProcessObject, "1.28");

// Instantiate object with no start, end, or progress methods.
vtkProcessObject::vtkProcessObject()
{
  this->StartTag = 0;
  this->ProgressTag = 0;
  this->EndTag = 0;
  this->AbortExecute = 0;
  this->Progress = 0.0;
  this->ProgressText = NULL;
  this->NumberOfInputs = 0;
  this->NumberOfRequiredInputs = 0;
  this->Inputs = NULL;
  this->SortedInputs = NULL;
  this->SortedInputs2 = NULL;
}

// Destructor for the vtkProcessObject class
vtkProcessObject::~vtkProcessObject()
{
  int idx;
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->UnRegister(this);
      this->Inputs[idx] = NULL;
      this->SortedInputs[idx] = NULL;
      this->SortedInputs2[idx] = NULL;
      }
    }
  if (this->Inputs)
    {
    delete [] this->Inputs;
    this->Inputs = NULL;
    this->NumberOfInputs = 0;
    delete [] this->SortedInputs;
    this->SortedInputs = NULL;
    delete [] this->SortedInputs2;
    this->SortedInputs2 = NULL;
    }
  delete [] this->ProgressText;
  this->ProgressText = NULL;
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
    delete [] this->SortedInputs;
    this->SortedInputs = NULL;
    delete [] this->SortedInputs2;
    this->SortedInputs2 = NULL;
    }
  
  // Set the new arrays
  this->Inputs = inputs;
  this->SortedInputs = new vtkDataObjectPointer[num];
  this->SortedInputs2 = new vtkDataObjectPointer[num];
  
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
    input->AddConsumer(this);
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
  
  this->Inputs[loc]->RemoveConsumer(this);
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
    this->Inputs[idx]->RemoveConsumer(this);
    this->Inputs[idx]->UnRegister(this);
    this->Inputs[idx] = NULL;
    }
  
  if (input)
    {
    input->AddConsumer(this);
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
  this->InvokeEvent(vtkCommand::ProgressEvent,(void *)&amount);
}

// Specify function to be called before object executes.
void vtkProcessObject::SetStartMethod(void (*f)(void *), void *arg)
{
  if ( this->StartTag )
    {
    this->RemoveObserver(this->StartTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->StartTag = this->AddObserver(vtkCommand::StartEvent,cbc);
    cbc->Delete();
    }
}

// Specify function to be called to show progress of filter
void vtkProcessObject::SetProgressMethod(void (*f)(void *), void *arg)
{
  if ( this->ProgressTag )
    {
    this->RemoveObserver(this->ProgressTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->ProgressTag = this->AddObserver(vtkCommand::ProgressEvent,cbc);
    cbc->Delete();
    }
}

// Specify function to be called after object executes.
void vtkProcessObject::SetEndMethod(void (*f)(void *), void *arg)
{
  if ( this->EndTag )
    {
    this->RemoveObserver(this->EndTag);
    }
  
  if ( f )
    {
    vtkOldStyleCallbackCommand *cbc = vtkOldStyleCallbackCommand::New();
    cbc->Callback = f;
    cbc->ClientData = arg;
    this->EndTag = this->AddObserver(vtkCommand::EndEvent,cbc);
    cbc->Delete();
    }
}


// Set the arg delete method. This is used to free user memory.
void vtkProcessObject::SetStartMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->StartTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkProcessObject::SetProgressMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->ProgressTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkProcessObject::SetEndMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->EndTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

void vtkProcessObject::RemoveAllInputs()
{
  if ( this->Inputs )
    {
    for (int idx = 0; idx < this->NumberOfInputs; ++idx)
      {
      if ( this->Inputs[idx] )
        {
        this->Inputs[idx]->UnRegister(this);
        this->Inputs[idx] = NULL;
        }
      }
    delete [] this->Inputs;
    this->Inputs = NULL;
    this->NumberOfInputs = 0;
    this->Modified();
    }
}

void vtkProcessObject::SortInputsByLocality()
{
  int i1, i2;
  int l1, l2;
  // length starts at 1 and doubles every pass.
  int length;
  vtkDataObject **tmp;
  
  // Copy inputs over to sorted array.
  memcpy(this->SortedInputs, this->Inputs, 
         this->NumberOfInputs * sizeof(void*));

  length = 1;
  while (length < this->NumberOfInputs)
    {  
    i1 = 0;
    while (i1 < this->NumberOfInputs)
      {
      l1 = length;
      i2 = i1 + l1;
      if (i2 > this->NumberOfInputs)
        { // Piece one has all the remaining entries.
        l1 = this->NumberOfInputs - i1;
        i2 = this->NumberOfInputs;
        l2 = 0;
        }
      else
        { // l2 is the smaller of the remainder or the current length.
        l2 = this->NumberOfInputs - i2;
        if (l2 > length)
          {
          l2 = length;
          }
        }
      this->SortMerge(this->SortedInputs+i1, l1, 
                      this->SortedInputs+i2, l2,
                      this->SortedInputs2+i1);
      i1 = i2 + l2;
      }
    // swap the two arrays
    tmp = this->SortedInputs;
    this->SortedInputs = this->SortedInputs2;
    this->SortedInputs2 = tmp;
    length *= 2;
    }
}

void vtkProcessObject::SortMerge(vtkDataObject **a1, int l1,
                                 vtkDataObject **a2, int l2,
                                 vtkDataObject **results)
{
  while (l1 > 0 || l2 > 0)
    {
    // When the second list is empty, finish the first.
    if (l2 == 0)
      {
      *results++ = *a1++;
      --l1;
      }
    // When the first list is empty, finish the second.
    else if (l1 == 0 || *a1 == NULL)
      {
      *results++ = *a2++;
      --l2;
      }
    // Handle NULL pointers (put them at the end).
    else if (*a2 == NULL)
      {
      *results++ = *a1++;
      --l1;
      }
    else if (*a1 == NULL)
      {
      *results++ = *a2++;
      --l2;
      }
    // Sort by locality.
    else if ((*a1)->GetLocality() < (*a2)->GetLocality())
      {
      *results++ = *a1++;
      --l1;
      }
    else
      {
      *results++ = *a2++;
      --l2;
      }
    }
}


void vtkProcessObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Required Inputs: "
     << this->NumberOfRequiredInputs << endl;

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

  os << indent << "AbortExecute: " << (this->AbortExecute ? "On\n" : "Off\n");
  os << indent << "Progress: " << this->Progress << "\n";
  if ( this->ProgressText )
    {
    os << indent << "Progress Text: " << this->ProgressText << "\n";
    }
  else
    {
    os << indent << "Progress Text: (None)\n";
    }

  
}
