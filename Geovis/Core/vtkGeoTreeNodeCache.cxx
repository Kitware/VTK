/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTreeNodeCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkGeoTreeNodeCache.h"

#include "vtkGeoTreeNode.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkGeoTreeNodeCache);
//----------------------------------------------------------------------------
vtkGeoTreeNodeCache::vtkGeoTreeNodeCache()
{
  this->Oldest = 0;
  this->Newest = 0;
  this->Size = 0;
  this->CacheMaximumLimit = 500;
  this->CacheMinimumLimit = 250;
}

//----------------------------------------------------------------------------
vtkGeoTreeNodeCache::~vtkGeoTreeNodeCache()
{
  // Break reference loops by explicitly setting all prev/next pointers to null.
  vtkGeoTreeNode* cur;
  for (cur = this->Newest; cur; cur = cur->GetOlder())
    {
    cur->SetOlder(0);
    cur->SetNewer(0);
    }
}

//----------------------------------------------------------------------------
void vtkGeoTreeNodeCache::SendToFront(vtkGeoTreeNode* node)
{
  if (node == this->Newest)
    {
    return;
    }

  // Remove from the list if in the list already
  this->RemoveNode(node);

  // Add to the beginning of the list
  if (this->Size > 0)
    {
    node->SetNewer(0);
    node->SetOlder(this->Newest);
    this->Newest->SetNewer(node);
    this->Newest = node;
    }
  else
    {
    node->SetNewer(0);
    node->SetOlder(0);
    this->Newest = node;
    this->Oldest = node;
    }
  this->Size++;
  if (this->Size > this->CacheMaximumLimit)
    {
    this->TrimToCacheMinimum();
    }
}

//----------------------------------------------------------------------------
void vtkGeoTreeNodeCache::TrimToCacheMinimum()
{
  while (this->Size > this->CacheMinimumLimit)
    {
    vtkGeoTreeNode* node = this->Oldest;
    node->GetNewer()->SetOlder(0);
    this->Oldest = node->GetNewer();
    node->SetOlder(0);
    node->SetNewer(0);

    // If this was the last of a set of siblings to leave the list,
    // delete data from all siblings.
    this->DeleteDataFromSiblings(node);

    this->Size--;
    }
}

//----------------------------------------------------------------------------
void vtkGeoTreeNodeCache::DeleteDataFromSiblings(vtkGeoTreeNode* node)
{
  // Delete data from node or siblings if possible.
  vtkGeoTreeNode* parent = node->GetParentTreeNode();
  if (!parent)
    {
    return;
    }
  bool canDeleteSiblings = true;
  for (int c = 0; c < 4; ++c)
    {
    vtkGeoTreeNode* child = parent->GetChildTreeNode(c);
    if (!child || child->GetOlder() || child->GetNewer() || child == this->Newest)
      {
      canDeleteSiblings = false;
      break;
      }
    }
  if (canDeleteSiblings)
    {
    for (int c = 0; c < 4; ++c)
      {
      vtkGeoTreeNode* child = parent->GetChildTreeNode(c);
      child->DeleteData();
      }
    }
}

//----------------------------------------------------------------------------
void vtkGeoTreeNodeCache::RemoveNode(vtkGeoTreeNode* node)
{
  if (!node->GetNewer() && !node->GetOlder() && node != this->Newest)
    {
    // The node is not in the list
    return;
    }

  if (!node->GetNewer())
    {
    this->Newest = node->GetOlder();
    }
  else
    {
    node->GetNewer()->SetOlder(node->GetOlder());
    }
  if (!node->GetOlder())
    {
    this->Oldest = node->GetNewer();
    }
  else
    {
    node->GetOlder()->SetNewer(node->GetNewer());
    }
  node->SetOlder(0);
  node->SetNewer(0);
  this->Size--;
}

//----------------------------------------------------------------------------
void vtkGeoTreeNodeCache::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "CacheMinimumLimit: " << this->CacheMinimumLimit << endl;
  os << indent << "CacheMaximumLimit: " << this->CacheMaximumLimit << endl;
  os << indent << "Size: " << this->Size << endl;
}
