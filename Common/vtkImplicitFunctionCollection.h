/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionCollection.h
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
// .NAME vtkImplicitFunctionCollection - maintain a list of implicit functions
// .SECTION Description
// vtkImplicitFunctionCollection is an object that creates and manipulates
// lists of objects of type vtkImplicitFunction. 
// .SECTION See Also
// vtkCollection vtkPlaneCollection

#ifndef __vtkImplicitFunctionCollection_h
#define __vtkImplicitFunctionCollection_h

#include "vtkCollection.h"
#include "vtkImplicitFunction.h"

class VTK_COMMON_EXPORT vtkImplicitFunctionCollection : public vtkCollection
{
public:
  vtkTypeRevisionMacro(vtkImplicitFunctionCollection,vtkCollection);
  static vtkImplicitFunctionCollection *New();

  // Description:
  // Add an implicit function to the list.
  void AddItem(vtkImplicitFunction *);

  // Description:
  // Get the next implicit function in the list.
  vtkImplicitFunction *GetNextItem();
  
protected:
  vtkImplicitFunctionCollection() {};
  ~vtkImplicitFunctionCollection() {};
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkImplicitFunctionCollection(const vtkImplicitFunctionCollection&);  // Not implemented.
  void operator=(const vtkImplicitFunctionCollection&);  // Not implemented.
};

inline void vtkImplicitFunctionCollection::AddItem(vtkImplicitFunction *f) 
{
  this->vtkCollection::AddItem((vtkObject *)f);
}

inline vtkImplicitFunction *vtkImplicitFunctionCollection::GetNextItem() 
{ 
  return static_cast<vtkImplicitFunction *>(this->GetNextItemAsObject());
}

#endif
