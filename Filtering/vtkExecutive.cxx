/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExecutive.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationExecutiveKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSource.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkExecutive, "1.6");
vtkInformationKeyMacro(vtkExecutive, EXECUTIVE, Executive);
vtkInformationKeyMacro(vtkExecutive, PORT_NUMBER, Integer);

//----------------------------------------------------------------------------
class vtkExecutiveInternals
{
public:
  vtkSmartPointer<vtkInformation> RequestInformation;
  vtkSmartPointer<vtkInformationVector> OutputInformation;
  vtkstd::vector< vtkSmartPointer<vtkInformationVector> > InputInformation;
  vtkstd::vector<vtkInformationVector*> InputInformationArray;

  vtkExecutiveInternals()
    {
    this->OutputInformation = vtkSmartPointer<vtkInformationVector>::New();
    }

  vtkInformationVector** GetInputInformation(int newNumberOfPorts);
};

//----------------------------------------------------------------------------
vtkExecutive::vtkExecutive()
{
  this->ExecutiveInternal = new vtkExecutiveInternals;
  this->GarbageCollecting = 0;
  this->Algorithm = 0;
}

//----------------------------------------------------------------------------
vtkExecutive::~vtkExecutive()
{
  this->SetAlgorithm(0);
  delete this->ExecutiveInternal;
}

//----------------------------------------------------------------------------
void vtkExecutive::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if(this->Algorithm)
    {
    os << indent << "Algorithm: " << this->Algorithm << "\n";
    }
  else
    {
    os << indent << "Algorithm: (none)\n";
    }
}

//----------------------------------------------------------------------------
void vtkExecutive::UnRegister(vtkObjectBase* o)
{
  int check = (this->GetReferenceCount() > 1);
  this->Superclass::UnRegister(o);
  if(check && !this->GarbageCollecting)
    {
    vtkGarbageCollector::Check(this);
    }
}

//----------------------------------------------------------------------------
void vtkExecutive::GarbageCollectionStarting()
{
  this->GarbageCollecting = 1;
  this->Superclass::GarbageCollectionStarting();
}

