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
  if ( (matrix = n->GetMatrix()) != NULL )
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
  if ( this->Top == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkAssemblyNode *>(this->Top->Item);
    }
}

vtkAssemblyNode *vtkAssemblyPath::GetLastNode() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkAssemblyNode *>(this->Bottom->Item);
    }
}

void vtkAssemblyPath::DeleteLastNode()
{
  vtkAssemblyNode *node = this->GetLastNode();
  this->vtkCollection::RemoveItem(node);
  
  this->Transform->Pop();
}

void vtkAssemblyPath::ShallowCopy(vtkAssemblyPath *path)
{
  vtkAssemblyNode *node;

  this->RemoveAllItems();
  for ( path->InitTraversal(); (node = path->GetNextNode()); )
    {
    this->vtkCollection::AddItem(node);
    }
}

unsigned long vtkAssemblyPath::GetMTime()
{
  unsigned long mtime=this->vtkCollection::GetMTime();
  unsigned long nodeMTime;
  vtkAssemblyNode *node;
  
  for ( this->InitTraversal(); (node = this->GetNextNode()); )
    {
    nodeMTime = node->GetMTime();
    if ( nodeMTime > mtime )
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
