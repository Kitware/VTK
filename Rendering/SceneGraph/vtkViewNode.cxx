/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkViewNode.h"

#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkViewNodeCollection.h"
#include "vtkViewNodeFactory.h"

//----------------------------------------------------------------------------
const char *vtkViewNode::operation_type_strings[] =
{
  "noop",
  "build",
  "synchronize",
  "render",
  NULL
};

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkViewNode,Children,vtkViewNodeCollection);

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkViewNode,MyFactory,vtkViewNodeFactory);

//----------------------------------------------------------------------------
vtkViewNode::vtkViewNode()
{
  this->Renderable = NULL;
  this->Parent = NULL;
  this->Children = vtkViewNodeCollection::New();
  this->PreparedNodes = vtkCollection::New();
  this->MyFactory = NULL;

  this->RenderTime = 0;
}

//----------------------------------------------------------------------------
vtkViewNode::~vtkViewNode()
{
  this->Parent = 0;
  if (this->Children)
  {
    this->Children->Delete();
    this->Children = 0;
  }
  if (this->MyFactory)
  {
    this->MyFactory->Delete();
    this->MyFactory = 0;
  }
  if (this->PreparedNodes)
  {
    this->PreparedNodes->Delete();
    this->PreparedNodes = 0;
  }
}

//----------------------------------------------------------------------------
void vtkViewNode::SetParent(vtkViewNode *p)
{
  this->Parent = p;
}

//----------------------------------------------------------------------------
vtkViewNode *vtkViewNode::GetParent()
{
  return this->Parent.GetPointer();
}


//----------------------------------------------------------------------------
void vtkViewNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkViewNode::PrepareNodes()
{
  this->PreparedNodes->RemoveAllItems();
}

//----------------------------------------------------------------------------
void vtkViewNode::RemoveUnusedNodes()
{
  //remove viewnodes if their renderables are no longer present
  vtkViewNodeCollection *nodes = this->GetChildren();
  vtkCollectionIterator *nit = nodes->NewIterator();
  nit->InitTraversal();
  while (!nit->IsDoneWithTraversal())
  {
    vtkViewNode *node = vtkViewNode::SafeDownCast(nit->GetCurrentObject());
    if (node)
    {
      vtkObject *obj = node->GetRenderable();
      if (!this->PreparedNodes->IsItemPresent(obj))
      {
        nodes->RemoveItem(node);
        nit->InitTraversal(); //don't stumble over deleted node
      }
    }
    nit->GoToNextItem();
  }
  nit->Delete();

  this->PrepareNodes();
}

//----------------------------------------------------------------------------
void vtkViewNode::AddMissingNodes(vtkCollection *col)
{
  //add viewnodes for renderables that are not yet present
  vtkViewNodeCollection *nodes = this->GetChildren();
  vtkCollectionIterator *rit = col->NewIterator();
  rit->InitTraversal();
  while (!rit->IsDoneWithTraversal())
  {
    vtkObject *obj = rit->GetCurrentObject();
    if (obj)
    {
      this->PreparedNodes->AddItem(obj);
      if (!nodes->IsRenderablePresent(obj))
      {
        vtkViewNode *node = this->CreateViewNode(obj);
        if (node)
        {
          nodes->AddItem(node);
          node->SetParent(this);
          node->Delete();
        }
      }
    }
    rit->GoToNextItem();
  }
  rit->Delete();
}

//----------------------------------------------------------------------------
void vtkViewNode::AddMissingNode(vtkObject *obj)
{
  if (!obj)
  {
    return;
  }

  //add viewnodes for renderables that are not yet present
  vtkViewNodeCollection *nodes = this->GetChildren();
  this->PreparedNodes->AddItem(obj);
  if (!nodes->IsRenderablePresent(obj))
  {
    vtkViewNode *node = this->CreateViewNode(obj);
    if (node)
    {
      nodes->AddItem(node);
      node->SetParent(this);
      node->Delete();
    }
  }
}

//----------------------------------------------------------------------------
void vtkViewNode::TraverseAllPasses()
{
  this->Traverse(build);
  this->Traverse(synchronize);
  this->Traverse(render);
}

//----------------------------------------------------------------------------
void vtkViewNode::Traverse(int operation)
{
  this->Apply(operation,true);

  vtkCollectionIterator *it = this->Children->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
  {
    vtkViewNode *child = vtkViewNode::SafeDownCast(it->GetCurrentObject());
    child->Traverse(operation);
    it->GoToNextItem();
  }
  it->Delete();

  this->Apply(operation, false);
}

//----------------------------------------------------------------------------
vtkViewNode *vtkViewNode::CreateViewNode(vtkObject *obj)
{
  vtkViewNode *ret = NULL;
  if (!this->MyFactory)
  {
    vtkWarningMacro("Can not create view nodes without my own factory");
  }
  else
  {
    ret = this->MyFactory->CreateNode(obj);
    if (ret)
    {
      ret->Renderable = obj;
    }
  }
  return ret;
}

//----------------------------------------------------------------------------
vtkViewNode *vtkViewNode::GetFirstAncestorOfType(const char *type)
{
  if (!this->Parent)
  {
    return NULL;
  }
  if (this->Parent->IsA(type))
  {
    return this->Parent;
  }
  return this->Parent->GetFirstAncestorOfType(type);
}

//----------------------------------------------------------------------------
void vtkViewNode::SetRenderable(vtkObject *obj)
{
  this->Renderable = obj;
}

//----------------------------------------------------------------------------
void vtkViewNode::Apply(int operation, bool prepass)
{
  //cerr << this->GetClassName() << "(" << this << ") Apply("
  //     << vtkViewNode::operation_type_strings[operation] << ")" << endl;
  switch (operation)
  {
    case noop:
      break;
    case build:
      this->Build(prepass);
      break;
    case synchronize:
      this->Synchronize(prepass);
      break;
    case render:
      this->Render(prepass);
      break;
    case invalidate:
      this->Invalidate(prepass);
      break;
    default:
      cerr << "UNKNOWN OPERATION" << operation << endl;
  }
}

//----------------------------------------------------------------------------
vtkViewNode* vtkViewNode::GetViewNodeFor(vtkObject *obj)
{
  if (this->Renderable == obj)
  {
    return this;
  }

  vtkViewNode *owner = NULL;
  vtkCollectionIterator *it = this->Children->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
  {
    vtkViewNode *child = vtkViewNode::SafeDownCast(it->GetCurrentObject());
    owner = child->GetViewNodeFor(obj);
    if (owner)
    {
      break;
    }
    it->GoToNextItem();
  }
  it->Delete();
  return owner;
}
