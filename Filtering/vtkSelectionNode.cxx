/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionNode.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectionNode.h"

#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSelectionNode, "1.1");
vtkStandardNewMacro(vtkSelectionNode);

vtkCxxSetObjectMacro(vtkSelectionNode, SelectionList, vtkDataArray);

vtkInformationKeyMacro(vtkSelectionNode,CONTENT_TYPE,Integer);
vtkInformationKeyMacro(vtkSelectionNode,SOURCE,ObjectBase);
vtkInformationKeyMacro(vtkSelectionNode,SOURCE_ID,Integer);
vtkInformationKeyMacro(vtkSelectionNode,PROP,ObjectBase);
vtkInformationKeyMacro(vtkSelectionNode,PROP_ID,Integer);
vtkInformationKeyMacro(vtkSelectionNode,PROCESS_ID,Integer);
vtkInformationKeyMacro(vtkSelectionNode,GROUP,Integer);
vtkInformationKeyMacro(vtkSelectionNode,BLOCK,Integer);

struct vtkSelectionNodeInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSelectionNode> > Children;
};


//----------------------------------------------------------------------------
vtkSelectionNode::vtkSelectionNode()
{
  this->Internal = new vtkSelectionNodeInternals;
  this->SelectionList = 0;
  this->ParentNode = 0;
  this->Properties = vtkInformation::New();
}

//----------------------------------------------------------------------------
vtkSelectionNode::~vtkSelectionNode()
{
  delete this->Internal;
  if (this->SelectionList)
    {
    this->SelectionList->Delete();
    }
  this->ParentNode = 0;
  this->Properties->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectionNode::Clear()
{
  delete this->Internal;
  this->Internal = new vtkSelectionNodeInternals;
  if (this->SelectionList)
    {
    this->SelectionList->Delete();
    }
  this->SelectionList = 0;
  this->Properties->Clear();
}

//----------------------------------------------------------------------------
unsigned int vtkSelectionNode::GetNumberOfChildren()
{
  return this->Internal->Children.size();
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkSelectionNode::GetChild(unsigned int idx)
{
  if (idx >= this->GetNumberOfChildren())
    {
    return 0;
    }
  return this->Internal->Children[idx];
}

//----------------------------------------------------------------------------
void vtkSelectionNode::AddChild(vtkSelectionNode* child)
{
  if (!child)
    {
    return;
    }

  // Make sure that child is not already added
  unsigned int numChildren = this->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    if (this->GetChild(i) == child)
      {
      return;
      }
    }
  this->Internal->Children.push_back(child);
  child->ParentNode = this;
}

//----------------------------------------------------------------------------
void vtkSelectionNode::RemoveChild(unsigned int idx)
{
  if (idx >= this->GetNumberOfChildren())
    {
    return;
    }
  vtkstd::vector<vtkSmartPointer<vtkSelectionNode> >::iterator iter =
    this->Internal->Children.begin();
  iter->GetPointer()->ParentNode = 0;
  this->Internal->Children.erase(iter+idx);
}

//----------------------------------------------------------------------------
void vtkSelectionNode::RemoveChild(vtkSelectionNode* child)
{
  if (!child)
    {
    return;
    }

  unsigned int numChildren = this->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    if (this->GetChild(i) == child)
      {
      child->ParentNode = 0;
      this->RemoveChild(i);
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSelectionNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SelectionList:";
  if (this->SelectionList)
    {
    this->SelectionList->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "Properties:";
  if (this->Properties)
    {
    this->Properties->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ParentNode: ";
  if (this->ParentNode)
    {
    os << this->ParentNode;
    }
  else
    {
    os << "(none)";
    }
  os << endl;

  unsigned int numChildren = this->GetNumberOfChildren();
  os << indent << "Number of children: " << numChildren << endl;
  os << indent << "Children: " << endl;
  for (unsigned int i=0; i<numChildren; i++)
    {
    this->GetChild(i)->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkSelectionNode::DeepCopy(vtkSelectionNode* input)
{
  if (!input)
    {
    return;
    }

  this->Clear();

  this->Properties->Copy(input->Properties, 1);
  if (input->SelectionList)
    {
    this->SelectionList = input->SelectionList->NewInstance();
    this->SelectionList->DeepCopy(input->SelectionList);
    }

  unsigned int numChildren = input->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkSelectionNode* newChild = vtkSelectionNode::New();
    newChild->DeepCopy(input->GetChild(i));
    this->AddChild(newChild);
    newChild->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSelectionNode::CopyChildren(vtkSelectionNode* input)
{
  if (!this->Properties->Has(CONTENT_TYPE()) ||
      this->Properties->Get(CONTENT_TYPE()) != SELECTIONS)
    {
    return;
    }
  if (!input->Properties->Has(CONTENT_TYPE()) ||
      input->Properties->Get(CONTENT_TYPE()) != SELECTIONS)
    {
    return;
    }

  unsigned int numChildren = input->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkSelectionNode* child = input->GetChild(i);
    if (child->Properties->Has(CONTENT_TYPE()) && 
        child->Properties->Get(CONTENT_TYPE()) == SELECTIONS)
      {
      // TODO: Handle trees
      }
    else
      {
      vtkSelectionNode* newChild = vtkSelectionNode::New();
      newChild->DeepCopy(child);
      this->AddChild(newChild);
      newChild->Delete();
      }
    }
}
