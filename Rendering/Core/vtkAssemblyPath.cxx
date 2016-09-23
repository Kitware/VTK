/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAssemblyPath.h"

#include "vtkAssemblyNode.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"

vtkStandardNewMacro(vtkAssemblyPath);

vtkAssemblyPath::vtkAssemblyPath()
{
  this->Transform = vtkTransform::New();
  this->Transform->PreMultiply();
  this->TransformedProp = NULL;
}

vtkAssemblyPath::~vtkAssemblyPath()
{
  this->Transform->Delete();
  if ( this->TransformedProp != NULL )
  {
    this->TransformedProp->Delete();
  }
}

void vtkAssemblyPath::AddNode(vtkProp *p, vtkMatrix4x4 *m)
{
  vtkAssemblyNode *n = vtkAssemblyNode::New();
  n->SetViewProp(p);
  n->SetMatrix(m); //really a copy because we're gonna compute with it
  this->AddNode(n);
  n->Delete(); //ok reference counted
}

void vtkAssemblyPath::AddNode(vtkAssemblyNode *n)
{
  // First add the node to the list
  this->vtkCollection::AddItem(n);

  // Grab the matrix, if any, and concatenate it
  this->Transform->Push(); //keep in synch with list of nodes
  vtkMatrix4x4 *matrix;
  if ((matrix = n->GetMatrix()) != NULL)
  {
    this->Transform->Concatenate(matrix);
    this->Transform->GetMatrix(matrix); //replace previous matrix
  }
}

vtkAssemblyNode *vtkAssemblyPath::GetNextNode()
{
  return static_cast<vtkAssemblyNode *>(this->GetNextItemAsObject());
}

vtkAssemblyNode *vtkAssemblyPath::GetFirstNode()
{
  return this->Top ?
    static_cast<vtkAssemblyNode*>(this->Top->Item) : 0;
}

vtkAssemblyNode *vtkAssemblyPath::GetLastNode()
{
  return this->Bottom ?
    static_cast<vtkAssemblyNode*>(this->Bottom->Item) : 0;
}

void vtkAssemblyPath::DeleteLastNode()
{
  vtkAssemblyNode *node = this->GetLastNode();
  this->vtkCollection::RemoveItem(node);
  this->Transform->Pop();
}

void vtkAssemblyPath::ShallowCopy(vtkAssemblyPath *path)
{
  this->RemoveAllItems();

  vtkAssemblyNode *node;
  for (path->InitTraversal(); (node = path->GetNextNode());)
  {
    this->vtkCollection::AddItem(node);
  }
}

vtkMTimeType vtkAssemblyPath::GetMTime()
{
  vtkMTimeType mtime = this->vtkCollection::GetMTime();

  vtkAssemblyNode *node;
  for (this->InitTraversal(); (node = this->GetNextNode());)
  {
    vtkMTimeType nodeMTime = node->GetMTime();
    if (nodeMTime > mtime)
    {
      mtime = nodeMTime;
    }
  }
  return mtime;
}

void vtkAssemblyPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
