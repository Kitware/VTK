/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DataSetC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataSetCollection - maintain an unordered list of dataset objects
// .SECTION Description
// vtkDataSetCollection is an object that creates and manipulates lists of
// datasets. See also vtkCollection and subclasses.

#ifndef __vtkDataSetCollection_hh
#define __vtkDataSetCollection_hh

#include "Collect.hh"
#include "DataSet.hh"

class vtkDataSetCollection : public vtkCollection
{
public:
  char *GetClassName() {return "vtkDataSetCollection";};

  void AddItem(vtkDataSet *);
  void RemoveItem(vtkDataSet *);
  int IsItemPresent(vtkDataSet *);
  vtkDataSet *GetNextItem();

};

// Description:
// Add an DataSet to the list.
inline void vtkDataSetCollection::AddItem(vtkDataSet *ds) 
{
  this->vtkCollection::AddItem((vtkObject *)ds);
}

// Description:
// Remove an DataSet from the list.
inline void vtkDataSetCollection::RemoveItem(vtkDataSet *ds) 
{
  this->vtkCollection::RemoveItem((vtkObject *)ds);
}

// Description:
// Determine whether a particular DataSet is present. Returns its position
// in the list.
inline int vtkDataSetCollection::IsItemPresent(vtkDataSet *ds) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)ds);
}

// Description:
// Get the next DataSet in the list.
inline vtkDataSet *vtkDataSetCollection::GetNextItem() 
{ 
  return (vtkDataSet *)(this->vtkCollection::GetNextItem());
}

#endif
