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

vtkCxxRevisionMacro(vtkExecutive, "1.4");
vtkCxxSetObjectMacro(vtkExecutive, Algorithm, vtkAlgorithm);
vtkInformationKeyMacro(vtkExecutive, EXECUTIVE, Executive);
vtkInformationKeyMacro(vtkExecutive, PORT_NUMBER, Integer);

//----------------------------------------------------------------------------
class vtkExecutiveInternals
{
public:
  vtkSmartPointer<vtkInformationVector> OutputInformation;
  vtkSmartPointer<vtkInformationVector> InputInformation;
  vtkSmartPointer<vtkInformation> RequestInformation;

  vtkExecutiveInternals()
    {
    this->OutputInformation = vtkSmartPointer<vtkInformationVector>::New();
    this->InputInformation = vtkSmartPointer<vtkInformationVector>::New();
    }
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
vtkInformationVector* vtkExecutive::GetInputInformation()
{
  if(!this->ExecutiveInternal->InputInformation)
    {
    this->ExecutiveInternal->InputInformation =
      vtkSmartPointer<vtkInformationVector>::New();
    }
  this->ExecutiveInternal->InputInformation
    ->SetNumberOfInformationObjects(this->Algorithm->GetNumberOfInputPorts());
  return this->ExecutiveInternal->InputInformation.GetPointer();
}

//----------------------------------------------------------------------------
vtkInformation* vtkExecutive::GetInputInformation(int port)
{
  return this->GetInputInformation()->GetInformationObject(port);
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
void vtkExecutive::ReportReferences(vtkGarbageCollector* collector)
{
  // Report reference to our algorithm.
  collector->ReportReference(this->GetAlgorithm(), "Algorithm");
  collector->ReportReference(this->ExecutiveInternal->InputInformation,
                             "Input Information Vector");
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
int vtkExecutive::InputPortIndexInRange(int port,
                                                   const char* action)
{
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
int vtkExecutive::OutputPortIndexInRange(int port,
                                                    const char* action)
{
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