//----------------------------------------------------------------------------
void vtkExecutive::SetAlgorithm(vtkAlgorithm* newAlgorithm)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting Algorithm to " << newAlgorithm);
  vtkAlgorithm* oldAlgorithm = this->Algorithm;
  if(oldAlgorithm != newAlgorithm)
    {
    this->Algorithm = newAlgorithm;
    if(newAlgorithm)
      {
      newAlgorithm->Register(this);
      }
    if(oldAlgorithm)
      {
      oldAlgorithm->UnRegister(this);
      }

    // Update the connected pipeline information objects.
    this->UpdateInputInformationVector();

    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkAlgorithm* vtkExecutive::GetAlgorithm()
{
  return this->Algorithm;
}

//----------------------------------------------------------------------------
vtkInformation* vtkExecutive::GetRequestInformation()
{
  if(!this->ExecutiveInternal->RequestInformation)
    {
    this->ExecutiveInternal->RequestInformation =
      vtkSmartPointer<vtkInformation>::New();
    }
  return this->ExecutiveInternal->RequestInformation.GetPointer();
}

//----------------------------------------------------------------------------
vtkInformationVector**
vtkExecutiveInternals::GetInputInformation(int newNumberOfPorts)
{
  int oldNumberOfPorts = int(this->InputInformation.size());
  if(oldNumberOfPorts != newNumberOfPorts)
    {
    this->InputInformation.resize(newNumberOfPorts);
    this->InputInformationArray.resize(newNumberOfPorts);
    for(int i=oldNumberOfPorts; i < newNumberOfPorts; ++i)
      {
      this->InputInformation[i] = vtkSmartPointer<vtkInformationVector>::New();
      this->InputInformationArray[i] = this->InputInformation[i];
      }
    }
  if(newNumberOfPorts > 0)
    {
    return &this->InputInformationArray[0];
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
vtkInformationVector** vtkExecutive::GetInputInformation()
{
  if(this->Algorithm)
    {
    int numPorts = this->Algorithm->GetNumberOfInputPorts();
    return this->ExecutiveInternal->GetInputInformation(numPorts);
    }
  else
    {
    return this->ExecutiveInternal->GetInputInformation(0);
    }
}

//----------------------------------------------------------------------------
void vtkExecutive::UpdateInputInformationVector()
{
  if(this->Algorithm)
    {
    // Store all the input connection information objects.
    int numPorts = this->Algorithm->GetNumberOfInputPorts();
    vtkInformationVector** inVectorArray =
      this->ExecutiveInternal->GetInputInformation(numPorts);
    for(int i=0; i < this->Algorithm->GetNumberOfInputPorts(); ++i)
      {
      vtkInformationVector* inVector = inVectorArray[i];
      int numConnections = this->Algorithm->GetNumberOfInputConnections(i);
      inVector->SetNumberOfInformationObjects(numConnections);
      for(int j=0; j < numConnections; ++j)
        {
        vtkAlgorithmOutput* input = this->Algorithm->GetInputConnection(i, j);
        vtkExecutive* inExecutive = this->GetInputExecutive(i, j);
        vtkInformation* info =
          inExecutive->GetOutputInformation(input->GetIndex());
        inVector->SetInformationObject(j, info);
        }
      }
    }
  else
    {
    this->ExecutiveInternal->GetInputInformation(0);
    }
}

//----------------------------------------------------------------------------
vtkInformation* vtkExecutive::GetInputInformation(int port, int connection)
{
  if(!this->InputPortIndexInRange(port, "get connected input information from"))
    {
    return 0;
    }
  vtkInformationVector* inVector = this->GetInputInformation()[port];
  return inVector->GetInformationObject(connection);
}

//----------------------------------------------------------------------------
vtkInformationVector* vtkExecutive::GetOutputInformation()
{
  if(!this->ExecutiveInternal->OutputInformation)
    {
    this->ExecutiveInternal->OutputInformation =
      vtkSmartPointer<vtkInformationVector>::New();
    }

  // Set the length of the vector to match the number of ports.
  int oldNumberOfPorts =
    this->ExecutiveInternal->OutputInformation
    ->GetNumberOfInformationObjects();
  this->ExecutiveInternal->OutputInformation
    ->SetNumberOfInformationObjects(this->Algorithm->GetNumberOfOutputPorts());

  // For any new information obects, fill in default information values.
  for (int i = oldNumberOfPorts;
       i < this->Algorithm->GetNumberOfOutputPorts();++i)
    {
    this->FillDefaultOutputInformation(i,
      this->ExecutiveInternal->OutputInformation->GetInformationObject(i));
    }

  return this->ExecutiveInternal->OutputInformation;
}

//----------------------------------------------------------------------------
vtkInformation* vtkExecutive::GetOutputInformation(int port)
{
  return this->GetOutputInformation()->GetInformationObject(port);
}

//----------------------------------------------------------------------------
vtkExecutive* vtkExecutive::GetInputExecutive(int port, int index)
{
  if(!this->InputPortIndexInRange(port, "get the executive for a connection on"))
    {
    return 0;
    }
  if(index < 0 || index >= this->Algorithm->GetNumberOfInputConnections(port))
    {
    vtkErrorMacro("Attempt to get executive for connection index " << index
                  << " on input port " << port << " of algorithm "
                  << this->Algorithm->GetClassName() << "(" << this->Algorithm
                  << "), which has "
                  << this->Algorithm->GetNumberOfInputConnections(port)
                  << " connections.");
    return 0;
    }
  if(vtkAlgorithmOutput* input =
     this->Algorithm->GetInputConnection(port, index))
    {
    return input->GetProducer()->GetExecutive();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkExecutive::ReportReferences(vtkGarbageCollector* collector)
{
  // Report reference to our algorithm.
  collector->ReportReference(this->GetAlgorithm(), "Algorithm");
  for(int i=0; i < int(this->ExecutiveInternal->InputInformation.size()); ++i)
    {
    collector->ReportReference(this->ExecutiveInternal->InputInformation[i],
                               "Input Information Vector");
    }
  collector->ReportReference(this->ExecutiveInternal->OutputInformation,
                             "Output Information Vector");
}

//----------------------------------------------------------------------------
void vtkExecutive::RemoveReferences()
{
  this->SetAlgorithm(0);
  this->ExecutiveInternal->OutputInformation = 0;
}

//----------------------------------------------------------------------------
int vtkExecutive::Update()
{
  return this->Update(0);
}

//----------------------------------------------------------------------------
int vtkExecutive::Update(int)
{
  vtkErrorMacro("This class does not implement Update.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkExecutive::InputPortIndexInRange(int port, const char* action)
{
  // Make sure the algorithm is set.
  if(!this->Algorithm)
    {
    vtkErrorMacro("Attempt to " << (action?action:"access") <<
                  " input port index " << port << " with no algorithm set.");
    return 0;
    }

  // Make sure the index of the input port is in range.
  if(port < 0 || port >= this->Algorithm->GetNumberOfInputPorts())
    {
    vtkErrorMacro("Attempt to " << (action?action:"access")
                  << " input port index " << port << " for algorithm "
                  << this->Algorithm->GetClassName()
                  << "(" << this->Algorithm << "), which has "
                  << this->Algorithm->GetNumberOfInputPorts()
                  << " input ports.");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExecutive::OutputPortIndexInRange(int port, const char* action)
{
  // Make sure the algorithm is set.
  if(!this->Algorithm)
    {
    vtkErrorMacro("Attempt to " << (action?action:"access") <<
                  " output port index " << port << " with no algorithm set.");
    return 0;
    }

  // Make sure the index of the output port is in range.
  if(port < 0 || port >= this->Algorithm->GetNumberOfOutputPorts())
    {
    vtkErrorMacro("Attempt to " << (action?action:"access")
                  << " output port index " << port << " for algorithm "
                  << this->Algorithm->GetClassName()
                  << "(" << this->Algorithm << "), which has "
                  << this->Algorithm->GetNumberOfOutputPorts()
                  << " output ports.");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkExecutive::GetProducerPort(vtkDataObject* d)
{
  if(this->Algorithm && d)
    {
    vtkInformation* info = d->GetPipelineInformation();
    vtkExecutive* dExecutive = info->Get(vtkExecutive::EXECUTIVE());
    int port = info->Get(vtkExecutive::PORT_NUMBER());
    if(dExecutive == this)
      {
      return this->Algorithm->GetOutputPort(port);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExecutive::GetOutputData(int port)
{
  if(!this->OutputPortIndexInRange(port, "get data for"))
    {
    return 0;
    }

  // Bring the data object up to date.
  this->UpdateDataObject();

  // Return the data object.
  if(vtkInformation* info = this->GetOutputInformation(port))
    {
    return info->Get(vtkDataObject::DATA_OBJECT());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkExecutive::SetOutputData(int newPort, vtkDataObject* newOutput)
{
  if(vtkInformation* info = this->GetOutputInformation(newPort))
    {
    if(!newOutput || newOutput->GetPipelineInformation() != info)
      {
      if(newOutput)
        {
        newOutput->SetPipelineInformation(info);
        }
      else if(vtkDataObject* oldOutput = info->Get(vtkDataObject::DATA_OBJECT()))
        {
        oldOutput->SetPipelineInformation(0);
        }

      // Output has changed.  Reset the pipeline information.
      this->ResetPipelineInformation(newPort, info);
      }
    }
  else
    {
    vtkErrorMacro("Could not set output on port " << newPort << ".");
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExecutive::GetInputData(int port, int index)
{
  if(vtkExecutive* e = this->GetInputExecutive(port, index))
    {
    vtkAlgorithmOutput* input =
      this->Algorithm->GetInputConnection(port, index);
    return e->GetOutputData(input->GetIndex());
    }
  else
    {
    return 0;
    }
}
