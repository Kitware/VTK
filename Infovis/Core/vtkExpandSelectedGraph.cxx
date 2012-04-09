/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExpandSelectedGraph.cxx

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

#include "vtkExpandSelectedGraph.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelection.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInEdgeIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include <vtksys/stl/set>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkExpandSelectedGraph);

vtkExpandSelectedGraph::vtkExpandSelectedGraph()
{
  this->SetNumberOfInputPorts(2);
  this->BFSDistance = 1;
  this->IncludeShortestPaths = false;
  this->UseDomain = false;
  this->Domain = 0;
}

vtkExpandSelectedGraph::~vtkExpandSelectedGraph()
{
  this->SetDomain(0);
}

int vtkExpandSelectedGraph::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    return 1;
    }
  return 0;
}

void vtkExpandSelectedGraph::SetGraphConnection(vtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

int vtkExpandSelectedGraph::RequestData(
  vtkInformation* vtkNotUsed(request), 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkSelection* input = vtkSelection::GetData(inputVector[0]);
  vtkGraph* graph = vtkGraph::GetData(inputVector[1]);
  vtkSelection* output = vtkSelection::GetData(outputVector);

  VTK_CREATE(vtkIdTypeArray, indexArray);
  vtkConvertSelection::GetSelectedVertices(input, graph, indexArray);
  this->Expand(indexArray, graph);

  // TODO: Get rid of this HACK. 
  // This guarantees we have unique indices.
  // vtkConvertSelection should "flatten out" the input selection
  // to a single index selection which we then expand, instead of
  // expanding each child selection and merging them which creates
  // duplicates.
  vtksys_stl::set<vtkIdType> indexSet;
  for(int i=0; i<indexArray->GetNumberOfTuples(); ++i)
    {
    indexSet.insert(indexArray->GetValue(i));
    }
  // Delete any entries in the current selection list
  indexArray->Reset();
  // Convert the stl set into the selection list
  vtksys_stl::set<vtkIdType>::iterator I;
  for(I = indexSet.begin(); I != indexSet.end(); ++I)
    {
    indexArray->InsertNextValue(*I);
    }

  // Convert back to a pedigree id selection
  VTK_CREATE(vtkSelection, indexSelection);
  VTK_CREATE(vtkSelectionNode, node);
  indexSelection->AddNode(node);
  node->SetSelectionList(indexArray);
  node->SetFieldType(vtkSelectionNode::VERTEX);
  node->SetContentType(vtkSelectionNode::INDICES);
  VTK_CREATE(vtkSelection, pedigreeIdSelection);
  pedigreeIdSelection.TakeReference(vtkConvertSelection::ToPedigreeIdSelection(indexSelection, graph));
  output->DeepCopy(pedigreeIdSelection);

  return 1;
}

void vtkExpandSelectedGraph::Expand(vtkIdTypeArray *indexArray, vtkGraph *graph)
{
  // Now expand the selection to include neighborhoods around
  // the selected vertices
  int distance = this->BFSDistance;
  while(distance > 0)
    {
    this->BFSExpandSelection(indexArray, graph); 
    --distance;
    }
}

void vtkExpandSelectedGraph::BFSExpandSelection(vtkIdTypeArray *indexArray, 
                                            vtkGraph *graph)
{
  // For each vertex in the selection get its adjacent vertices
  VTK_CREATE(vtkInEdgeIterator, inIt);
  VTK_CREATE(vtkOutEdgeIterator, outIt);

  vtkAbstractArray* domainArr = graph->GetVertexData()->GetAbstractArray("domain");
  vtksys_stl::set<vtkIdType> indexSet;
  for (int i=0; i<indexArray->GetNumberOfTuples(); ++i)
  {  
    // First insert myself
    indexSet.insert(indexArray->GetValue(i));
    
    // Now insert all adjacent vertices
    graph->GetInEdges(indexArray->GetValue(i), inIt);
    while (inIt->HasNext())
      {
      vtkInEdgeType e = inIt->Next();
      if(this->UseDomain && this->Domain && 
        domainArr->GetVariantValue(e.Source).ToString() != this->Domain)
        {
        continue;
        }
      indexSet.insert(e.Source);
      }
    graph->GetOutEdges(indexArray->GetValue(i), outIt);
    while (outIt->HasNext())
      {
      vtkOutEdgeType e = outIt->Next();
      if(this->UseDomain && this->Domain && domainArr &&
        domainArr->GetVariantValue(e.Target).ToString() != this->Domain)
        {
        continue;
        }
      indexSet.insert(e.Target);
      }
  }
  
  // Delete any entries in the current selection list
  indexArray->Reset();
  
  // Convert the stl set into the selection list
  vtksys_stl::set<vtkIdType>::iterator I;
  for(I = indexSet.begin(); I != indexSet.end(); ++I)
    {
    indexArray->InsertNextValue(*I);
    }
}

void vtkExpandSelectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BFSDistance: " << this->BFSDistance << endl;
  os << indent << "IncludeShortestPaths: " 
     << (this->IncludeShortestPaths ? "on" : "off") << endl;
  os << indent << "Domain: " 
     << (this->Domain ? this->Domain : "(null)") << endl;
  os << indent << "UseDomain: " 
     << (this->UseDomain ? "on" : "off") << endl;
}

