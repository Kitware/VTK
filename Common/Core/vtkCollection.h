/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCollection
 * @brief   create and manipulate unsorted lists of objects
 *
 * vtkCollection is a general object for creating and manipulating lists
 * of objects. The lists are unsorted and allow duplicate entries.
 * vtkCollection also serves as a base class for lists of specific types
 * of objects.
 *
 * @sa
 * vtkActorCollection vtkAssemblyPaths vtkDataSetCollection
 * vtkImplicitFunctionCollection vtkLightCollection vtkPolyDataCollection
 * vtkRenderWindowCollection vtkRendererCollection
 * vtkStructuredPointsCollection vtkTransformCollection vtkVolumeCollection
*/

#ifndef vtkCollection_h
#define vtkCollection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkCollectionElement //;prevents pick-up by man page generator
{
 public:
  vtkCollectionElement():Item(NULL),Next(NULL) {}
  vtkObject *Item;
  vtkCollectionElement *Next;
};
typedef void * vtkCollectionSimpleIterator;

class vtkCollectionIterator;

class VTKCOMMONCORE_EXPORT vtkCollection : public vtkObject
{
public:
  vtkTypeMacro(vtkCollection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct with empty list.
   */
  static vtkCollection *New();

  /**
   * Add an object to the list. Does not prevent duplicate entries.
   */
  void AddItem(vtkObject *);

  /**
   * Insert item into the list after the i'th item. Does not prevent duplicate entries.
   * If i < 0 the item is placed at the top of the list.
   */
  void InsertItem(int i, vtkObject *);

  /**
   * Replace the i'th item in the collection with a
   */
  void ReplaceItem(int i, vtkObject *);

  /**
   * Remove the i'th item in the list.
   * Be careful if using this function during traversal of the list using
   * GetNextItemAsObject (or GetNextItem in derived class).  The list WILL
   * be shortened if a valid index is given!  If this->Current is equal to the
   * element being removed, have it point to then next element in the list.
   */
  void RemoveItem(int i);

  /**
   * Remove an object from the list. Removes the first object found, not
   * all occurrences. If no object found, list is unaffected.  See warning
   * in description of RemoveItem(int).
   */
  void RemoveItem(vtkObject *);

  /**
   * Remove all objects from the list.
   */
  void RemoveAllItems();

  /**
   * Search for an object and return location in list. If the return value is
   * 0, the object was not found. If the object was found, the location is
   * the return value-1.
   */
  int IsItemPresent(vtkObject *a);

  /**
   * Return the number of objects in the list.
   */
  int  GetNumberOfItems() { return this->NumberOfItems; }

  /**
   * Initialize the traversal of the collection. This means the data pointer
   * is set at the beginning of the list.
   */
  void InitTraversal() { this->Current = this->Top;};

  /**
   * A reentrant safe way to iterate through a collection.
   * Just pass the same cookie value around each time
   */
  void InitTraversal(vtkCollectionSimpleIterator &cookie) {
    cookie = static_cast<vtkCollectionSimpleIterator>(this->Top);};

  /**
   * Get the next item in the collection. NULL is returned if the collection
   * is exhausted.
   */
  vtkObject *GetNextItemAsObject();

  /**
   * Get the i'th item in the collection. NULL is returned if i is out
   * of range
   */
  vtkObject *GetItemAsObject(int i);

  /**
   * A reentrant safe way to get the next object as a collection. Just pass the
   * same cookie back and forth.
   */
  vtkObject *GetNextItemAsObject(vtkCollectionSimpleIterator &cookie);

  /**
   * Get an iterator to traverse the objects in this collection.
   */
  VTK_NEWINSTANCE vtkCollectionIterator* NewIterator();

  //@{
  /**
   * Participate in garbage collection.
   */
  void Register(vtkObjectBase* o) VTK_OVERRIDE;
  void UnRegister(vtkObjectBase* o) VTK_OVERRIDE;
  //@}

protected:
  vtkCollection();
  ~vtkCollection() VTK_OVERRIDE;

  virtual void RemoveElement(vtkCollectionElement *element,
                             vtkCollectionElement *previous);
  virtual void DeleteElement(vtkCollectionElement *);
  int NumberOfItems;
  vtkCollectionElement *Top;
  vtkCollectionElement *Bottom;
  vtkCollectionElement *Current;

  friend class vtkCollectionIterator;

  // See vtkGarbageCollector.h:
  void ReportReferences(vtkGarbageCollector* collector) VTK_OVERRIDE;
private:
  vtkCollection(const vtkCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCollection&) VTK_DELETE_FUNCTION;
};


inline vtkObject *vtkCollection::GetNextItemAsObject()
{
  vtkCollectionElement *elem=this->Current;

  if ( elem != NULL )
  {
    this->Current = elem->Next;
    return elem->Item;
  }
  else
  {
    return NULL;
  }
}

inline vtkObject *vtkCollection::GetNextItemAsObject(void *&cookie)
{
  vtkCollectionElement *elem=static_cast<vtkCollectionElement *>(cookie);

  if ( elem != NULL )
  {
    cookie = static_cast<void *>(elem->Next);
    return elem->Item;
  }
  else
  {
    return NULL;
  }
}

#endif





