/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformCollection.h
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
// .NAME vtkTransformCollection - maintain a list of transforms

// .SECTION Description
// vtkTransformCollection is an object that creates and manipulates lists of
// objects of type vtkTransform.

// .SECTION see also
// vtkCollection vtkTransform

#ifndef __vtkTransformCollection_h
#define __vtkTransformCollection_h

#include "vtkCollection.h"
#include "vtkTransform.h"

class VTK_COMMON_EXPORT vtkTransformCollection : public vtkCollection
{
public:
  vtkTypeRevisionMacro(vtkTransformCollection,vtkCollection);
  static vtkTransformCollection *New();

  // Description:
  // Add a Transform to the list.
  void AddItem(vtkTransform *);

  // Description:
  // Get the next Transform in the list. Return NULL when the end of the
  // list is reached.
  vtkTransform *GetNextItem();

protected:
  vtkTransformCollection() {};
  ~vtkTransformCollection() {};


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkTransformCollection(const vtkTransformCollection&);  // Not implemented.
  void operator=(const vtkTransformCollection&);  // Not implemented.
};

inline void vtkTransformCollection::AddItem(vtkTransform *t) 
{
  this->vtkCollection::AddItem((vtkObject *)t);
}

inline vtkTransform *vtkTransformCollection::GetNextItem() 
{ 
  return static_cast<vtkTransform *>(this->GetNextItemAsObject());
}

#endif
