/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayCollection.h
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
// .NAME vtkDataArrayCollection - maintain an unordered list of dataarray objects
// .SECTION Description
// vtkDataArrayCollection is an object that creates and manipulates lists of
// datasets. See also vtkCollection and subclasses.

#ifndef __vtkDataArrayCollection_h
#define __vtkDataArrayCollection_h

#include "vtkCollection.h"
#include "vtkDataArray.h"

class VTK_COMMON_EXPORT vtkDataArrayCollection : public vtkCollection
{
public:
  static vtkDataArrayCollection *New();
  vtkTypeRevisionMacro(vtkDataArrayCollection,vtkCollection);

  // Description:
  // Add a dataset to the list.
  void AddItem(vtkDataArray *ds) {
    this->vtkCollection::AddItem((vtkObject *)ds);};
  
  // Description:
  // Get the next dataset in the list.
  vtkDataArray *GetNextItem() { 
    return static_cast<vtkDataArray *>(this->GetNextItemAsObject());};

  // Description:
  // Get the ith dataset in the list.
  vtkDataArray *GetItem(int i) { 
    return static_cast<vtkDataArray *>(this->GetItemAsObject(i));};
  
protected:
  vtkDataArrayCollection() {};
  ~vtkDataArrayCollection() {};


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkDataArrayCollection(const vtkDataArrayCollection&);  // Not implemented.
  void operator=(const vtkDataArrayCollection&);  // Not implemented.
};


#endif
