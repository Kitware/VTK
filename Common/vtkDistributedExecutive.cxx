/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistributedExecutive.h"

#include "vtkObjectFactory.h"
#include "vtkAlgorithm.h"
#include "vtkGarbageCollector.h"

vtkCxxRevisionMacro(vtkDistributedExecutive, "1.3");
vtkStandardNewMacro(vtkDistributedExecutive);
vtkCxxSetObjectMacro(vtkDistributedExecutive, Algorithm, vtkAlgorithm);

//----------------------------------------------------------------------------
vtkDistributedExecutive::vtkDistributedExecutive()
{
  this->Algorithm = 0;
}

//----------------------------------------------------------------------------
vtkDistributedExecutive::~vtkDistributedExecutive()
{
  this->SetAlgorithm(0);
}

//----------------------------------------------------------------------------
void vtkDistributedExecutive::PrintSelf(ostream& os, vtkIndent indent)
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
void vtkDistributedExecutive::AddAlgorithm(vtkAlgorithm* algorithm)
{
  if(vtkAlgorithm* oldAlgorithm = this->GetAlgorithm())
    {
    vtkErrorMacro("Cannot add more than one vtkAlgorithm.  "
                  "Current algorithm is " << oldAlgorithm
                  << ".  Attempting to add algorithm " << algorithm << ".");
    return;
    }
  this->SetAlgorithm(algorithm);
}

//----------------------------------------------------------------------------
void vtkDistributedExecutive::RemoveAlgorithm(vtkAlgorithm* algorithm)
{
  vtkAlgorithm* oldAlgorithm = this->GetAlgorithm();
  if(algorithm == oldAlgorithm)
    {
    this->SetAlgorithm(0);
    }
  else
    {
    vtkErrorMacro("Cannot remove a vtkAlgorithm that has not been added.  "
                  "Current algorithm is " << oldAlgorithm
                  << ".  Attempting to remove algorithm " << algorithm << ".");
    }
}

//----------------------------------------------------------------------------
vtkAlgorithm* vtkDistributedExecutive::GetAlgorithm()
{
  return this->Algorithm;
}

//----------------------------------------------------------------------------
void vtkDistributedExecutive::ReportReferences(vtkGarbageCollector* collector)
{
  // Report reference to our algorithm.
  collector->ReportReference(this->GetAlgorithm());
}

//----------------------------------------------------------------------------
void vtkDistributedExecutive::RemoveReferences()
{
  this->SetAlgorithm(0);
}

//----------------------------------------------------------------------------
int vtkDistributedExecutive::Update(vtkAlgorithm* algorithm)
{
  if(algorithm != this->GetAlgorithm())
    {
    vtkErrorMacro("Request to update algorithm not managed by this "
                  "executive: " << algorithm);
    return 0;
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDistributedExecutive::GetOutputData(int port)
{
  vtkErrorMacro("GetOutputData(int) must be implemented for this executive.");
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDistributedExecutive::GetOutputData(vtkAlgorithm* algorithm,
                                                      int port)
{
  if(algorithm != this->GetAlgorithm())
    {
    vtkErrorMacro("Request for output data from an algorithm not managed "
                  "by this executive: " << algorithm);
    return 0;
    }
  return this->GetOutputData(port);
}

//----------------------------------------------------------------------------
int vtkDistributedExecutive::InputPortIndexInRange(int port,
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
int vtkDistributedExecutive::OutputPortIndexInRange(int port,
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
