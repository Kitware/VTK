/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraph.h"

#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"
#include "vtkGraphIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNodeLinks.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <vtkstd/algorithm>

// 
// Standard functions
//

vtkCxxRevisionMacro(vtkGraph, "1.3");
vtkStandardNewMacro(vtkGraph);

//----------------------------------------------------------------------------

void vtkGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Arcs: " << endl;
  this->Arcs->PrintSelf(os, indent.GetNextIndent());
  os << indent << "NodeLinks: " << endl;
  this->NodeLinks->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Directed: " << (this->Directed ? "yes" : "no") << endl;
}

//----------------------------------------------------------------------------

vtkGraph::vtkGraph()
{
  this->Directed = 0;
  this->Arcs = vtkIdTypeArray::New();
  this->Arcs->SetNumberOfComponents(2);
  this->NodeLinks = vtkNodeLinks::New();
}

//----------------------------------------------------------------------------

void vtkGraph::Initialize()
{
  this->Superclass::Initialize();
  this->Directed = 0;
  this->Arcs->Delete();
  this->Arcs = vtkIdTypeArray::New();
  this->Arcs->SetNumberOfComponents(2);
  this->NodeLinks->Delete();
  this->NodeLinks = vtkNodeLinks::New();
}

//----------------------------------------------------------------------------

