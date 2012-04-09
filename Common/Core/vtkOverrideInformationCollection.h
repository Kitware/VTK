/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverrideInformationCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOverrideInformationCollection - maintain a list of override information objects
// .SECTION Description
// vtkOverrideInformationCollection is an object that creates and manipulates
// lists of objects of type vtkOverrideInformation. 
// .SECTION See Also
// vtkCollection

#ifndef __vtkOverrideInformationCollection_h
#define __vtkOverrideInformationCollection_h

#include "vtkCollection.h"

#include "vtkOverrideInformation.h" // Needed for inline methods

class VTK_COMMON_EXPORT vtkOverrideInformationCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkOverrideInformationCollection,vtkCollection);
  static vtkOverrideInformationCollection *New();

  // Description:
  // Add a OverrideInformation to the list.
  void AddItem(vtkOverrideInformation *);

  // Description:
  // Get the next OverrideInformation in the list.
  vtkOverrideInformation *GetNextItem();
  
  //BTX
  // Description: 
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth. 
  vtkOverrideInformation *GetNextOverrideInformation(
    vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkOverrideInformation *>(
      this->GetNextItemAsObject(cookie));};
  //ETX

protected:
  vtkOverrideInformationCollection() {};
  ~vtkOverrideInformationCollection() {};
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkOverrideInformationCollection(const vtkOverrideInformationCollection&);  // Not implemented.
  void operator=(const vtkOverrideInformationCollection&);  // Not implemented.
};

inline void vtkOverrideInformationCollection::AddItem(vtkOverrideInformation *f) 
{
  this->vtkCollection::AddItem(f);
}

inline vtkOverrideInformation *vtkOverrideInformationCollection::GetNextItem() 
{ 
  return static_cast<vtkOverrideInformation *>(this->GetNextItemAsObject());
}

#endif
