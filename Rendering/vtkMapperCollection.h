/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapperCollection.h
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
// .NAME vtkMapperCollection - a list of mappers
// .SECTION Description
// vtkMapperCollection represents and provides methods to manipulate a list of
// mappers (i.e., vtkMapper and subclasses). The list is unsorted and duplicate
// entries are not prevented.

// .SECTION see also
// vtkMapper vtkCollection 

#ifndef __vtkMapperC_h
#define __vtkMapperC_h

#include "vtkCollection.h"
#include "vtkMapper.h"

class VTK_RENDERING_EXPORT vtkMapperCollection : public vtkCollection
{
 public:
  static vtkMapperCollection *New();
  vtkTypeRevisionMacro(vtkMapperCollection,vtkCollection);

  // Description:
  // Add an mapper to the list.
  void AddItem(vtkMapper *a) {
    this->vtkCollection::AddItem(static_cast<vtkObject *>(a));};
  
  // Description:
  // Get the next mapper in the list.
  vtkMapper *GetNextItem() { 
    return static_cast<vtkMapper *>(this->GetNextItemAsObject());};
  
  // Description:
  // Get the last mapper in the list.
  vtkMapper *GetLastItem();
  
protected:  
  vtkMapperCollection() {};
  ~vtkMapperCollection() {};

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkMapperCollection(const vtkMapperCollection&);  // Not implemented.
  void operator=(const vtkMapperCollection&);  // Not implemented.
};


inline vtkMapper *vtkMapperCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkMapper *>(this->Bottom->Item);
    }
}

#endif





