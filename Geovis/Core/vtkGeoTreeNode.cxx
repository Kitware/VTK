/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTreeNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=============================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkObjectFactory.h"
#include "vtkGeoTreeNode.h"

vtkStandardNewMacro(vtkGeoTreeNode);


//----------------------------------------------------------------------------
vtkGeoTreeNode::vtkGeoTreeNode()
{
  this->Level = 0;
  this->Parent = 0;
  this->Older = 0;
  this->Newer = 0;
  this->Id = 0; // make valgrind happy
  this->LatitudeRange[0]  = this->LatitudeRange[1]  = 0.;
  this->LongitudeRange[0] = this->LongitudeRange[1] = 0.;
  this->Status = NONE;
}

//-----------------------------------------------------------------------------
vtkGeoTreeNode::~vtkGeoTreeNode()
{
  this->SetParent(0);
}

//-----------------------------------------------------------------------------
void vtkGeoTreeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Level: " << this->Level << "\n";
  os << indent << "Parent: " << this->Parent << "\n";
  os << indent << "Older: " << this->Older << "\n";
  os << indent << "Newer: " << this->Newer << "\n";
  os << indent << "Id: " << this->Id << "\n";
  os << indent << "LatitudeRange: ["  << this->LatitudeRange[0]  << "," << this->LatitudeRange[1]  << "]\n";
  os << indent << "LongitudeRange: [" << this->LongitudeRange[0] << "," << this->LongitudeRange[1] << "]\n";
  os << indent << "Children:";
  for ( int i = 0; i < 4; ++ i )
  {
    os << " " << this->Children[i];
  }
  os << "\n";
}

//-----------------------------------------------------------------------------
void vtkGeoTreeNode::SetChild(vtkGeoTreeNode* node, int idx)
{
  if (idx < 0 || idx > 3)
  {
    vtkErrorMacro("Index out of range.");
    return;
  }
  this->Children[idx] = node;
}

//-----------------------------------------------------------------------------
int vtkGeoTreeNode::GetWhichChildAreYou()
{
  if (this->Level == 0)
  {
    vtkErrorMacro("Node does not have a parent.");
    return 0;
  }

  unsigned long id = this->Id;
  id = id >> (this->Level*2-1);
  id = id & 3;
  return id;
}

