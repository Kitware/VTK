/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActorCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkActorCollection
 * @brief   an ordered list of actors
 *
 * vtkActorCollection represents and provides methods to manipulate a list of
 * actors (i.e., vtkActor and subclasses). The list is ordered and duplicate
 * entries are not prevented.
 *
 * @sa
 * vtkActor vtkCollection
*/

#ifndef vtkActorCollection_h
#define vtkActorCollection_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPropCollection.h"
#include "vtkActor.h" // For inline methods

class vtkProperty;

class VTKRENDERINGCORE_EXPORT vtkActorCollection : public vtkPropCollection
{
public:
  static vtkActorCollection *New();
  vtkTypeMacro(vtkActorCollection,vtkPropCollection);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add an actor to the bottom of the list.
   */
  void AddItem(vtkActor *a);

  /**
   * Get the next actor in the list.
   */
  vtkActor *GetNextActor();

  /**
   * Get the last actor in the list.
   */
  vtkActor *GetLastActor();

  //@{
  /**
   * Access routines that are provided for compatibility with previous
   * version of VTK.  Please use the GetNextActor(), GetLastActor() variants
   * where possible.
   */
  vtkActor *GetNextItem();
  vtkActor *GetLastItem();
  //@}

  /**
   * Apply properties to all actors in this collection.
   */
  void ApplyProperties(vtkProperty *p);

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkActor *GetNextActor(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkActor *>(this->GetNextItemAsObject(cookie));};

protected:
  vtkActorCollection() {}
  ~vtkActorCollection() VTK_OVERRIDE {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
  void AddItem(vtkProp *o) { this->vtkPropCollection::AddItem(o); };

private:
  vtkActorCollection(const vtkActorCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkActorCollection&) VTK_DELETE_FUNCTION;
};

inline void vtkActorCollection::AddItem(vtkActor *a)
{
  this->vtkCollection::AddItem(a);
}

inline vtkActor *vtkActorCollection::GetNextActor()
{
  return static_cast<vtkActor *>(this->GetNextItemAsObject());
}

inline vtkActor *vtkActorCollection::GetLastActor()
{
  if ( this->Bottom == NULL )
  {
    return NULL;
  }
  else
  {
    return static_cast<vtkActor *>(this->Bottom->Item);
  }
}

inline vtkActor *vtkActorCollection::GetNextItem()
{
  return this->GetNextActor();
}

inline vtkActor *vtkActorCollection::GetLastItem()
{
  return this->GetLastActor();
}

#endif





