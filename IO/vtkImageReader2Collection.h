/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader2Collection.h
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
// .NAME vtkImageReader2Collection - maintain a list of implicit functions
// .SECTION Description
// vtkImageReader2Collection is an object that creates and manipulates
// lists of objects of type vtkImplicitFunction. 
// .SECTION See Also
// vtkCollection vtkPlaneCollection

#ifndef __vtkImageReader2Collection_h
#define __vtkImageReader2Collection_h

#include "vtkCollection.h"
#include "vtkImageReader2.h"

class VTK_IO_EXPORT vtkImageReader2Collection : public vtkCollection
{
public:
  vtkTypeRevisionMacro(vtkImageReader2Collection,vtkCollection);
  static vtkImageReader2Collection *New();

  // Description:
  // Add an implicit function to the list.
  void AddItem(vtkImageReader2 *);

  // Description:
  // Get the next implicit function in the list.
  vtkImageReader2 *GetNextItem();
  
protected:
  vtkImageReader2Collection() {};
  ~vtkImageReader2Collection() {};
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkImageReader2Collection(const vtkImageReader2Collection&);  // Not implemented.
  void operator=(const vtkImageReader2Collection&);  // Not implemented.
};

inline void vtkImageReader2Collection::AddItem(vtkImageReader2 *f) 
{
  this->vtkCollection::AddItem((vtkObject *)f);
}

inline vtkImageReader2 *vtkImageReader2Collection::GetNextItem() 
{ 
  return static_cast<vtkImageReader2*>(this->GetNextItemAsObject());
}

#endif