vtkGraph::~vtkGraph()
{
  if (this->Arcs)
    {
    this->Arcs->Delete();
    this->Arcs = NULL;
    }
  if (this->NodeLinks)
    {
    this->NodeLinks->Delete();
    this->NodeLinks = NULL;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetNumberOfArcs()
{
  return this->Arcs->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetNumberOfNodes()
{
  return this->NodeLinks->GetNumberOfNodes();
}

//----------------------------------------------------------------------------
void vtkGraph::GetAdjacentNodes(vtkIdType node, vtkGraphIdList* nodeIds)
{
  nodeIds->Reset();
  vtkIdType narcs;
  const vtkIdType* arcs;
  this->NodeLinks->GetAdjacent(node, narcs, arcs);
  for (vtkIdType i = 0; i < narcs; i++)
    {
    nodeIds->InsertNextId(this->GetOppositeNode(arcs[i], node));
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetInNodes(vtkIdType node, vtkGraphIdList* nodeIds)
{
  if (!this->Directed)
    {
    this->GetAdjacentNodes(node, nodeIds);
    return;
    }
  nodeIds->Reset();
  vtkIdType narcs;
  const vtkIdType* arcs;
  this->NodeLinks->GetInAdjacent(node, narcs, arcs);
  for (vtkIdType i = 0; i < narcs; i++)
    {
    nodeIds->InsertNextId(this->GetOppositeNode(arcs[i], node));
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetOutNodes(vtkIdType node, vtkGraphIdList* nodeIds)
{
  if (!this->Directed)
    {
    this->GetAdjacentNodes(node, nodeIds);
    return;
    }
  nodeIds->Reset();
  vtkIdType narcs;
  const vtkIdType* arcs;
  this->NodeLinks->GetOutAdjacent(node, narcs, arcs);
  for (vtkIdType i = 0; i < narcs; i++)
    {
    nodeIds->InsertNextId(this->GetOppositeNode(arcs[i], node));
    }
}

//----------------------------------------------------------------------------
void vtkGraph::GetIncidentArcs(vtkIdType node, vtkGraphIdList* arcIds)
{
  vtkIdType narcs;
  const vtkIdType* arcs;
  this->NodeLinks->GetAdjacent(node, narcs, arcs);
  arcIds->SetArray(const_cast<vtkIdType*>(arcs), narcs, true);
}

//----------------------------------------------------------------------------
void vtkGraph::GetIncidentArcs(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs)
{
  this->NodeLinks->GetAdjacent(node, narcs, arcs);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetDegree(vtkIdType node)
{
  return this->NodeLinks->GetDegree(node);
}

//----------------------------------------------------------------------------
void vtkGraph::GetInArcs(vtkIdType node, vtkGraphIdList* arcIds)
{
  if (!this->Directed)
    {
    this->GetIncidentArcs(node, arcIds);
    return;
    }
  vtkIdType narcs;
  const vtkIdType* arcs;
  this->NodeLinks->GetInAdjacent(node, narcs, arcs);
  arcIds->SetArray(const_cast<vtkIdType*>(arcs), narcs, true);
}

//----------------------------------------------------------------------------
void vtkGraph::GetInArcs(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs)
{
  if (!this->Directed)
    {
    this->GetIncidentArcs(node, narcs, arcs);
    return;
    }
  this->NodeLinks->GetInAdjacent(node, narcs, arcs);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetInDegree(vtkIdType node)
{
  if (!this->Directed)
    {
    return this->GetDegree(node);
    }
  return this->NodeLinks->GetInDegree(node);
}

//----------------------------------------------------------------------------
void vtkGraph::GetOutArcs(vtkIdType node, vtkGraphIdList* arcIds)
{
  if (!this->Directed)
    {
    this->GetIncidentArcs(node, arcIds);
    return;
    }
  vtkIdType narcs;
  const vtkIdType* arcs;
  this->NodeLinks->GetOutAdjacent(node, narcs, arcs);
  arcIds->SetArray(const_cast<vtkIdType*>(arcs), narcs, true);
}

//----------------------------------------------------------------------------
void vtkGraph::GetOutArcs(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs)
{
  if (!this->Directed)
    {
    this->GetIncidentArcs(node, narcs, arcs);
    return;
    }
  this->NodeLinks->GetOutAdjacent(node, narcs, arcs);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetOutDegree(vtkIdType node)
{
  if (!this->Directed)
    {
    return this->GetDegree(node);
    }
  return this->NodeLinks->GetOutDegree(node);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetSourceNode(vtkIdType arc)
{
  return this->Arcs->GetValue(2*arc);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetTargetNode(vtkIdType arc)
{
  return this->Arcs->GetValue(2*arc + 1);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::GetOppositeNode(vtkIdType arc, vtkIdType node)
{
  if (this->GetSourceNode(arc) != node)
    {
    return this->GetSourceNode(arc);
    }
  return this->GetTargetNode(arc);
}

//----------------------------------------------------------------------------
void vtkGraph::SetNumberOfNodes(vtkIdType nodes)
{
  if (nodes >= this->GetNumberOfNodes())
    {
    for (vtkIdType i = this->GetNumberOfNodes(); i < nodes; i++)
      {
      this->AddNode();
      }
    }
  else
    {
    for (vtkIdType i = this->GetNumberOfNodes() - 1; i >= nodes; i--)
      {
      this->RemoveNode(i);
      }
    }
}

//----------------------------------------------------------------------------
void vtkGraph::ShallowCopy(vtkDataObject *dataObject)
{
  vtkGraph* graph = vtkGraph::SafeDownCast(dataObject);

  if ( graph != NULL )
    {
    if (this->Arcs)
      {
      this->Arcs->Delete();
      }
    this->Arcs = graph->Arcs;
    if (this->Arcs)
      {
      this->Arcs->Register(this);
      }

    if (this->NodeLinks)
      {
      this->NodeLinks->Delete();
      }
    this->NodeLinks = graph->NodeLinks;
    if (this->NodeLinks)
      {
      this->NodeLinks->Register(this);
      }

    this->Directed = graph->Directed;

    }

  // Do superclass
  this->Superclass::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkGraph::DeepCopy(vtkDataObject *dataObject)
{
  vtkGraph* graph = vtkGraph::SafeDownCast(dataObject);

  if ( graph != NULL )
    {
    this->Arcs->DeepCopy(graph->Arcs);
    this->NodeLinks->DeepCopy(graph->NodeLinks);
    this->Directed = graph->Directed;
    }

  // Do superclass
  this->Superclass::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::AddNode()
{
  return this->NodeLinks->AddNode();
}

//----------------------------------------------------------------------------
vtkIdType vtkGraph::AddArc(vtkIdType source, vtkIdType target)
{
  if (source > this->GetNumberOfNodes() - 1 || target > this->GetNumberOfNodes() - 1)
    {
    this->SetNumberOfNodes(source > target ? source + 1 : target + 1);
    }

  //cout << "inserting arc from " << source << " to " << target << endl;
  vtkIdType arc = this->Arcs->InsertNextValue(source) / 2;
  this->Arcs->InsertNextValue(target);

  // Insert the arc into the adjacency lists
  this->NodeLinks->AddOutAdjacent(source, arc);
  this->NodeLinks->AddInAdjacent(target, arc);

  return arc;
}

//----------------------------------------------------------------------------
void vtkGraph::RemoveNode(vtkIdType node)
{
  // Delete any arcs adjacent to the node.
  // We need to remove the const for sorting the arcs.
  // Remove the out arcs, then the in arcs.
  vtkIdType out;
  const vtkIdType* outArcs;
  this->NodeLinks->GetOutAdjacent(node, out, outArcs);
  this->RemoveArcs(const_cast<vtkIdType*>(outArcs), out);
  vtkIdType in;
  const vtkIdType* inArcs;
  this->NodeLinks->GetInAdjacent(node, in, inArcs);
  this->RemoveArcs(const_cast<vtkIdType*>(inArcs), in);

  // Move the final node on top of the deleted node
  vtkIdType movedNode = this->NodeLinks->RemoveNode(node);

  if (movedNode != node)
    {
    vtkIdType nodeDegree;
    const vtkIdType* nodeArcs;
    this->NodeLinks->GetAdjacent(node, nodeDegree, nodeArcs);
    for (vtkIdType e = 0; e < this->NodeLinks->GetInDegree(node); e++)
      {
      this->Arcs->SetValue(2*nodeArcs[e] + 1, node);
      }
    for (vtkIdType e = this->NodeLinks->GetInDegree(node); e < nodeDegree; e++)
      {
      this->Arcs->SetValue(2*nodeArcs[e], node);
      }
    }

  // Move the data of the final node on top of the data of the deleted node
  for (int i = 0; i < this->GetPointData()->GetNumberOfArrays(); i++)
    {
    vtkAbstractArray* aa = this->GetPointData()->GetAbstractArray(i);
    aa->SetTuple(node, movedNode, aa);
    aa->Resize(aa->GetNumberOfTuples() - 1);
    }
  if (this->Points)
    {
    this->Points->SetPoint(node, this->Points->GetPoint(movedNode));
    // NOTE:
    // vtkPoints does not have a resize method, so we have to do this the slow way.
    // The fast way would be:
    //this->Points->Resize(this->Points->GetNumberOfPoints() - 1);
    vtkPoints* newPoints = vtkPoints::New();
    for (vtkIdType i = 0; i < this->Points->GetNumberOfPoints() - 1; i++)
      {
      newPoints->InsertNextPoint(this->Points->GetPoint(i));
      }
    this->Points->Delete();
    this->Points = newPoints;
    }
}

//----------------------------------------------------------------------------
void vtkGraph::RemoveArc(vtkIdType arc)
{
  // Remove the arc from the source arc list
  vtkIdType source = this->Arcs->GetValue(2*arc);
  this->NodeLinks->RemoveOutAdjacent(source, arc);
  vtkIdType target = this->Arcs->GetValue(2*arc + 1);
  this->NodeLinks->RemoveInAdjacent(target, arc);

  // Move the final arc on top of the deleted arc
  vtkIdType movedArc = this->GetNumberOfArcs() - 1;
  vtkIdType movedSource = this->Arcs->GetValue(2*movedArc);
  vtkIdType movedTarget = this->Arcs->GetValue(2*movedArc + 1);

  this->Arcs->SetValue(2*arc, movedSource);
  this->Arcs->SetValue(2*arc + 1, movedTarget);
  this->Arcs->Resize(this->Arcs->GetNumberOfTuples() - 1);

  // Modify the adjacency lists to reflect the id change
  for (vtkIdType e = 0; e < this->NodeLinks->GetOutDegree(movedSource); e++)
    {
    if (this->NodeLinks->GetOutAdjacent(movedSource, e) == movedArc)
      {
      this->NodeLinks->SetOutAdjacent(movedSource, e, arc);
      break;
      }
    }
  for (vtkIdType e = 0; e < this->NodeLinks->GetInDegree(movedTarget); e++)
    {
    if (this->NodeLinks->GetInAdjacent(movedTarget, e) == movedArc)
      {
      this->NodeLinks->SetInAdjacent(movedTarget, e, arc);
      break;
      }
    }

  // Move the data of the final arc on top of the data of the deleted arc
  for (int i = 0; i < this->GetCellData()->GetNumberOfArrays(); i++)
    {
    vtkAbstractArray* aa = this->GetCellData()->GetAbstractArray(i);
    aa->SetTuple(arc, movedArc, aa);
    aa->Resize(aa->GetNumberOfTuples() - 1);
    }
}

//----------------------------------------------------------------------------
void vtkGraph::RemoveNodes(vtkIdType* nodes, vtkIdType size)
{
  // Sort the nodes
  vtkstd::sort(nodes, nodes + size);

  // Delete the nodes in reverse order
  for (vtkIdType i = size - 1; i >= 0; i--)
    {
    // Don't delete the same node twice
    if (i == size - 1 || nodes[i] != nodes[i+1])
      {
      this->RemoveNode(nodes[i]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkGraph::RemoveArcs(vtkIdType* arcs, vtkIdType size)
{
  // Sort the arcs
  vtkstd::sort(arcs, arcs + size);

  // Delete the arcs in reverse order
  for (vtkIdType i = size - 1; i >= 0; i--)
    {
    // Don't delete the same arc twice.
    // This may happen if there are loops in the graph.
    if (i == size - 1 || arcs[i] != arcs[i+1])
      {
      this->RemoveArc(arcs[i]);
      }
    }
}

void vtkGraph::ClearNode(vtkIdType node)
{
  // Delete any arcs adjacent to the node.
  // We need to remove the const for sorting the arcs.
  // Remove the out arcs, then the in arcs.
  vtkIdType out;
  const vtkIdType* outArcs;
  this->NodeLinks->GetOutAdjacent(node, out, outArcs);
  this->RemoveArcs(const_cast<vtkIdType*>(outArcs), out);
  vtkIdType in;
  const vtkIdType* inArcs;
  this->NodeLinks->GetInAdjacent(node, in, inArcs);
  this->RemoveArcs(const_cast<vtkIdType*>(inArcs), in);
}

//----------------------------------------------------------------------------
vtkGraph* vtkGraph::GetData(vtkInformation* info)
{
  return info? vtkGraph::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkGraph* vtkGraph::GetData(vtkInformationVector* v, int i)
{
  return vtkGraph::GetData(v->GetInformationObject(i));
}

