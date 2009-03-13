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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkGraphLayout.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkFloatArray.h"
#include "vtkGraphLayoutStrategy.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

vtkCxxRevisionMacro(vtkGraphLayout, "1.10");
vtkStandardNewMacro(vtkGraphLayout);

// ----------------------------------------------------------------------

vtkGraphLayout::vtkGraphLayout()
{
  this->LayoutStrategy = 0;
  this->StrategyChanged=false;
  this->LastInput = NULL;
  this->LastInputMTime = 0;
  this->InternalGraph = NULL;

  this->ObserverTag = 0;
  this->EventForwarder = vtkEventForwarderCommand::New();
  this->EventForwarder->SetTarget(this);
}

// ----------------------------------------------------------------------

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
  this->EventForwarder->Delete();
}

// ----------------------------------------------------------------------

void 
vtkGraphLayout::SetLayoutStrategy(vtkGraphLayoutStrategy *strategy)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except for the call to SetGraph in the middle :)
  if (strategy != this->LayoutStrategy)
    {
    vtkGraphLayoutStrategy *tmp = this->LayoutStrategy;
    this->LayoutStrategy = strategy;
    if (this->LayoutStrategy != NULL)
      {
      this->StrategyChanged = true;
      this->LayoutStrategy->Register(this);
      this->ObserverTag =
        this->LayoutStrategy->AddObserver(vtkCommand::ProgressEvent, 
                                          this->EventForwarder);
      if (this->InternalGraph)
        {
        // Set the graph in the layout strategy
        this->LayoutStrategy->SetGraph(this->InternalGraph);
        }
      }
    if (tmp != NULL)
      {
      tmp->RemoveObserver(this->ObserverTag);
      tmp->UnRegister(this);
      }
    this->Modified();
    }
  
}

// ----------------------------------------------------------------------

unsigned long 
vtkGraphLayout::GetMTime()
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

// ----------------------------------------------------------------------

int 
vtkGraphLayout::IsLayoutComplete()
{
  if (this->LayoutStrategy)
    {
    return this->LayoutStrategy->IsLayoutComplete();
    }
    
  // This is an error condition
  vtkErrorMacro("IsLayoutComplete called with layout strategy==NULL");
  return 0;
}

// ----------------------------------------------------------------------

int 
vtkGraphLayout::RequestData(vtkInformation *vtkNotUsed(request),
                            vtkInformationVector **inputVector,
                            vtkInformationVector *outputVector)
{
  if (this->LayoutStrategy == NULL)
    {
    vtkErrorMacro(<< "Layout strategy must be non-null.");
    return 0;
    }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Is this a completely new input?  Is it the same input as the last
  // time the filter ran but with a new MTime?  If either of those is
  // true, make a copy and give it to the strategy object anew.
  if (this->StrategyChanged ||
      input != this->LastInput ||
      input->GetMTime() > this->LastInputMTime)
    {
    if (this->StrategyChanged)
      {
      vtkDebugMacro(<<"Strategy changed so reading in input again.");
      this->StrategyChanged = false;
      }
    else if (input != this->LastInput)
      {
      vtkDebugMacro(<<"Filter running with different input.  Resetting in strategy.");
      }
    else 
      {
      vtkDebugMacro(<<"Input modified since last run.  Resetting in strategy.");
      }

    if (this->InternalGraph)
      {
      this->InternalGraph->Delete();
      }

    this->InternalGraph = input->NewInstance();
    // The strategy object is going to modify the Points member so
    // we'll replace that with a deep copy.  For everything else a
    // shallow copy is sufficient.
    this->InternalGraph->ShallowCopy(input);
    
    // The copy of the points will be to a float type
    vtkPoints* newPoints = vtkPoints::New(VTK_FLOAT);
    newPoints->DeepCopy(input->GetPoints());
    this->InternalGraph->SetPoints(newPoints);
    newPoints->Delete();
    

    // Save information about the input so that we can detect when
    // it's changed on future runs.  According to the VTK pipeline
    // design, this is a bad thing.  In this case there's no
    // particularly graceful way around it since the pipeline was not
    // designed to support incremental execution.
    this->LastInput = input;
    this->LastInputMTime = input->GetMTime();
    
    // Give the layout strategy a pointer to the input.  We set it to
    // NULL first to force the layout algorithm to re-initialize
    // itself.  This is necessary in case the input is the same data
    // object with a newer mtime.
    this->LayoutStrategy->SetGraph(NULL);
    this->LayoutStrategy->SetGraph(this->InternalGraph);
    } // Done handling a new or changed filter input.

  // No matter whether the input is new or not, the layout strategy
  // needs to do its thing.  It modifies its input
  // (this->InternalGraph) so we can just use that as the output.
  this->LayoutStrategy->Layout();
  output->ShallowCopy(this->InternalGraph);

  return 1;
}

// ----------------------------------------------------------------------


void vtkGraphLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StrategyChanged: " << (this->StrategyChanged ? "True" : "False") << endl;
  os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
    {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "InternalGraph: " << (this->InternalGraph ? "" : "(none)") << endl;
  if (this->InternalGraph)
    {
    this->InternalGraph->PrintSelf(os, indent.GetNextIndent());
    }
}
