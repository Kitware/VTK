/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamGraph.cxx

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

#include "vtkStreamGraph.h"

#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergeGraphs.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkMutableGraphHelper.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <vtkstd/map>

vtkStandardNewMacro(vtkStreamGraph);
//---------------------------------------------------------------------------
vtkStreamGraph::vtkStreamGraph()
{
  this->MaxEdges = -1;
  this->CurrentGraph = vtkMutableGraphHelper::New();
  this->MergeGraphs = vtkMergeGraphs::New();
}

//---------------------------------------------------------------------------
vtkStreamGraph::~vtkStreamGraph()
{
  if (this->CurrentGraph)
    {
    this->CurrentGraph->Delete();
    }
  if (this->MergeGraphs)
    {
    this->MergeGraphs->Delete();
    }
}

//---------------------------------------------------------------------------
int vtkStreamGraph::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkInformation* input_info = inputVector[0]->GetInformationObject(0);
  vtkGraph* input = vtkGraph::SafeDownCast(
    input_info->Get(vtkDataObject::DATA_OBJECT()));

  // Copy structure into output graph.
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkGraph* output = vtkGraph::SafeDownCast(
    outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  double progress = 0.1;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

  // First pass: make a copy of the graph and we're done
  if (!this->CurrentGraph->GetGraph())
    {
    if (vtkDirectedGraph::SafeDownCast(input))
      {
      vtkSmartPointer<vtkMutableDirectedGraph> g = vtkSmartPointer<vtkMutableDirectedGraph>::New();
      this->CurrentGraph->SetGraph(g);
      }
    else
      {
      vtkSmartPointer<vtkMutableUndirectedGraph> g = vtkSmartPointer<vtkMutableUndirectedGraph>::New();
      this->CurrentGraph->SetGraph(g);
      }
    this->CurrentGraph->GetGraph()->DeepCopy(input);
    if (!output->CheckedShallowCopy(input))
      {
      vtkErrorMacro("Output graph format invalid.");
      return 0;
      }
    return 1;
    }

  progress = 0.2;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

  this->MergeGraphs->SetMaxEdges(this->MaxEdges);

  if (!this->MergeGraphs->ExtendGraph(this->CurrentGraph, input))
    {
    return 0;
    }

  progress = 0.9;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

  if (!output->CheckedShallowCopy(this->CurrentGraph->GetGraph()))
    {
    vtkErrorMacro("Output graph format invalid.");
    return 0;
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkStreamGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MaxEdges: " << this->MaxEdges << endl;
}
