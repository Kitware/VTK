// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkViewNode.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkViewNodeFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
const char* vtkViewNode::operation_type_strings[] = { "noop", "build", "synchronize", "render",
  nullptr };

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkViewNode, MyFactory, vtkViewNodeFactory);

//------------------------------------------------------------------------------
vtkViewNode::vtkViewNode()
{
  this->Renderable = nullptr;
  this->Parent = nullptr;
  this->MyFactory = nullptr;

  this->RenderTime = 0;
}

//------------------------------------------------------------------------------
vtkViewNode::~vtkViewNode()
{
  this->Parent = nullptr;
  for (auto val : this->Children)
  {
    val->Delete();
  }
  this->Children.clear();
  if (this->MyFactory)
  {
    this->MyFactory->Delete();
    this->MyFactory = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkViewNode::SetParent(vtkViewNode* p)
{
  this->Parent = p;
}

//------------------------------------------------------------------------------
vtkViewNode* vtkViewNode::GetParent()
{
  return this->Parent;
}

//------------------------------------------------------------------------------
void vtkViewNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkViewNode::PrepareNodes()
{
  for (auto child : this->Children)
  {
    child->Used = false;
  }
}

//------------------------------------------------------------------------------
void vtkViewNode::RemoveUnusedNodes()
{
  for (auto it = this->Children.begin(); it != this->Children.end();)
  {
    auto curr = it;
    it++;
    if (!(*curr)->Used)
    {
      (*curr)->Delete();
      it = this->Children.erase(curr);
    }
  }

  // build the renderable set
  this->Renderables.clear();
  for (auto c : this->Children)
  {
    this->Renderables[c->GetRenderable()] = c;
  }
}

//------------------------------------------------------------------------------
void vtkViewNode::AddMissingNodes(vtkCollection* col)
{
  vtkCollectionSimpleIterator rit;
  col->InitTraversal(rit);
  while (rit)
  {
    vtkObject* obj = col->GetNextItemAsObject(rit);
    this->AddMissingNode(obj);
  }
}

//------------------------------------------------------------------------------
void vtkViewNode::AddMissingNode(vtkObject* obj)
{
  if (!obj)
  {
    return;
  }

  // add viewnodes for renderables that are not yet present
  auto nit = this->Renderables.find(obj);
  if (nit == this->Renderables.end())
  {
    vtkViewNode* node = this->CreateViewNode(obj);
    if (node)
    {
      this->Children.push_back(node);
      node->SetParent(this);
      node->Used = true;
    }
  }
  else
  {
    nit->second->Used = true;
  }
}

//------------------------------------------------------------------------------
void vtkViewNode::TraverseAllPasses()
{
  this->Traverse(build);
  this->Traverse(synchronize);
  this->Traverse(render);
}

//------------------------------------------------------------------------------
void vtkViewNode::Traverse(int operation)
{
  this->Apply(operation, true);

  for (auto val : this->Children)
  {
    val->Traverse(operation);
  }

  this->Apply(operation, false);
}

//------------------------------------------------------------------------------
vtkViewNode* vtkViewNode::CreateViewNode(vtkObject* obj)
{
  vtkViewNode* ret = nullptr;
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

//------------------------------------------------------------------------------
vtkViewNode* vtkViewNode::GetFirstAncestorOfType(const char* type)
{
  if (!this->Parent)
  {
    return nullptr;
  }
  if (this->Parent->IsA(type))
  {
    return this->Parent;
  }
  return this->Parent->GetFirstAncestorOfType(type);
}

//------------------------------------------------------------------------------
void vtkViewNode::SetRenderable(vtkObject* obj)
{
  this->Renderable = obj;
}

//------------------------------------------------------------------------------
void vtkViewNode::Apply(int operation, bool prepass)
{
  // cerr << this->GetClassName() << "(" << this << ") Apply("
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
  }
}

//------------------------------------------------------------------------------
vtkViewNode* vtkViewNode::GetViewNodeFor(vtkObject* obj)
{
  if (this->Renderable == obj)
  {
    return this;
  }

  for (auto child : this->Children)
  {
    auto owner = child->GetViewNodeFor(obj);
    if (owner)
    {
      return owner;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkViewNode* vtkViewNode::GetFirstChildOfType(const char* type)
{
  if (this->IsA(type))
  {
    return this;
  }

  for (auto child : this->Children)
  {
    if (child->IsA(type))
    {
      return child;
    }
  }
  return nullptr;
}
VTK_ABI_NAMESPACE_END
