/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayout.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkGraphLayout.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkMath.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDataArray.h>

#include "vtkGraphLayoutStrategy.h"

vtkCxxRevisionMacro(vtkGraphLayout, "1.1");
vtkStandardNewMacro(vtkGraphLayout);

vtkGraphLayout::vtkGraphLayout()
{
  this->LayoutStrategy = 0;
  this->LastInput = NULL;
  this->LastInputMTime = 0;
  this->InternalGraph = NULL;
}

vtkGraphLayout::~vtkGraphLayout()
{
  if (this->LayoutStrategy)
    {
    this->LayoutStrategy->Delete();
    }
  if (this->InternalGraph)
    {
    this->InternalGraph->Delete();
    }
}

void vtkGraphLayout::SetLayoutStrategy(vtkGraphLayoutStrategy *strategy)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except for the call to SetGraph in the middle :)
  if (strategy != this->LayoutStrategy)
    {
    vtkGraphLayoutStrategy *tmp = this->LayoutStrategy;
    this->LayoutStrategy = strategy;
    if (this->LayoutStrategy != NULL)
      {
      this->LayoutStrategy->Register(this);
      if (this->InternalGraph)
        {
        // Set the graph in the layout strategy
        this->LayoutStrategy->SetGraph(this->InternalGraph);
        }
      }
    if (tmp != NULL)
      {
      tmp->UnRegister(this);
      }
    this->Modified();
    }
  
}

unsigned long vtkGraphLayout::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if (this->LayoutStrategy != NULL)
    {
    time = this->LayoutStrategy->GetMTime();
    mTime = (time > mTime ? time : mTime);
    }
  return mTime;
}

int vtkGraphLayout::IsLayoutComplete()
{
  if (this->LayoutStrategy)
    {
    return this->LayoutStrategy->IsLayoutComplete();
    }
    
  // This is an error condition
  vtkErrorMacro("IsLayoutComplete called with layout strategy==NULL");
  return 0;
}


int vtkGraphLayout::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->LayoutStrategy == NULL)
    {
    vtkErrorMacro(<< "Layout strategy must me non-null.");
    return 0;
    }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkAbstractGraph *input = vtkAbstractGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkAbstractGraph *output = vtkAbstractGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
    
  // See if the input is the same as the last input
  // Note: This is keeping state and in vtk filter
  // land this is a bad thing. :)
  if ((input != this->LastInput) ||
      (input->GetMTime() != this->LastInputMTime))
    {
    
    // Okay create an internal graph for supporting
    // iterative graph layout strategies
    if (this->InternalGraph)
      {
      this->InternalGraph->Delete();
      }
    this->InternalGraph = input->NewInstance();
    
    // Capture info about graph
    this->LastInput = input;
    this->LastInputMTime = input->GetMTime();
    
    // Shallow copy data to output.
    this->InternalGraph->ShallowCopy(input);

    // Deep copy the point data from input to output
    vtkPoints* oldPoints = input->GetPoints();
    vtkPoints* newPoints = vtkPoints::New();
    newPoints->DeepCopy(oldPoints);
    this->InternalGraph->SetPoints(newPoints);
    newPoints->Delete();
    
    // Set the graph in the layout strategy
    this->LayoutStrategy->SetGraph(this->InternalGraph);
    }

  this->LayoutStrategy->Layout();
  output->ShallowCopy(this->InternalGraph);

  return 1;
}


void vtkGraphLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LayoutStrategy:" << endl;
  this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "InternalGraph:" << endl;
  this->InternalGraph->PrintSelf(os, indent.GetNextIndent());
}
