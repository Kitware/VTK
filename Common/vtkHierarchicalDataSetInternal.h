/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSetInternal.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkHierarchicalDataSetInternal_h
#define __vtkHierarchicalDataSetInternal_h

#include <vtkstd/vector>
#include <vtkstd/algorithm>

#include "vtkDataObject.h"
#include "vtkSmartPointer.h"

class vtkHDSNode;

struct vtkHierarchicalDataSetInternal
{
  typedef vtkstd::vector<vtkHDSNode*> LevelDataSetsType;
  typedef LevelDataSetsType::iterator LevelDataSetsIterator;
  typedef vtkstd::vector<LevelDataSetsType> DataSetsType;
  typedef DataSetsType::iterator DataSetsIterator;

  DataSetsType DataSets;
};

struct vtkHDSNodeRef
{
  vtkHDSNodeRef(int level, int index) : Level(level), Index(index) {}

  vtkstd_bool operator==(const vtkHDSNodeRef& rhs)
    {
      return (this->Level == rhs.Level) && (this->Index && rhs.Index);
    }

  int Level;
  int Index;
};

class vtkHDSNode
{
public:

  vtkHDSNode() : DataSet(0) {}
  vtkSmartPointer<vtkDataObject> DataSet;

  void AddParent(const vtkHDSNodeRef& parent);
  void AddChild (const vtkHDSNodeRef& child );

  void RemoveParent(const vtkHDSNodeRef& parent);
  void RemoveChild (const vtkHDSNodeRef& child );

  void DisconnectFromParent(const vtkHDSNodeRef& self,
                            const vtkHDSNodeRef& parent,
                            vtkHierarchicalDataSetInternal::DataSetsType& ds);
  void DisconnectFromChild (const vtkHDSNodeRef& self,
                            const vtkHDSNodeRef& child,
                            vtkHierarchicalDataSetInternal::DataSetsType& ds);
  void ConnectToParent(const vtkHDSNodeRef& self,
                       const vtkHDSNodeRef& parent,
                       vtkHierarchicalDataSetInternal::DataSetsType& ds);
  void ConnectToChild (const vtkHDSNodeRef& self,
                       const vtkHDSNodeRef& child,
                       vtkHierarchicalDataSetInternal::DataSetsType& ds);

  void DisconnectAll(const vtkHDSNodeRef& self,
                     vtkHierarchicalDataSetInternal::DataSetsType& ds);

protected:
  vtkstd::vector<vtkHDSNodeRef> Parents;
  vtkstd::vector<vtkHDSNodeRef> Children;
};

inline void vtkHDSNode::AddParent(const vtkHDSNodeRef& parent)
{
  this->Parents.push_back(parent);
}

inline void vtkHDSNode::AddChild(const vtkHDSNodeRef& child)
{
  this->Children.push_back(child);
}

inline void vtkHDSNode::RemoveParent(const vtkHDSNodeRef& parent)
{
  vtkstd::vector<vtkHDSNodeRef>::iterator it = 
    vtkstd::find(this->Parents.begin(), this->Parents.end(), parent);
  if (it != this->Parents.end())
    {
    this->Parents.erase(it);
    }
}

inline void vtkHDSNode::RemoveChild (const vtkHDSNodeRef& child)
{
  vtkstd::vector<vtkHDSNodeRef>::iterator it = 
    vtkstd::find(this->Children.begin(), this->Children.end(), child);
  if (it != this->Children.end())
    {
    this->Children.erase(it);
    }
}

inline void vtkHDSNode::ConnectToParent(
  const vtkHDSNodeRef& self,
  const vtkHDSNodeRef& parent,
  vtkHierarchicalDataSetInternal::DataSetsType& ds)
{
  this->AddParent(parent);
  ds[parent.Level][parent.Index]->AddChild(self);
}

inline void vtkHDSNode::ConnectToChild(
  const vtkHDSNodeRef& self,
  const vtkHDSNodeRef& child,
  vtkHierarchicalDataSetInternal::DataSetsType& ds)
{
  this->AddChild(child);
  ds[child.Level][child.Index]->AddParent(self);
}

inline void vtkHDSNode::DisconnectFromParent(
  const vtkHDSNodeRef& self,
  const vtkHDSNodeRef& parent, 
  vtkHierarchicalDataSetInternal::DataSetsType& ds)
{
  this->RemoveParent(parent);
  ds[parent.Level][parent.Index]->RemoveChild(self);
}

inline void vtkHDSNode::DisconnectFromChild(
  const vtkHDSNodeRef& self,
  const vtkHDSNodeRef& child, 
  vtkHierarchicalDataSetInternal::DataSetsType& ds)
{
  this->RemoveChild(child);
  ds[child.Level][child.Index]->RemoveParent(self);
}

inline void vtkHDSNode::DisconnectAll(
  const vtkHDSNodeRef& self, vtkHierarchicalDataSetInternal::DataSetsType& ds)
{
  vtkstd::vector<vtkHDSNodeRef>::iterator it;

  for(it=this->Parents.begin(); it!=this->Parents.end(); ++it)
    {
    this->DisconnectFromParent(self, *it, ds);
    }

  for(it=this->Children.begin(); it!=this->Children.end(); ++it)
    {
    this->DisconnectFromChild(self, *it, ds);
    }

}

#endif