//-----------------------------------------------------------------------------
bool vtkGeoTreeNode::IsDescendantOf(vtkGeoTreeNode* elder)
{
  if (elder == 0)
  {
    return false;
  }
  if (this->Level <= elder->GetLevel())
  {
    return false;
  }
  // All descendants will have the same first N bits in their Id.
  int N = ((elder->GetLevel() * 2) + 1);
  unsigned long mask = (1 << N) - 1;
  if ((this->Id & mask) == elder->GetId())
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
int vtkGeoTreeNode::CreateChildren()
{
  // Create the four children.
  if (this->Children[0])
  { // This node is already has children.
    return VTK_OK;
  }
  int childLevel = this->GetLevel()+1;

  // Where the child index get coded in the node id.
  unsigned long idBit0 = 0;
  unsigned long idBit1 = 0;
  if (childLevel <= 15)
  {
    idBit0 = 1 << (2*childLevel - 1);
    idBit1 = 1 << (2*childLevel);
  }
  else
  {
    // if (childLevel > ((sizeof(unsigned long)*8) - 1) / 2)
    // this particular message gets printed too much and clutters the console...
    static bool msg_printed = false;
    if (!msg_printed)
    {
      vtkWarningMacro("Level too high to be encoded in node id. (this warning only emitted once)");
      msg_printed = true;
    }
  }
  unsigned long id = this->GetId();
  double longitudeRange[2];
  double latitudeRange[2];
  vtkGeoTreeNode* child;
  // Child 0
  this->GetLongitudeRange(longitudeRange);
  this->GetLatitudeRange(latitudeRange);
  double longitudeMid = 0.5 * (longitudeRange[0]+longitudeRange[1]);
  double latitudeMid = 0.5 * (latitudeRange[0]+latitudeRange[1]);
  // Child type is the same as parent.
  child = this->NewInstance();
  child->SetLevel(childLevel);
  child->SetId(id);
  longitudeRange[1] = longitudeMid;
  child->SetLongitudeRange(longitudeRange);
  latitudeRange[1] = latitudeMid;
  child->SetLatitudeRange(latitudeRange);
  this->SetChild(child, 0);
  child->SetParent(this);
  child->Delete();
  // Child 1
  this->GetLongitudeRange(longitudeRange);
  this->GetLatitudeRange(latitudeRange);
  // Child type is the same as parent.
  child = this->NewInstance();
  child->SetLevel(childLevel);
  child->SetId(id | idBit0);
  longitudeRange[0] = longitudeMid;
  child->SetLongitudeRange(longitudeRange);
  latitudeRange[1] = latitudeMid;
  child->SetLatitudeRange(latitudeRange);
  this->SetChild(child, 1);
  child->SetParent(this);
  child->Delete();
  // Child 2
  this->GetLongitudeRange(longitudeRange);
  this->GetLatitudeRange(latitudeRange);
  // Child type is the same as parent.
  child = this->NewInstance();
  child->SetLevel(childLevel);
  child->SetId(id | idBit1);
  longitudeRange[1] = longitudeMid;
  child->SetLongitudeRange(longitudeRange);
  latitudeRange[0] = latitudeMid;
  child->SetLatitudeRange(latitudeRange);
  this->SetChild(child, 2);
  child->SetParent(this);
  child->Delete();
  // Child 3
  this->GetLongitudeRange(longitudeRange);
  this->GetLatitudeRange(latitudeRange);
  // Child type is the same as parent.
  child = this->NewInstance();
  child->SetLevel(childLevel);
  child->SetId(id | idBit1 | idBit0);
  longitudeRange[0] = longitudeMid;
  child->SetLongitudeRange(longitudeRange);
  latitudeRange[0] = latitudeMid;
  child->SetLatitudeRange(latitudeRange);
  this->SetChild(child, 3);
  child->SetParent(this);
  child->Delete();
  //cerr << "creating child with range "
  //     << child->GetLongitudeRange()[0] << ","
  //     << child->GetLongitudeRange()[1] << ","
  //     << child->GetLatitudeRange()[0] << ","
  //     << child->GetLatitudeRange()[1] << endl;

  return VTK_OK;
}

vtkGeoTreeNode::NodeStatus vtkGeoTreeNode::GetStatus()
{
  return this->Status;
}

void vtkGeoTreeNode::SetStatus(NodeStatus status)
{
  this->Status = status;
}

void vtkGeoTreeNode::ShallowCopy(vtkGeoTreeNode *src)
{
  this->Level = src->Level;
  this->Id = src->Id;
  this->LongitudeRange[0] = src->LongitudeRange[0];
  this->LongitudeRange[1] = src->LongitudeRange[1];
  this->LatitudeRange[0] = src->LatitudeRange[0];
  this->LatitudeRange[1] = src->LatitudeRange[1];

  this->Children[0] = src->Children[0];
  this->Children[1] = src->Children[1];
  this->Children[2] = src->Children[2];
  this->Children[3] = src->Children[3];

  this->Status = src->Status;
  this->Parent = src->Parent;
}

void vtkGeoTreeNode::DeepCopy(vtkGeoTreeNode *src)
{
  this->Level = src->Level;
  this->Id = src->Id;
  this->LongitudeRange[0] = src->LongitudeRange[0];
  this->LongitudeRange[1] = src->LongitudeRange[1];
  this->LatitudeRange[0] = src->LatitudeRange[0];
  this->LatitudeRange[1] = src->LatitudeRange[1];

  this->Children[0] = src->Children[0];
  this->Children[1] = src->Children[1];
  this->Children[2] = src->Children[2];
  this->Children[3] = src->Children[3];

  this->Status = src->Status;
  this->Parent = src->Parent;
}
