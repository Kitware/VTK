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
#include "vtkObjectFactory.h"
#include "vtkSource.h"

vtkCxxRevisionMacro(vtkExecutive, "1.3");
vtkStandardNewMacro(vtkExecutive);
vtkCxxSetObjectMacro(vtkExecutive, Algorithm, vtkAlgorithm);
vtkInformationKeyMacro(vtkExecutive, EXECUTIVE, Executive);
vtkInformationKeyMacro(vtkExecutive, PORT_NUMBER, Integer);

//----------------------------------------------------------------------------
vtkExecutive::vtkExecutive()
{
  this->GarbageCollecting = 0;
  this->Algorithm = 0;
}

//----------------------------------------------------------------------------
vtkExecutive::~vtkExecutive()
{
  this->SetAlgorithm(0);
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
void vtkExecutive::ReportReferences(vtkGarbageCollector* collector)
{
  // Report reference to our algorithm.
  collector->ReportReference(this->GetAlgorithm(), "Algorithm");
}

//----------------------------------------------------------------------------
void vtkExecutive::RemoveReferences()
{
  this->SetAlgorithm(0);
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
vtkInformation* vtkExecutive::GetOutputInformation(int)
{
  vtkErrorMacro("GetOutputInformation(int) must be implemented for this executive.");
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExecutive::GetOutputData(int)
{
  vtkErrorMacro("GetOutputData(int) must be implemented for this executive.");
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExecutive::GetInputData(int,int)
{
  vtkErrorMacro("GetInputData(int,int) must be implemented for this executive.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkExecutive::SetOutputData(int, vtkDataObject*)
{
  vtkErrorMacro("SetOutputData(int, vtkDataObject*) must be implemented for this executive.");
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
