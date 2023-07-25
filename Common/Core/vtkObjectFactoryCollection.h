// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkCollection.h"
#include "vtkCommonCoreModule.h" // For export macro

#include "vtkObjectFactory.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkObjectFactoryCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkObjectFactoryCollection, vtkCollection);
  static vtkObjectFactoryCollection* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add an ObjectFactory the bottom of the list.
   */
  void AddItem(vtkObjectFactory* t) { this->vtkCollection::AddItem(t); }

  /**
   * Get the next ObjectFactory in the list. Return nullptr when the end of the
   * list is reached.
   */
  vtkObjectFactory* GetNextItem()
  {
    return static_cast<vtkObjectFactory*>(this->GetNextItemAsObject());
  }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkObjectFactory* GetNextObjectFactory(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkObjectFactory*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkObjectFactoryCollection() = default;
  ~vtkObjectFactoryCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkObjectFactoryCollection(const vtkObjectFactoryCollection&) = delete;
  void operator=(const vtkObjectFactoryCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
