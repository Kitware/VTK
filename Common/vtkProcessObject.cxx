/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessObject.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkErrorCode.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"
#include "vtkTrivialProducer.h"
#include "vtkInformation.h"

#include "vtkDebugLeaks.h"

vtkCxxRevisionMacro(vtkProcessObject, "1.44");

//----------------------------------------------------------------------------

// Fake data object type used to represent NULL connections for the
// compatibility layer.
class vtkProcessObjectDummyData: public vtkDataObject
{
public:
  vtkTypeMacro(vtkProcessObjectDummyData, vtkDataObject);
  static vtkProcessObjectDummyData* New()
    {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::ConstructClass("vtkProcessObjectDummyData");
#endif
    return new vtkProcessObjectDummyData;
    }
protected:
  vtkProcessObjectDummyData() {}; 
  virtual ~vtkProcessObjectDummyData() {}; 
private:
  vtkProcessObjectDummyData(const vtkProcessObjectDummyData&);
  void operator=(const vtkProcessObjectDummyData&);  // Not implemented.
};

//----------------------------------------------------------------------------
vtkProcessObject::vtkProcessObject()
{
  this->NumberOfInputs = 0;
  this->NumberOfRequiredInputs = 0;
  this->Inputs = NULL;
  this->SortedInputs = NULL;
  this->SortedInputs2 = NULL;
  this->ErrorCode = 0;

  this->SetNumberOfInputPorts(1);
}

// Destructor for the vtkProcessObject class
vtkProcessObject::~vtkProcessObject()
{
  int idx;

  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->RemoveConsumer(this);
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

  if (this->SortedInputs)
    {
    delete [] this->SortedInputs;
    this->SortedInputs = NULL;
    }
  if (this->SortedInputs2)
    {
    delete [] this->SortedInputs2;
    this->SortedInputs2 = NULL;
    }
}

//----------------------------------------------------------------------------
vtkDataObject** vtkProcessObject::GetInputs()
{
  return this->Inputs;
}

//----------------------------------------------------------------------------
int vtkProcessObject::GetNumberOfInputs()
{
  return this->NumberOfInputs;
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_EXECUTIVES
void vtkProcessObject::SetNumberOfInputs(int)
{
  // Input array size management is automatic.  Do nothing.
}
#else
typedef vtkDataObject *vtkDataObjectPointer;
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
  delete [] this->Inputs;
  this->Inputs = NULL;
  this->NumberOfInputs = 0;

  delete [] this->SortedInputs;
  this->SortedInputs = NULL;

  delete [] this->SortedInputs2;
  this->SortedInputs2 = NULL;
  
  // Set the new arrays
  this->Inputs = inputs;
  this->SortedInputs = new vtkDataObjectPointer[num];
  this->SortedInputs2 = new vtkDataObjectPointer[num];
  
  this->NumberOfInputs = num;
  this->Modified();
}
#endif

//----------------------------------------------------------------------------
void vtkProcessObject::AddInput(vtkDataObject* input)
{
#ifdef VTK_USE_EXECUTIVES
  this->AddInputInternal(input);
#else
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
#endif
}

//----------------------------------------------------------------------------
void vtkProcessObject::RemoveInput(vtkDataObject* input)
{
#ifdef VTK_USE_EXECUTIVES
  this->RemoveInputInternal(input);
#else
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
#endif
}

//----------------------------------------------------------------------------
void vtkProcessObject::SqueezeInputArray()
{
#ifdef VTK_USE_EXECUTIVES
  // Array is always squeezed.  Do nothing.
#else
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
#endif
}

//----------------------------------------------------------------------------
void vtkProcessObject::SetNthInput(int idx, vtkDataObject* input)
{
#ifdef VTK_USE_EXECUTIVES
  int num = idx;
  // Check whether anything will change.
  if(num >= 0 && num < this->GetNumberOfInputConnections(0))
    {
    if(this->Inputs[num] == input)
      {
      return;
      }
    }
  else if(num < 0)
    {
    vtkErrorMacro("SetNthInput cannot set input index " << num << ".");
    return;
    }

  if(input && num > this->GetNumberOfInputConnections(0))
    {
    // Avoid creating holes in input array.  Use dummy data to fill in
    // the missing connections.
    for(int i=this->GetNumberOfInputConnections(0); i < num; ++i)
      {
      vtkProcessObjectDummyData* d = vtkProcessObjectDummyData::New();
      this->AddInputInternal(d);
      d->Delete();
      }

    // Now add the real input.
    this->AddInputInternal(input);
    }
  else if(!input && num < this->GetNumberOfInputConnections(0)-1)
    {
    vtkErrorMacro("SetNthInput cannot set input index " << num
                  << " to NULL because there are "
                  << this->GetNumberOfInputConnections(0)
                  << " connections and NULL connections are not allowed.");
    }
  else if(input && num == this->GetNumberOfInputConnections(0))
    {
    this->AddInputInternal(input);
    }
  else if(!input && num == this->GetNumberOfInputConnections(0)-1)
    {
    this->RemoveInputInternal(input);
    }
  else if(input && num < this->GetNumberOfInputConnections(0))
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->SetNthInputConnection(0, num, producerPort);
      }
    else
      {
      // The data object has no producer.  Give it a trivial source.
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(input);
      this->SetNthInputConnection(0, num, producer->GetOutputPort(0));
      producer->Delete();
      }
    }
#else
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
#endif
}

