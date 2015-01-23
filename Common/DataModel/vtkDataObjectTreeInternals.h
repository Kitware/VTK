/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectTreeInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataObjectTreeInternals
// .SECTION Description

#ifndef vtkDataObjectTreeInternals_h
#define vtkDataObjectTreeInternals_h

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkSmartPointer.h"

#include <vector>

//-----------------------------------------------------------------------------
// Item in the VectorOfDataObjects.
struct vtkDataObjectTreeItem
{
  vtkSmartPointer<vtkDataObject> DataObject;
  vtkSmartPointer<vtkInformation> MetaData;

  vtkDataObjectTreeItem(vtkDataObject* dobj =0, vtkInformation* info=0)
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
  int IsValid()
    {
    return (this->size()> 0);
    }
};

#endif


// VTK-HeaderTest-Exclude: vtkDataObjectTreeInternals.h
