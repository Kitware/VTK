/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverrideInformationCollection.h
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
// .NAME vtkOverrideInformationCollection - maintain a list of override information objects
// .SECTION Description
// vtkOverrideInformationCollection is an object that creates and manipulates
// lists of objects of type vtkOverrideInformation. 
// .SECTION See Also
// vtkCollection

#ifndef __vtkOverrideInformationCollection_h
#define __vtkOverrideInformationCollection_h

#include "vtkCollection.h"
#include "vtkOverrideInformation.h"

class VTK_COMMON_EXPORT vtkOverrideInformationCollection : public vtkCollection
{
public:
  vtkTypeRevisionMacro(vtkOverrideInformationCollection,vtkCollection);
  static vtkOverrideInformationCollection *New();

  // Description:
  // Add a OverrideInformation to the list.
  void AddItem(vtkOverrideInformation *);

  // Description:
  // Get the next OverrideInformation in the list.
  vtkOverrideInformation *GetNextItem();
  
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
  this->vtkCollection::AddItem((vtkObject *)f);
}

inline vtkOverrideInformation *vtkOverrideInformationCollection::GetNextItem() 
{ 
  return static_cast<vtkOverrideInformation *>(this->GetNextItemAsObject());
}

#endif
