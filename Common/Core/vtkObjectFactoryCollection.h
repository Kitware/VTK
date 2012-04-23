/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectFactoryCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkObjectFactoryCollection - maintain a list of object factories
// .SECTION Description
// vtkObjectFactoryCollection is an object that creates and manipulates lists
// of object of type vtkObjectFactory.

// .SECTION see also
// vtkCollection vtkObjectFactory

#ifndef __vtkObjectFactoryCollection_h
#define __vtkObjectFactoryCollection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkObjectFactory.h" // Needed for inline methods

class VTKCOMMONCORE_EXPORT vtkObjectFactoryCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkObjectFactoryCollection,vtkCollection);
  static vtkObjectFactoryCollection *New();

  // Description:
  // Add an ObjectFactory from the list.
  void AddItem(vtkObjectFactory *t)
    {
      this->vtkCollection::AddItem(t);
    }

  // Description:
  // Get the next ObjectFactory in the list. Return NULL when the end of the
  // list is reached.
  vtkObjectFactory *GetNextItem()
    { return static_cast<vtkObjectFactory *>(this->GetNextItemAsObject());}

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkObjectFactory *GetNextObjectFactory(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkObjectFactory *>(
      this->GetNextItemAsObject(cookie));};
  //ETX

protected:
  vtkObjectFactoryCollection() {};
  ~vtkObjectFactoryCollection() {};


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkObjectFactoryCollection(const vtkObjectFactoryCollection&);  // Not implemented.
  void operator=(const vtkObjectFactoryCollection&);  // Not implemented.
};


#endif
// VTK-HeaderTest-Exclude: vtkObjectFactoryCollection.h
