// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkStreamGraph.h"

#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergeGraphs.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableGraphHelper.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <map>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStreamGraph);
//------------------------------------------------------------------------------
vtkStreamGraph::vtkStreamGraph()
{
  this->CurrentGraph = vtkMutableGraphHelper::New();
  this->MergeGraphs = vtkMergeGraphs::New();
  this->UseEdgeWindow = false;
  this->EdgeWindowArrayName = nullptr;
  this->SetEdgeWindowArrayName("time");
  this->EdgeWindow = 10000.0;
}

//------------------------------------------------------------------------------
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
  this->SetEdgeWindowArrayName(nullptr);
}

//------------------------------------------------------------------------------
int vtkStreamGraph::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* input_info = inputVector[0]->GetInformationObject(0);
  vtkGraph* input = vtkGraph::SafeDownCast(input_info->Get(vtkDataObject::DATA_OBJECT()));

  // Copy structure into output graph.
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkGraph* output = vtkGraph::SafeDownCast(outputInfo->Get(vtkDataObject::DATA_OBJECT()));

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
      vtkSmartPointer<vtkMutableUndirectedGraph> g =
        vtkSmartPointer<vtkMutableUndirectedGraph>::New();
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

  this->MergeGraphs->SetUseEdgeWindow(this->UseEdgeWindow);
  this->MergeGraphs->SetEdgeWindowArrayName(this->EdgeWindowArrayName);
  this->MergeGraphs->SetEdgeWindow(this->EdgeWindow);

  if (!this->MergeGraphs->ExtendGraph(this->CurrentGraph, input))
  {
    return 0;
  }

  progress = 0.9;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

  output->DeepCopy(this->CurrentGraph->GetGraph());

  return 1;
}

//------------------------------------------------------------------------------
void vtkStreamGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseEdgeWindow: " << this->UseEdgeWindow << endl;
  os << indent << "EdgeWindowArrayName: "
     << (this->EdgeWindowArrayName ? this->EdgeWindowArrayName : "(none)") << endl;
  os << indent << "EdgeWindow: " << this->EdgeWindow << endl;
}
VTK_ABI_NAMESPACE_END
