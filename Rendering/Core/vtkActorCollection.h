// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkActor.h" // For inline methods
#include "vtkPropCollection.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkProperty;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkActorCollection : public vtkPropCollection
{
public:
  static vtkActorCollection* New();
  vtkTypeMacro(vtkActorCollection, vtkPropCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add an actor to the bottom of the list.
   */
  void AddItem(vtkActor* a);

  /**
   * Get the next actor in the list.
   */
  vtkActor* GetNextActor();

  /**
   * Get the last actor in the list.
   */
  vtkActor* GetLastActor();

  ///@{
  /**
   * Access routines that are provided for compatibility with previous
   * version of VTK.  Please use the GetNextActor(), GetLastActor() variants
   * where possible.
   */
  vtkActor* GetNextItem();
  vtkActor* GetLastItem();
  ///@}

  /**
   * Apply properties to all actors in this collection.
   */
  void ApplyProperties(vtkProperty* p);

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkActor* GetNextActor(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkActor*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkActorCollection() = default;
  ~vtkActorCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }
  void AddItem(vtkProp* o) { this->vtkPropCollection::AddItem(o); }

  vtkActorCollection(const vtkActorCollection&) = delete;
  void operator=(const vtkActorCollection&) = delete;
};

inline void vtkActorCollection::AddItem(vtkActor* a)
{
  this->vtkCollection::AddItem(a);
}

inline vtkActor* vtkActorCollection::GetNextActor()
{
  return static_cast<vtkActor*>(this->GetNextItemAsObject());
}

inline vtkActor* vtkActorCollection::GetLastActor()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<vtkActor*>(this->Bottom->Item);
  }
}

inline vtkActor* vtkActorCollection::GetNextItem()
{
  return this->GetNextActor();
}

inline vtkActor* vtkActorCollection::GetLastItem()
{
  return this->GetLastActor();
}

VTK_ABI_NAMESPACE_END
#endif