//----------------------------------------------------------------------------
void vtkProcessObject::RemoveAllInputs()
{
#ifdef VTK_USE_EXECUTIVES
  this->SetInputConnection(0, 0);
#else
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
#endif
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

//----------------------------------------------------------------------------
int vtkProcessObject::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkInformation::INPUT_IS_REPEATABLE(), 1);
  if(this->NumberOfRequiredInputs == 0)
    {
    info->Set(vtkInformation::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkProcessObject::FillOutputPortInformation(int, vtkInformation*)
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkProcessObject::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
#ifdef VTK_USE_EXECUTIVES
  for(int i=0; i < this->NumberOfInputs; ++i)
    {
    collector->ReportReference(this->Inputs[i], "Inputs");
    }
#endif
}

//----------------------------------------------------------------------------
void vtkProcessObject::RemoveReferences()
{
#ifdef VTK_USE_EXECUTIVES
  for(int i=0; i < this->NumberOfInputs; ++i)
    {
    if(this->Inputs[i])
      {
      this->Inputs[i]->RemoveConsumer(this);
      this->Inputs[i]->UnRegister(this);
      this->Inputs[i] = 0;
      }
    }
#endif
  this->Superclass::RemoveReferences();
}

//----------------------------------------------------------------------------
void vtkProcessObject::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::SetInputConnection(port, input);
  this->SetupInputs();
}

//----------------------------------------------------------------------------
void vtkProcessObject::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::AddInputConnection(port, input);
  this->SetupInputs();
}

//----------------------------------------------------------------------------
void vtkProcessObject::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::RemoveInputConnection(port, input);
  this->SetupInputs();
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_EXECUTIVES
void vtkProcessObject::AddInputInternal(vtkDataObject* input)
{
  if(input)
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->AddInputConnection(0, producerPort);
      }
    else
      {
      // The data object has no producer.  Give it a trivial source.
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(input);
      this->AddInputConnection(0, producer->GetOutputPort(0));
      producer->Delete();
      }
    }
}
#else
void vtkProcessObject::AddInputInternal(vtkDataObject*)
{
}
#endif

//----------------------------------------------------------------------------
#ifdef VTK_USE_EXECUTIVES
void vtkProcessObject::RemoveInputInternal(vtkDataObject* input)
{
  if(input)
    {
    if(vtkAlgorithmOutput* producerPort = input->GetProducerPort())
      {
      this->RemoveInputConnection(0, producerPort);
      }
    else
      {
      // The data object has no producer.  Search for an input
      // connection with this data object.
      for(int i=0; i < this->GetNumberOfInputConnections(0); ++i)
        {
        vtkAlgorithmOutput* ic = this->GetInputConnection(0, i);
        if(input == ic->GetProducer()->GetOutputDataObject(ic->GetIndex()))
          {
          // Remove the connection.
          this->RemoveInputConnection(0, ic);
          return;
          }
        }
      vtkErrorMacro("Cannot remove input " << input->GetClassName()
                    << "(" << input << ") because it is not a current input.");
      }
    }
}
#else
void vtkProcessObject::RemoveInputInternal(vtkDataObject*)
{
}
#endif

//----------------------------------------------------------------------------
void vtkProcessObject::SetupInputs()
{
#ifdef VTK_USE_EXECUTIVES
  // Construct a new array of input data objects using connections
  // from input port 0.
  typedef vtkDataObject* vtkDataObjectPointer;
  vtkDataObject** newInputs = 0;
  int newNumberOfInputs = this->GetNumberOfInputConnections(0);
  if(newNumberOfInputs > 0)
    {
    newInputs = new vtkDataObjectPointer[newNumberOfInputs];
    int count=0;
    for(int i=0; i < this->GetNumberOfInputConnections(0); ++i)
      {
      vtkAlgorithmOutput* ic = this->GetInputConnection(0, i);
      newInputs[count] = ic->GetProducer()->GetOutputDataObject(ic->GetIndex());
      if(newInputs[count])
        {
        // If the connection has dummy data, set a NULL input.
        if(newInputs[count]->IsA("vtkProcessObjectDummyData"))
          {
          newInputs[count] = 0;
          }
        else
          {
          newInputs[count]->Register(this);
          newInputs[count]->AddConsumer(this);
          }
        ++count;
        }
      }
    newNumberOfInputs = count;
    }

  // Remove the old array of input data objects.
  if(this->NumberOfInputs)
    {
    for(int i=0; i < this->NumberOfInputs; ++i)
      {
      if(this->Inputs[i])
        {
        this->Inputs[i]->RemoveConsumer(this);
        this->Inputs[i]->UnRegister(this);
        }
      }
    delete [] this->Inputs;
    }

  // Save the new array of input data objects.
  this->NumberOfInputs = newNumberOfInputs;
  this->Inputs = newInputs;

  if(this->SortedInputs)
    {
    delete [] this->SortedInputs;
    this->SortedInputs = 0;
    }
  if(this->SortedInputs2)
    {
    delete [] this->SortedInputs2;
    this->SortedInputs2 = 0;
    }
  this->SortedInputs = new vtkDataObjectPointer[this->NumberOfInputs];
  this->SortedInputs2 = new vtkDataObjectPointer[this->NumberOfInputs];
#endif
}

//----------------------------------------------------------------------------
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
  
  os << indent << "ErrorCode: " << vtkErrorCode::GetStringFromErrorCode(this->ErrorCode) << endl;
}
