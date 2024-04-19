// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkActor2DCollection
 * @brief    a list of 2D actors
 *
 * vtkActor2DCollection is a subclass of vtkCollection.  vtkActor2DCollection
 * maintains a collection of vtkActor2D objects that is sorted by layer
 * number, with lower layer numbers at the start of the list.  This allows
 * the vtkActor2D objects to be rendered in the correct order.
 *
 * @sa
 * vtkActor2D vtkCollection
 */

#ifndef vtkActor2DCollection_h
#define vtkActor2DCollection_h

#include "vtkPropCollection.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

#include "vtkActor2D.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class vtkViewport;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkActor2DCollection : public vtkPropCollection
{
public:
  /**
   * Destructor for the vtkActor2DCollection class. This removes all
   * objects from the collection.
   */
  static vtkActor2DCollection* New();

  vtkTypeMacro(vtkActor2DCollection, vtkPropCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sorts the vtkActor2DCollection by layer number.  Smaller layer
   * numbers are first.  Layer numbers can be any integer value.
   */
  void Sort();

  /**
   * Add an actor to the list.  The new actor is inserted in the list
   * according to it's layer number.
   */
  void AddItem(vtkActor2D* a);

  ///@{
  /**
   * Standard Collection methods
   */
  int IsItemPresent(vtkActor2D* a);
  int IndexOfFirstOccurence(vtkActor2D* a);
  vtkActor2D* GetNextActor2D();
  vtkActor2D* GetLastActor2D();
  ///@}

  ///@{
  /**
   * Access routines that are provided for compatibility with previous
   * version of VTK.  Please use the GetNextActor2D(), GetLastActor2D()
   * variants where possible.
   */
  vtkActor2D* GetNextItem();
  vtkActor2D* GetLastItem();
  ///@}

  /**
   * Sort and then render the collection of 2D actors.
   */
  void RenderOverlay(vtkViewport* viewport);

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkActor2D* GetNextActor2D(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkActor2D*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkActor2DCollection() = default;
  ~vtkActor2DCollection() override;

  void DeleteElement(vtkCollectionElement*) override;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }
  void AddItem(vtkProp* o) { this->vtkPropCollection::AddItem(o); }
  int IsItemPresent(vtkObject* o) { return this->vtkCollection::IsItemPresent(o); }
  int IndexOfFirstOccurence(vtkObject* o) { return this->vtkCollection::IndexOfFirstOccurence(o); }

  vtkActor2DCollection(const vtkActor2DCollection&) = delete;
  void operator=(const vtkActor2DCollection&) = delete;
};

inline int vtkActor2DCollection::IsItemPresent(vtkActor2D* a)
{
  return this->vtkCollection::IsItemPresent(a);
}

inline int vtkActor2DCollection::IndexOfFirstOccurence(vtkActor2D* a)
{
  return this->vtkCollection::IndexOfFirstOccurence(a);
}

inline vtkActor2D* vtkActor2DCollection::GetNextActor2D()
{
  return static_cast<vtkActor2D*>(this->GetNextItemAsObject());
}

inline vtkActor2D* vtkActor2DCollection::GetLastActor2D()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<vtkActor2D*>(this->Bottom->Item);
  }
}

inline vtkActor2D* vtkActor2DCollection::GetNextItem()
{
  return this->GetNextActor2D();
}

inline vtkActor2D* vtkActor2DCollection::GetLastItem()
{
  return this->GetLastActor2D();
}

VTK_ABI_NAMESPACE_END
#endif
