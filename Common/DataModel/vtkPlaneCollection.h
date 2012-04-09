/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPlaneCollection - maintain a list of planes
// .SECTION Description
// vtkPlaneCollection is an object that creates and manipulates
// lists of objects of type vtkPlane. 
// .SECTION See Also
// vtkCollection

#ifndef __vtkPlaneCollection_h
#define __vtkPlaneCollection_h

#include "vtkCollection.h"

#include "vtkPlane.h" // Needed for inline methods

class VTK_COMMON_EXPORT vtkPlaneCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkPlaneCollection,vtkCollection);
  static vtkPlaneCollection *New();

  // Description:
  // Add a plane to the list.
  void AddItem(vtkPlane *);

  // Description:
  // Get the next plane in the list.
  vtkPlane *GetNextItem();

  // Description:
  // Get the ith plane in the list.
  vtkPlane *GetItem(int i) { 
    return static_cast<vtkPlane *>(this->GetItemAsObject(i));}; 

  //BTX
  // Description: 
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth. 
  vtkPlane *GetNextPlane(vtkCollectionSimpleIterator &cookie);
  //ETX

protected:
  vtkPlaneCollection() {};
  ~vtkPlaneCollection() {};
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkPlaneCollection(const vtkPlaneCollection&);  // Not implemented.
  void operator=(const vtkPlaneCollection&);  // Not implemented.
};

inline void vtkPlaneCollection::AddItem(vtkPlane *f) 
{
  this->vtkCollection::AddItem(f);
}

inline vtkPlane *vtkPlaneCollection::GetNextItem() 
{ 
 return static_cast<vtkPlane *>(this->GetNextItemAsObject());
}

#endif
