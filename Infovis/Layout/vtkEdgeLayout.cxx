/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeLayout.cxx

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

#include "vtkEdgeLayout.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkFloatArray.h"
#include "vtkEdgeLayoutStrategy.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkEdgeLayout);

// ----------------------------------------------------------------------

vtkEdgeLayout::vtkEdgeLayout()
{
  this->LayoutStrategy = 0;
  this->InternalGraph = NULL;

  this->ObserverTag = 0;
  this->EventForwarder = vtkEventForwarderCommand::New();
  this->EventForwarder->SetTarget(this);
}

// ----------------------------------------------------------------------

vtkEdgeLayout::~vtkEdgeLayout()
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

void vtkEdgeLayout::SetLayoutStrategy(vtkEdgeLayoutStrategy *strategy)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except for the call to SetEdge in the middle :)
  if (strategy != this->LayoutStrategy)
    {
    vtkEdgeLayoutStrategy *tmp = this->LayoutStrategy;
    this->LayoutStrategy = strategy;
    if (this->LayoutStrategy != NULL)
      {
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

unsigned long vtkEdgeLayout::GetMTime()
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

int vtkEdgeLayout::RequestData(vtkInformation *vtkNotUsed(request),
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

  if (this->InternalGraph)
    {
    this->InternalGraph->Delete();
    }

  this->InternalGraph = input->NewInstance();
  // The strategy object is going to modify the Points member so
  // we'll replace that with a deep copy.  For everything else a
  // shallow copy is sufficient.
  this->InternalGraph->ShallowCopy(input);

  // Copy the edge layout points.
  this->InternalGraph->DeepCopyEdgePoints(input);

  // Give the layout strategy a pointer to the input.  We set it to
  // NULL first to force the layout algorithm to re-initialize
  // itself.  This is necessary in case the input is the same data
  // object with a newer mtime.
  this->LayoutStrategy->SetGraph(NULL);
  this->LayoutStrategy->SetGraph(this->InternalGraph);

  // No matter whether the input is new or not, the layout strategy
  // needs to do its thing.  It modifies its input
  // (this->InternalGraph) so we can just use that as the output.
  this->LayoutStrategy->Layout();
  output->ShallowCopy(this->InternalGraph);

  return 1;
}

// ----------------------------------------------------------------------

void vtkEdgeLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LayoutStrategy: "
    << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
    {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "InternalGraph: "
    << (this->InternalGraph ? "" : "(none)") << endl;
  if (this->InternalGraph)
    {
    this->InternalGraph->PrintSelf(os, indent.GetNextIndent());
    }
}
