/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolyDatC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolyDataCollection - maintain a list of polygonal data objects
// .SECTION Description
// vtkPolyDataCollection is an object that creates and manipulates lists of
// datasets of type vtkPolyData. See also vtkDataSetCollection and vtkCollection
// and subclasses. 

#ifndef __vtkPolyDataCollection_hh
#define __vtkPolyDataCollection_hh

#include "Collect.hh"
#include "PolyData.hh"

class vtkPolyDataCollection : public vtkCollection
{
public:
  char *GetClassName() {return "vtkPolyDataCollection";};

  void AddItem(vtkPolyData *);
  void RemoveItem(vtkPolyData *);
  int IsItemPresent(vtkPolyData *);
  vtkPolyData *GetNextItem();
};

// Description:
// Add a poly data to the list.
inline void vtkPolyDataCollection::AddItem(vtkPolyData *pd) 
{
  this->vtkCollection::AddItem((vtkObject *)pd);
}

// Description:
// Remove an poly data from the list.
inline void vtkPolyDataCollection::RemoveItem(vtkPolyData *pd) 
{
  this->vtkCollection::RemoveItem((vtkObject *)pd);
}

// Description:
// Determine whether a particular poly data is present. Returns its position
// in the list.
inline int vtkPolyDataCollection::IsItemPresent(vtkPolyData *pd) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)pd);
}

// Description:
// Get the next poly data in the list.
inline vtkPolyData *vtkPolyDataCollection::GetNextItem() 
{ 
  return (vtkPolyData *)(this->vtkCollection::GetNextItem());
}

#endif
