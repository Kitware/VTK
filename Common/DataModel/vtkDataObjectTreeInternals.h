// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataObjectTreeInternals
 *
 */

#ifndef vtkDataObjectTreeInternals_h
#define vtkDataObjectTreeInternals_h

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkSmartPointer.h"

#include <vector>

//-----------------------------------------------------------------------------
// Item in the VectorOfDataObjects.
VTK_ABI_NAMESPACE_BEGIN
struct vtkDataObjectTreeItem
{
  vtkSmartPointer<vtkDataObject> DataObject;
  vtkSmartPointer<vtkInformation> MetaData;

  vtkDataObjectTreeItem(vtkDataObject* dobj = nullptr, vtkInformation* info = nullptr)
  {
    this->DataObject = dobj;
    this->MetaData = info;
  }
};

//-----------------------------------------------------------------------------
class vtkDataObjectTreeInternals
{
public:
  typedef std::vector<vtkDataObjectTreeItem> VectorOfDataObjects;
  typedef VectorOfDataObjects::iterator Iterator;
  typedef VectorOfDataObjects::reverse_iterator ReverseIterator;

  VectorOfDataObjects Children;
};

//-----------------------------------------------------------------------------
class vtkDataObjectTreeIndex : public std::vector<unsigned int>
{
  int IsValid() { return !this->empty(); }
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkDataObjectTreeInternals.h
