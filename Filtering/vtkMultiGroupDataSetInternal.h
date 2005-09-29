/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataSetInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataSetInternal - internal structure for vtkMultiGroupDataSet
// .SECTION Description
// vtkMultiGroupDataSetInternal is used in internal implementation of
// vtkMultiGroupDataSet. This should only be accessed by friends
// and sub-classes of that class.

#ifndef __vtkMultiGroupDataSetInternal_h
#define __vtkMultiGroupDataSetInternal_h

#include <vtkstd/vector>
#include <vtkstd/algorithm>

#include "vtkDataObject.h"
#include "vtkSmartPointer.h"

class vtkMGDSNode;

struct vtkMultiGroupDataSetInternal
{
  typedef vtkstd::vector<vtkSmartPointer<vtkDataObject> > GroupDataSetsType;
  typedef GroupDataSetsType::iterator GroupDataSetsIterator;
  typedef vtkstd::vector<GroupDataSetsType> DataSetsType;
  typedef DataSetsType::iterator DataSetsIterator;

  DataSetsType DataSets;
};

struct vtkMGDSNodeRef
{
  vtkMGDSNodeRef(int group, int index) : Group(group), Index(index) {}

  vtkstd_bool operator==(const vtkMGDSNodeRef& rhs)
    {
      return (this->Group == rhs.Group) && (this->Index == rhs.Index);
    }
  vtkstd_bool operator!=(const vtkMGDSNodeRef& rhs)
    {
      return (this->Group != rhs.Group) || (this->Index != rhs.Index);
    }
  
  int Group;
  int Index;
};

class vtkMGDSNode
{
public:

protected:
  vtkstd::vector<vtkMGDSNodeRef> Parents;
  vtkstd::vector<vtkMGDSNodeRef> Children;
};

#endif
