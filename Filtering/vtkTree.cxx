/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTree.h"

#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"
#include "vtkGraphIdList.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkVoidArray.h"
#include "vtkNodeLinks.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include "vtkstd/algorithm"

// 
// Standard functions
//

vtkCxxRevisionMacro(vtkTree, "1.3");
vtkStandardNewMacro(vtkTree);

//----------------------------------------------------------------------------

void vtkTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NodeLinks: " << endl;
  this->NodeLinks->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------

vtkTree::vtkTree()
{
  this->Root = -1;
  this->NodeLinks = vtkNodeLinks::New();
}

//----------------------------------------------------------------------------

void vtkTree::Initialize()
{
  this->Superclass::Initialize();
  this->Root = -1;
  this->NodeLinks->Delete();
  this->NodeLinks = vtkNodeLinks::New();
}

//----------------------------------------------------------------------------

vtkTree::~vtkTree()
{
  if (this->NodeLinks)
    {
    this->NodeLinks->Delete();
    this->NodeLinks = NULL;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetNumberOfArcs()
{
  return this->GetNumberOfNodes() - 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetNumberOfNodes()
{
  return this->NodeLinks->GetNumberOfNodes();
}

//----------------------------------------------------------------------------
void vtkTree::GetAdjacentNodes(vtkIdType node, vtkGraphIdList* nodeIds)
{
  nodeIds->Reset();
  if (node != this->Root)
    {
    nodeIds->InsertNextId(this->GetParent(node));
    }
  vtkIdType nchildren;
  const vtkIdType* children;
  this->NodeLinks->GetOutAdjacent(node, nchildren, children);
  for (vtkIdType i = 0; i < nchildren; i++)
    {
    nodeIds->InsertNextId(children[i]);
    }
}

//----------------------------------------------------------------------------
void vtkTree::GetInNodes(vtkIdType node, vtkGraphIdList* nodeIds)
{
  nodeIds->Reset();
  if (node != this->Root)
    {
    nodeIds->InsertNextId(this->NodeLinks->GetInAdjacent(node, 0));
    }
}

//----------------------------------------------------------------------------
void vtkTree::GetOutNodes(vtkIdType node, vtkGraphIdList* nodeIds)
{
  vtkIdType nchildren;
  const vtkIdType* children;
  this->NodeLinks->GetOutAdjacent(node, nchildren, children);
  nodeIds->SetArray(const_cast<vtkIdType*>(children), nchildren, true);
}

//----------------------------------------------------------------------------
void vtkTree::GetIncidentArcs(vtkIdType node, vtkGraphIdList* arcIds)
{
  arcIds->Reset();
  if (node != this->Root)
    {
    arcIds->InsertNextId(this->GetParentArc(node));
    }
  vtkIdType nchildren;
  const vtkIdType* children;
  this->NodeLinks->GetOutAdjacent(node, nchildren, children);
  for (vtkIdType i = 0; i < nchildren; i++)
    {
    arcIds->InsertNextId(this->GetParentArc(children[i]));
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetDegree(vtkIdType node)
{
  return this->NodeLinks->GetDegree(node);
}

//----------------------------------------------------------------------------
void vtkTree::GetInArcs(vtkIdType node, vtkGraphIdList* arcIds)
{
  arcIds->Reset();
  if (node != this->Root)
    {
    arcIds->InsertNextId(this->GetParentArc(node));
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetInDegree(vtkIdType node)
{
  if (node == this->Root)
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkTree::GetOutArcs(vtkIdType node, vtkGraphIdList* arcIds)
{
  arcIds->Reset();
  vtkIdType nchildren;
  const vtkIdType* children;
  this->NodeLinks->GetOutAdjacent(node, nchildren, children);
  for (vtkIdType i = 0; i < nchildren; i++)
    {
    arcIds->InsertNextId(this->GetParentArc(children[i]));
    }
}

//----------------------------------------------------------------------------
void vtkTree::GetChildren(vtkIdType node, vtkIdType& nchildren, const vtkIdType*& children)
{
  this->NodeLinks->GetOutAdjacent(node, nchildren, children);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetOutDegree(vtkIdType node)
{
  return this->NodeLinks->GetOutDegree(node);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetNumberOfChildren(vtkIdType node)
{
  return this->NodeLinks->GetOutDegree(node);
}

vtkIdType vtkTree::GetChild(vtkIdType parent, vtkIdType index)
{
  return this->NodeLinks->GetOutAdjacent(parent, index);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetSourceNode(vtkIdType arc)
{
  if (arc == this->Root - 1)
    {
    return 0;
    }
  return arc + 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetTargetNode(vtkIdType arc)
{
  if (arc == this->Root - 1)
    {
    return this->NodeLinks->GetInAdjacent(0, 0);
    }
  return this->NodeLinks->GetInAdjacent(arc + 1, 0);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetOppositeNode(vtkIdType arc, vtkIdType node)
{
  if (this->GetSourceNode(arc) != node)
    {
    return this->GetSourceNode(arc);
    }
  return this->GetTargetNode(arc);
}

//----------------------------------------------------------------------------
void vtkTree::ShallowCopy(vtkDataObject *dataObject)
{
  vtkTree* tree = vtkTree::SafeDownCast(dataObject);

  if ( tree != NULL )
    {
    if (this->NodeLinks)
      {
      this->NodeLinks->Delete();
      }
    this->NodeLinks = tree->NodeLinks;
    if (this->NodeLinks)
      {
      this->NodeLinks->Register(this);
      }
    this->Root = tree->Root;
    }

  // Do superclass
  this->Superclass::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkTree::DeepCopy(vtkDataObject *dataObject)
{
  vtkTree* tree = vtkTree::SafeDownCast(dataObject);

  if ( tree != NULL )
    {
    this->NodeLinks->DeepCopy(tree->NodeLinks);
    this->Root = tree->Root;
    }

  // Do superclass
  this->Superclass::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::AddNode()
{
  return this->NodeLinks->AddNode();
}

//----------------------------------------------------------------------------
void vtkTree::RemoveNode(vtkIdType node)
{
  // Remove the node from its parent's child list
  if (node != this->Root)
    {
    vtkIdType parent = this->GetParent(node);
    this->NodeLinks->RemoveOutAdjacent(parent, node);
    }

  // Move the final node on top of the deleted node
  vtkIdType movedNode = this->NodeLinks->RemoveNode(node);

  // Update the parent link from all children
  vtkIdType nchildren;
  const vtkIdType* children;
  this->NodeLinks->GetOutAdjacent(node, nchildren, children);
  for (vtkIdType c = 0; c < nchildren; c++)
    {
    this->NodeLinks->SetInAdjacent(children[c], 0, node);
    }

  // If not the root, update the child link from the parent
  if (movedNode != this->Root)
    {
    vtkIdType parent = this->GetParent(node);
    for (vtkIdType c = 0; c < this->NodeLinks->GetOutDegree(parent); c++)
      {
      if (this->NodeLinks->GetOutAdjacent(parent, c) == movedNode)
        {
        this->NodeLinks->SetOutAdjacent(parent, c, node);
        }
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

  // If not the root, move the data of the final arc on top of the data of the deleted arc
  if (node != this->Root)
    {
    vtkIdType arc = this->GetParentArc(node);
    vtkIdType movedArc = this->GetParentArc(movedNode);
    for (int i = 0; i < this->GetCellData()->GetNumberOfArrays(); i++)
      {
      vtkAbstractArray* aa = this->GetCellData()->GetAbstractArray(i);
      aa->SetTuple(arc, movedArc, aa);
      aa->Resize(aa->GetNumberOfTuples() - 1);
      }
    }

  // Change the root if we moved it
  if (movedNode == this->Root)
    {
    this->Root = node;
    }

  // Resize the tree, set the root to -1 if the tree is empty.
  if (this->NodeLinks->GetNumberOfNodes() == 0)
    {
    this->Root = -1;
    }
}

//----------------------------------------------------------------------------
void vtkTree::RemoveNodes(vtkIdType* nodes, vtkIdType size)
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

vtkIdType vtkTree::GetRoot()
{
  return this->Root;
}

vtkIdType vtkTree::GetParent(vtkIdType child)
{
  if (child < 0 || child >= this->GetNumberOfNodes())
    {
    return -1;
    }

  // The root is its own parent
  if (child == this->Root)
    {
    return child;
    }

  return this->NodeLinks->GetInAdjacent(child, 0);
}

vtkIdType vtkTree::GetLevel(vtkIdType node)
{
  vtkIdType level = 0;
  while (node != this->Root)
    {
    node = this->GetParent(node);
    level++;
    }
  return level;
}

bool vtkTree::IsLeaf(vtkIdType node)
{
  return this->GetNumberOfChildren(node) == 0;
}

vtkIdType vtkTree::AddRoot()
{
  if (this->GetNumberOfNodes() > 0)
    {
    return -1;
    }

  this->Root = this->AddNode();
  return this->Root;
}

vtkIdType vtkTree::AddChild(vtkIdType parent)
{
  if (parent >= this->GetNumberOfNodes())
    {
    return -1;
    }

  vtkIdType node = this->AddNode();
  this->NodeLinks->AddOutAdjacent(parent, node);
  this->NodeLinks->AddInAdjacent(node, parent);
  return node;
}

void vtkTree::SetRoot(vtkIdType root)
{
  if (root >= this->GetNumberOfNodes())
    {
    return;
    }

  if (root < 0)
    {
    return;
    }

  if (this->Root == root)
    {
    return;
    }

  // Reverse the hierarchy on the path from new root to old root
  vtkIdType cur = root;
  vtkIdType parent = this->GetParent(cur);
  while (cur != this->Root)
    {
    // Remove current node from parent's child list
    this->NodeLinks->RemoveOutAdjacent(parent, cur);

    // Add the parent to the current node's child list
    this->NodeLinks->AddOutAdjacent(cur, parent);

    // Change the parent's parent to the current node
    vtkIdType oldCur = cur;
    cur = parent;
    parent = this->NodeLinks->GetInAdjacent(parent, 0);
    if (this->NodeLinks->GetInDegree(cur) > 0)
      {
      this->NodeLinks->SetInAdjacent(cur, 0, oldCur);
      }
    else
      {
      this->NodeLinks->AddInAdjacent(cur, oldCur);
      }
    }
  // Remove any in edge from the new root node
  if (this->NodeLinks->GetInDegree(root) > 0)
    {
    this->NodeLinks->RemoveInAdjacent(root, this->NodeLinks->GetInAdjacent(root, 0));
    }
  this->Root = root;
}

void vtkTree::RemoveNodeAndDescendants(vtkIdType node)
{
  // Get the list of all descendants
  vtkIdList* descend = vtkIdList::New();
  descend->InsertNextId(node);
  vtkIdType completed = 0;
  while (completed < descend->GetNumberOfIds())
    {
    vtkIdType unfinishedNode = descend->GetId(completed);
    vtkIdType nchildren;
    const vtkIdType* children;
    this->GetChildren(unfinishedNode, nchildren, children);
    for (vtkIdType i = 0; i < nchildren; i++)
      {
      descend->InsertNextId(children[i]);
      }
    completed++;
    }

  // Delete the nodes
  this->RemoveNodes(descend->GetPointer(0), descend->GetNumberOfIds());
  descend->Delete();
}

//----------------------------------------------------------------------------
vtkTree* vtkTree::GetData(vtkInformation* info)
{
  return info? vtkTree::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkTree* vtkTree::GetData(vtkInformationVector* v, int i)
{
  return vtkTree::GetData(v->GetInformationObject(i));
}


void vtkTree::SetParent(vtkIdType child, vtkIdType parent)
{
  if (child < 0 || child >= this->GetNumberOfNodes())
    {
    vtkErrorMacro(<< "Child " << child << " has invalid ID.");
    return;
    }
  if (parent < 0 || parent >= this->GetNumberOfNodes())
    {
    vtkErrorMacro(<< "Parent " << parent << " has invalid ID.");
    return;
    }
  if (child == this->Root)
    {
    vtkErrorMacro(<< "Cannot move the root node under another node.");
    return;
    }

  vtkIdType oldParent = this->GetParent(child);

  // Delete the node from the old parent's child list
  this->NodeLinks->RemoveOutAdjacentShift(oldParent, child);

  // Add the node to the new parent's child list
  this->NodeLinks->AddOutAdjacent(parent, child);

  // Change the parent of the child to the new parent
  this->NodeLinks->SetInAdjacent(child, 0, parent);
}

vtkIdType vtkTree::GetParentArc(vtkIdType node)
{
  if (node == this->Root)
    {
    return -1;
    }
  if (node == 0)
    {
    return this->Root - 1;
    }
  return node - 1;
}
