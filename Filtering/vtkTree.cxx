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
#include "vtkVertexLinks.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include "vtkstd/algorithm"

// 
// Standard functions
//

vtkCxxRevisionMacro(vtkTree, "1.4");
vtkStandardNewMacro(vtkTree);

//----------------------------------------------------------------------------

void vtkTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VertexLinks: " << endl;
  this->VertexLinks->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------

vtkTree::vtkTree()
{
  this->Root = -1;
  this->VertexLinks = vtkVertexLinks::New();
}

//----------------------------------------------------------------------------

void vtkTree::Initialize()
{
  this->Superclass::Initialize();
  this->Root = -1;
  this->VertexLinks->Delete();
  this->VertexLinks = vtkVertexLinks::New();
}

//----------------------------------------------------------------------------

vtkTree::~vtkTree()
{
  if (this->VertexLinks)
    {
    this->VertexLinks->Delete();
    this->VertexLinks = NULL;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetNumberOfEdges()
{
  return this->GetNumberOfVertices() - 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetNumberOfVertices()
{
  return this->VertexLinks->GetNumberOfVertices();
}

//----------------------------------------------------------------------------
void vtkTree::GetAdjacentVertices(vtkIdType vertex, vtkGraphIdList* vertexIds)
{
  vertexIds->Reset();
  if (vertex != this->Root)
    {
    vertexIds->InsertNextId(this->GetParent(vertex));
    }
  vtkIdType nchildren;
  const vtkIdType* children;
  this->VertexLinks->GetOutAdjacent(vertex, nchildren, children);
  for (vtkIdType i = 0; i < nchildren; i++)
    {
    vertexIds->InsertNextId(children[i]);
    }
}

//----------------------------------------------------------------------------
void vtkTree::GetInVertices(vtkIdType vertex, vtkGraphIdList* vertexIds)
{
  vertexIds->Reset();
  if (vertex != this->Root)
    {
    vertexIds->InsertNextId(this->VertexLinks->GetInAdjacent(vertex, 0));
    }
}

//----------------------------------------------------------------------------
void vtkTree::GetOutVertices(vtkIdType vertex, vtkGraphIdList* vertexIds)
{
  vtkIdType nchildren;
  const vtkIdType* children;
  this->VertexLinks->GetOutAdjacent(vertex, nchildren, children);
  vertexIds->SetArray(const_cast<vtkIdType*>(children), nchildren, true);
}

//----------------------------------------------------------------------------
void vtkTree::GetIncidentEdges(vtkIdType vertex, vtkGraphIdList* edgeIds)
{
  edgeIds->Reset();
  if (vertex != this->Root)
    {
    edgeIds->InsertNextId(this->GetParentEdge(vertex));
    }
  vtkIdType nchildren;
  const vtkIdType* children;
  this->VertexLinks->GetOutAdjacent(vertex, nchildren, children);
  for (vtkIdType i = 0; i < nchildren; i++)
    {
    edgeIds->InsertNextId(this->GetParentEdge(children[i]));
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetDegree(vtkIdType vertex)
{
  return this->VertexLinks->GetDegree(vertex);
}

//----------------------------------------------------------------------------
void vtkTree::GetInEdges(vtkIdType vertex, vtkGraphIdList* edgeIds)
{
  edgeIds->Reset();
  if (vertex != this->Root)
    {
    edgeIds->InsertNextId(this->GetParentEdge(vertex));
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetInDegree(vtkIdType vertex)
{
  if (vertex == this->Root)
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkTree::GetOutEdges(vtkIdType vertex, vtkGraphIdList* edgeIds)
{
  edgeIds->Reset();
  vtkIdType nchildren;
  const vtkIdType* children;
  this->VertexLinks->GetOutAdjacent(vertex, nchildren, children);
  for (vtkIdType i = 0; i < nchildren; i++)
    {
    edgeIds->InsertNextId(this->GetParentEdge(children[i]));
    }
}

//----------------------------------------------------------------------------
void vtkTree::GetChildren(vtkIdType vertex, vtkIdType& nchildren, const vtkIdType*& children)
{
  this->VertexLinks->GetOutAdjacent(vertex, nchildren, children);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetOutDegree(vtkIdType vertex)
{
  return this->VertexLinks->GetOutDegree(vertex);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetNumberOfChildren(vtkIdType vertex)
{
  return this->VertexLinks->GetOutDegree(vertex);
}

vtkIdType vtkTree::GetChild(vtkIdType parent, vtkIdType index)
{
  return this->VertexLinks->GetOutAdjacent(parent, index);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetSourceVertex(vtkIdType edge)
{
  if (edge == this->Root - 1)
    {
    return 0;
    }
  return edge + 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetTargetVertex(vtkIdType edge)
{
  if (edge == this->Root - 1)
    {
    return this->VertexLinks->GetInAdjacent(0, 0);
    }
  return this->VertexLinks->GetInAdjacent(edge + 1, 0);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::GetOppositeVertex(vtkIdType edge, vtkIdType vertex)
{
  if (this->GetSourceVertex(edge) != vertex)
    {
    return this->GetSourceVertex(edge);
    }
  return this->GetTargetVertex(edge);
}

//----------------------------------------------------------------------------
void vtkTree::ShallowCopy(vtkDataObject *dataObject)
{
  vtkTree* tree = vtkTree::SafeDownCast(dataObject);

  if ( tree != NULL )
    {
    if (this->VertexLinks)
      {
      this->VertexLinks->Delete();
      }
    this->VertexLinks = tree->VertexLinks;
    if (this->VertexLinks)
      {
      this->VertexLinks->Register(this);
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
    this->VertexLinks->DeepCopy(tree->VertexLinks);
    this->Root = tree->Root;
    }

  // Do superclass
  this->Superclass::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
vtkIdType vtkTree::AddVertex()
{
  return this->VertexLinks->AddVertex();
}

//----------------------------------------------------------------------------
void vtkTree::RemoveVertex(vtkIdType vertex)
{
  // Remove the vertex from its parent's child list
  if (vertex != this->Root)
    {
    vtkIdType parent = this->GetParent(vertex);
    this->VertexLinks->RemoveOutAdjacent(parent, vertex);
    }

  // Move the final vertex on top of the deleted vertex
  vtkIdType movedVertex = this->VertexLinks->RemoveVertex(vertex);

  // Update the parent link from all children
  vtkIdType nchildren;
  const vtkIdType* children;
  this->VertexLinks->GetOutAdjacent(vertex, nchildren, children);
  for (vtkIdType c = 0; c < nchildren; c++)
    {
    this->VertexLinks->SetInAdjacent(children[c], 0, vertex);
    }

  // If not the root, update the child link from the parent
  if (movedVertex != this->Root)
    {
    vtkIdType parent = this->GetParent(vertex);
    for (vtkIdType c = 0; c < this->VertexLinks->GetOutDegree(parent); c++)
      {
      if (this->VertexLinks->GetOutAdjacent(parent, c) == movedVertex)
        {
        this->VertexLinks->SetOutAdjacent(parent, c, vertex);
        }
      }
    }

  // Move the data of the final vertex on top of the data of the deleted vertex
  for (int i = 0; i < this->GetPointData()->GetNumberOfArrays(); i++)
    {
    vtkAbstractArray* aa = this->GetPointData()->GetAbstractArray(i);
    aa->SetTuple(vertex, movedVertex, aa);
    aa->Resize(aa->GetNumberOfTuples() - 1);
    }
  if (this->Points)
    {
    this->Points->SetPoint(vertex, this->Points->GetPoint(movedVertex));
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

  // If not the root, move the data of the final edge on top of the data of the deleted edge
  if (vertex != this->Root)
    {
    vtkIdType edge = this->GetParentEdge(vertex);
    vtkIdType movedEdge = this->GetParentEdge(movedVertex);
    for (int i = 0; i < this->GetCellData()->GetNumberOfArrays(); i++)
      {
      vtkAbstractArray* aa = this->GetCellData()->GetAbstractArray(i);
      aa->SetTuple(edge, movedEdge, aa);
      aa->Resize(aa->GetNumberOfTuples() - 1);
      }
    }

  // Change the root if we moved it
  if (movedVertex == this->Root)
    {
    this->Root = vertex;
    }

  // Resize the tree, set the root to -1 if the tree is empty.
  if (this->VertexLinks->GetNumberOfVertices() == 0)
    {
    this->Root = -1;
    }
}

//----------------------------------------------------------------------------
void vtkTree::RemoveVertices(vtkIdType* vertices, vtkIdType size)
{
  // Sort the vertices
  vtkstd::sort(vertices, vertices + size);

  // Delete the vertices in reverse order
  for (vtkIdType i = size - 1; i >= 0; i--)
    {
    // Don't delete the same vertex twice
    if (i == size - 1 || vertices[i] != vertices[i+1])
      {
      this->RemoveVertex(vertices[i]);
      }
    }
}

vtkIdType vtkTree::GetRoot()
{
  return this->Root;
}

vtkIdType vtkTree::GetParent(vtkIdType child)
{
  if (child < 0 || child >= this->GetNumberOfVertices())
    {
    return -1;
    }

  // The root is its own parent
  if (child == this->Root)
    {
    return child;
    }

  return this->VertexLinks->GetInAdjacent(child, 0);
}

vtkIdType vtkTree::GetLevel(vtkIdType vertex)
{
  vtkIdType level = 0;
  while (vertex != this->Root)
    {
    vertex = this->GetParent(vertex);
    level++;
    }
  return level;
}

bool vtkTree::IsLeaf(vtkIdType vertex)
{
  return this->GetNumberOfChildren(vertex) == 0;
}

vtkIdType vtkTree::AddRoot()
{
  if (this->GetNumberOfVertices() > 0)
    {
    return -1;
    }

  this->Root = this->AddVertex();
  return this->Root;
}

vtkIdType vtkTree::AddChild(vtkIdType parent)
{
  if (parent >= this->GetNumberOfVertices())
    {
    return -1;
    }

  vtkIdType vertex = this->AddVertex();
  this->VertexLinks->AddOutAdjacent(parent, vertex);
  this->VertexLinks->AddInAdjacent(vertex, parent);
  return vertex;
}

void vtkTree::SetRoot(vtkIdType root)
{
  if (root >= this->GetNumberOfVertices())
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

  // Reverse the hieredgehy on the path from new root to old root
  vtkIdType cur = root;
  vtkIdType parent = this->GetParent(cur);
  while (cur != this->Root)
    {
    // Remove current vertex from parent's child list
    this->VertexLinks->RemoveOutAdjacent(parent, cur);

    // Add the parent to the current vertex's child list
    this->VertexLinks->AddOutAdjacent(cur, parent);

    // Change the parent's parent to the current vertex
    vtkIdType oldCur = cur;
    cur = parent;
    parent = this->VertexLinks->GetInAdjacent(parent, 0);
    if (this->VertexLinks->GetInDegree(cur) > 0)
      {
      this->VertexLinks->SetInAdjacent(cur, 0, oldCur);
      }
    else
      {
      this->VertexLinks->AddInAdjacent(cur, oldCur);
      }
    }
  // Remove any in edge from the new root vertex
  if (this->VertexLinks->GetInDegree(root) > 0)
    {
    this->VertexLinks->RemoveInAdjacent(root, this->VertexLinks->GetInAdjacent(root, 0));
    }
  this->Root = root;
}

void vtkTree::RemoveVertexAndDescendants(vtkIdType vertex)
{
  // Get the list of all descendants
  vtkIdList* descend = vtkIdList::New();
  descend->InsertNextId(vertex);
  vtkIdType completed = 0;
  while (completed < descend->GetNumberOfIds())
    {
    vtkIdType unfinishedVertex = descend->GetId(completed);
    vtkIdType nchildren;
    const vtkIdType* children;
    this->GetChildren(unfinishedVertex, nchildren, children);
    for (vtkIdType i = 0; i < nchildren; i++)
      {
      descend->InsertNextId(children[i]);
      }
    completed++;
    }

  // Delete the vertices
  this->RemoveVertices(descend->GetPointer(0), descend->GetNumberOfIds());
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
  if (child < 0 || child >= this->GetNumberOfVertices())
    {
    vtkErrorMacro(<< "Child " << child << " has invalid ID.");
    return;
    }
  if (parent < 0 || parent >= this->GetNumberOfVertices())
    {
    vtkErrorMacro(<< "Parent " << parent << " has invalid ID.");
    return;
    }
  if (child == this->Root)
    {
    vtkErrorMacro(<< "Cannot move the root vertex under another vertex.");
    return;
    }

  vtkIdType oldParent = this->GetParent(child);

  // Delete the vertex from the old parent's child list
  this->VertexLinks->RemoveOutAdjacentShift(oldParent, child);

  // Add the vertex to the new parent's child list
  this->VertexLinks->AddOutAdjacent(parent, child);

  // Change the parent of the child to the new parent
  this->VertexLinks->SetInAdjacent(child, 0, parent);
}

vtkIdType vtkTree::GetParentEdge(vtkIdType vertex)
{
  if (vertex == this->Root)
    {
    return -1;
    }
  if (vertex == 0)
    {
    return this->Root - 1;
    }
  return vertex - 1;
}
