// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderWindowCollection
 * @brief   an ordered list of RenderWindows
 *
 * vtkRenderWindowCollection represents and provides methods to manipulate a
 * list of RenderWindows. The list is ordered and duplicate entries are
 * not prevented.
 *
 * @sa
 * vtkRenderWindow vtkCollection
 */

#ifndef vtkRenderWindowCollection_h
#define vtkRenderWindowCollection_h

#include "vtkCollection.h"
#include "vtkRenderWindow.h"        // Needed for static cast
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT vtkRenderWindowCollection : public vtkCollection
{
public:
  static vtkRenderWindowCollection* New();
  vtkTypeMacro(vtkRenderWindowCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a RenderWindow to the bottom of the list.
   */
  void AddItem(vtkRenderWindow* a) { this->vtkCollection::AddItem(a); }

  /**
   * Get the next RenderWindow in the list. Return NULL when at the end of the
   * list.
   */
  vtkRenderWindow* GetNextItem()
  {
    return static_cast<vtkRenderWindow*>(this->GetNextItemAsObject());
  }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkRenderWindow* GetNextRenderWindow(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkRenderWindow*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkRenderWindowCollection() = default;
  ~vtkRenderWindowCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkRenderWindowCollection(const vtkRenderWindowCollection&) = delete;
  void operator=(const vtkRenderWindowCollection&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
