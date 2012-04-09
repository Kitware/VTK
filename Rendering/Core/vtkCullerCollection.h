/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCullerCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCullerCollection - a list of Cullers
// .SECTION Description
// vtkCullerCollection represents and provides methods to manipulate a list
// of Cullers (i.e., vtkCuller and subclasses). The list is unsorted and
// duplicate entries are not prevented.

// .SECTION see also
// vtkCuller vtkCollection 

#ifndef __vtkCullerCollection_h
#define __vtkCullerCollection_h

#include "vtkCollection.h"
#include "vtkCuller.h" // for inline functions

class VTK_RENDERING_EXPORT vtkCullerCollection : public vtkCollection
{
 public:
  static vtkCullerCollection *New();
  vtkTypeMacro(vtkCullerCollection,vtkCollection);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an Culler to the list.
  void AddItem(vtkCuller *a)
    {
      this->vtkCollection::AddItem(a);
    }

  // Description:
  // Get the next Culler in the list.
  vtkCuller *GetNextItem()
    { 
      return static_cast<vtkCuller *>(this->GetNextItemAsObject());
    }
  
  // Description:
  // Get the last Culler in the list.
  vtkCuller *GetLastItem();
  
  //BTX
  // Description: 
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth. 
  vtkCuller *GetNextCuller(vtkCollectionSimpleIterator &cookie)
    {
      return static_cast<vtkCuller *>(this->GetNextItemAsObject(cookie));
    }
  //ETX

protected:
  vtkCullerCollection() {}
  ~vtkCullerCollection() {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
    {
      this->vtkCollection::AddItem(o);
    }

private:
  vtkCullerCollection(const vtkCullerCollection&);  // Not implemented.
  void operator=(const vtkCullerCollection&);  // Not implemented.
};


inline vtkCuller *vtkCullerCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkCuller *>(this->Bottom->Item);
    }
}

#endif





