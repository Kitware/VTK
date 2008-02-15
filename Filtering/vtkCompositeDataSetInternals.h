/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSetInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataSetInternals 
// .SECTION Description

#ifndef __vtkCompositeDataSetInternals_h
#define __vtkCompositeDataSetInternals_h

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

//-----------------------------------------------------------------------------
// Item in the VectorOfDataObjects.
struct vtkCompositeDataSetItem
{
  vtkSmartPointer<vtkDataObject> DataObject;
  vtkSmartPointer<vtkInformation> MetaData;

  vtkCompositeDataSetItem(vtkDataObject* dobj =0, vtkInformation* info=0)
    {
    this->DataObject = dobj;
    this->MetaData = info;
    }
};

//-----------------------------------------------------------------------------
class vtkCompositeDataSetInternals
{
public:
  typedef vtkstd::vector<vtkCompositeDataSetItem> VectorOfDataObjects;
  typedef VectorOfDataObjects::iterator Iterator;
  typedef VectorOfDataObjects::reverse_iterator ReverseIterator;

  VectorOfDataObjects Children;
};


//-----------------------------------------------------------------------------
class vtkCompositeDataSetIndex : public vtkstd::vector<unsigned int>
{
  int IsValid()
    {
    return (this->size()> 0);
    }
};

#endif


