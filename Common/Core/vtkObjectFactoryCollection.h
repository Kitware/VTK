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
/**
 * @class   vtkObjectFactoryCollection
 * @brief   maintain a list of object factories
 *
 * vtkObjectFactoryCollection is an object that creates and manipulates
 * ordered lists of objects of type vtkObjectFactory.
 *
 * @sa
 * vtkCollection vtkObjectFactory
*/

#ifndef vtkObjectFactoryCollection_h
#define vtkObjectFactoryCollection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkObjectFactory.h" // Needed for inline methods

class VTKCOMMONCORE_EXPORT vtkObjectFactoryCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkObjectFactoryCollection,vtkCollection);
  static vtkObjectFactoryCollection *New();

  /**
   * Add an ObjectFactory the bottom of the list.
   */
  void AddItem(vtkObjectFactory *t)
  {
      this->vtkCollection::AddItem(t);
  }

  /**
   * Get the next ObjectFactory in the list. Return nullptr when the end of the
   * list is reached.
   */
  vtkObjectFactory *GetNextItem()
    { return static_cast<vtkObjectFactory *>(this->GetNextItemAsObject());}

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkObjectFactory *GetNextObjectFactory(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkObjectFactory *>(
      this->GetNextItemAsObject(cookie));};

protected:
  vtkObjectFactoryCollection() {}
  ~vtkObjectFactoryCollection() override {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkObjectFactoryCollection(const vtkObjectFactoryCollection&) = delete;
  void operator=(const vtkObjectFactoryCollection&) = delete;
};


#endif
// VTK-HeaderTest-Exclude: vtkObjectFactoryCollection.h
