/*=========================================================================

  Program:   ParaView
  Module:    vtkSelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelection.h"

#include "vtkAbstractArray.h"
#include "vtkFieldData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <vector>

using namespace std;


//----------------------------------------------------------------------------
struct vtkSelectionInternals
{
  std::vector<vtkSmartPointer<vtkSelectionNode> > Nodes;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSelection);

//----------------------------------------------------------------------------
vtkSelection::vtkSelection()
{
  this->Internal = new vtkSelectionInternals;
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
}

//----------------------------------------------------------------------------
vtkSelection::~vtkSelection()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSelection::Initialize()
{
  this->Superclass::Initialize();
  delete this->Internal;
  this->Internal = new vtkSelectionInternals;
}

//----------------------------------------------------------------------------
unsigned int vtkSelection::GetNumberOfNodes()
{
  return static_cast<unsigned int>(this->Internal->Nodes.size());
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkSelection::GetNode(unsigned int idx)
{
  if (idx >= this->GetNumberOfNodes())
  {
    return 0;
  }
  return this->Internal->Nodes[idx];
}

//----------------------------------------------------------------------------
void vtkSelection::AddNode(vtkSelectionNode* node)
{
  if (!node)
  {
    return;
  }
  // Make sure that node is not already added
  unsigned int numNodes = this->GetNumberOfNodes();
  for (unsigned int i = 0; i < numNodes; i++)
  {
    if (this->GetNode(i) == node)
    {
      return;
    }
  }
  this->Internal->Nodes.push_back(node);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveNode(unsigned int idx)
{
  if (idx >= this->GetNumberOfNodes())
  {
    return;
  }
  std::vector<vtkSmartPointer<vtkSelectionNode> >::iterator iter =
    this->Internal->Nodes.begin();
  this->Internal->Nodes.erase(iter+idx);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveNode(vtkSelectionNode* node)
{
  if (!node)
  {
    return;
  }
  unsigned int numNodes = this->GetNumberOfNodes();
  for (unsigned int i = 0; i < numNodes; i++)
  {
    if (this->GetNode(i) == node)
    {
      this->RemoveNode(i);
      return;
    }
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveAllNodes()
{
  this->Internal->Nodes.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  unsigned int numNodes = this->GetNumberOfNodes();
  os << indent << "Number of nodes: " << numNodes << endl;
  os << indent << "Nodes: " << endl;
  for (unsigned int i = 0; i < numNodes; i++)
  {
    os << indent << "Node #" << i << endl;
    this->GetNode(i)->PrintSelf(os, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
void vtkSelection::ShallowCopy(vtkDataObject* src)
{
  vtkSelection *input = vtkSelection::SafeDownCast(src);
  if (!input)
  {
    return;
  }
  this->Initialize();
  this->Superclass::ShallowCopy(src);
  unsigned int numNodes = input->GetNumberOfNodes();
  for (unsigned int i=0; i<numNodes; i++)
  {
    vtkSmartPointer<vtkSelectionNode> newNode =
      vtkSmartPointer<vtkSelectionNode>::New();
    newNode->ShallowCopy(input->GetNode(i));
    this->AddNode(newNode);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::DeepCopy(vtkDataObject* src)
{
  vtkSelection *input = vtkSelection::SafeDownCast(src);
  if (!input)
  {
    return;
  }
  this->Initialize();
  this->Superclass::DeepCopy(src);
  unsigned int numNodes = input->GetNumberOfNodes();
  for (unsigned int i=0; i<numNodes; i++)
  {
    vtkSmartPointer<vtkSelectionNode> newNode =
      vtkSmartPointer<vtkSelectionNode>::New();
    newNode->DeepCopy(input->GetNode(i));
    this->AddNode(newNode);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::Union(vtkSelection* s)
{
  for (unsigned int n = 0; n < s->GetNumberOfNodes(); ++n)
  {
    this->Union(s->GetNode(n));
  }
}

//----------------------------------------------------------------------------
void vtkSelection::Union(vtkSelectionNode* node)
{
  bool merged = false;
  for (unsigned int tn = 0; tn < this->GetNumberOfNodes(); ++tn)
  {
    vtkSelectionNode* tnode = this->GetNode(tn);
    if (tnode->EqualProperties(node))
    {
      tnode->UnionSelectionList(node);
      merged = true;
      break;
    }
  }
  if (!merged)
  {
    vtkSmartPointer<vtkSelectionNode> clone =
      vtkSmartPointer<vtkSelectionNode>::New();
    clone->DeepCopy(node);
    this->AddNode(clone);
  }
}

//----------------------------------------------------------------------------
void vtkSelection::Subtract(vtkSelection* s)
{
  for(unsigned int n=0; n<s->GetNumberOfNodes(); ++n)
  {
    this->Subtract(s->GetNode(n));
  }
}

//----------------------------------------------------------------------------
void vtkSelection::Subtract(vtkSelectionNode* node)
{
  bool subtracted = false;
  for( unsigned int tn = 0; tn<this->GetNumberOfNodes(); ++tn)
  {
    vtkSelectionNode* tnode = this->GetNode(tn);

    if(tnode->EqualProperties(node))
    {
      tnode->SubtractSelectionList(node);
      subtracted = true;
    }
  }
  if( !subtracted )
  {
    vtkErrorMacro("Could not subtract selections");
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSelection::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType nodeMTime;
  for (unsigned int n = 0; n < this->GetNumberOfNodes(); ++n)
  {
    vtkSelectionNode* node = this->GetNode(n);
    nodeMTime = node->GetMTime();
    mTime = ( nodeMTime > mTime ? nodeMTime : mTime );
  }
  return mTime;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetData(vtkInformation* info)
{
  return info? vtkSelection::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetData(vtkInformationVector* v, int i)
{
  return vtkSelection::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkSelection::Dump()
{
  this->Dump(cout);
}

//----------------------------------------------------------------------------
void vtkSelection::Dump(ostream& os)
{
  vtkSmartPointer<vtkTable> tmpTable = vtkSmartPointer<vtkTable>::New();
  cerr << "==Selection==" << endl;
  for (unsigned int i = 0; i < this->GetNumberOfNodes(); ++i)
  {
    os << "===Node " << i << "===" << endl;
    vtkSelectionNode* node = this->GetNode(i);
    os << "ContentType: ";
    switch (node->GetContentType())
    {
      case vtkSelectionNode::GLOBALIDS:
        os << "GLOBALIDS";
        break;
      case vtkSelectionNode::PEDIGREEIDS:
        os << "PEDIGREEIDS";
        break;
      case vtkSelectionNode::VALUES:
        os << "VALUES";
        break;
      case vtkSelectionNode::INDICES:
        os << "INDICES";
        break;
      case vtkSelectionNode::FRUSTUM:
        os << "FRUSTUM";
        break;
      case vtkSelectionNode::LOCATIONS:
        os << "LOCATIONS";
        break;
      case vtkSelectionNode::THRESHOLDS:
        os << "THRESHOLDS";
        break;
      case vtkSelectionNode::BLOCKS:
        os << "BLOCKS";
        break;
      default:
        os << "UNKNOWN";
        break;
    }
    os << endl;
    os << "FieldType: ";
    switch (node->GetFieldType())
    {
      case vtkSelectionNode::CELL:
        os << "CELL";
        break;
      case vtkSelectionNode::POINT:
        os << "POINT";
        break;
      case vtkSelectionNode::FIELD:
        os << "FIELD";
        break;
      case vtkSelectionNode::VERTEX:
        os << "VERTEX";
        break;
      case vtkSelectionNode::EDGE:
        os << "EDGE";
        break;
      case vtkSelectionNode::ROW:
        os << "ROW";
        break;
      default:
        os << "UNKNOWN";
        break;
    }
    os << endl;
    if (node->GetSelectionData())
    {
      tmpTable->SetRowData(node->GetSelectionData());
      tmpTable->Dump(10);
    }
  }
}
