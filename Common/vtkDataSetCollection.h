/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetCollection.h
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
// .NAME vtkDataSetCollection - maintain an unordered list of dataset objects
// .SECTION Description
// vtkDataSetCollection is an object that creates and manipulates lists of
// datasets. See also vtkCollection and subclasses.

#ifndef __vtkDataSetCollection_h
#define __vtkDataSetCollection_h

#include "vtkCollection.h"
#include "vtkDataSet.h"

class VTK_COMMON_EXPORT vtkDataSetCollection : public vtkCollection
{
public:
  static vtkDataSetCollection *New();
  vtkTypeRevisionMacro(vtkDataSetCollection,vtkCollection);

  // Description:
  // Add a dataset to the list.
  void AddItem(vtkDataSet *ds) {
    this->vtkCollection::AddItem((vtkObject *)ds);};
  
  // Description:
  // Get the next dataset in the list.
  vtkDataSet *GetNextItem() { 
    return static_cast<vtkDataSet *>(this->GetNextItemAsObject());};

  // Description:
  // Get the ith dataset in the list.
  vtkDataSet *GetItem(int i) { 
    return static_cast<vtkDataSet *>(this->GetItemAsObject(i));};
  
protected:
  vtkDataSetCollection() {};
  ~vtkDataSetCollection() {};


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkDataSetCollection(const vtkDataSetCollection&);  // Not implemented.
  void operator=(const vtkDataSetCollection&);  // Not implemented.
};


#endif
