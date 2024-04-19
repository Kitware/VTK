// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRendererCollection
 * @brief   an ordered list of renderers
 *
 * vtkRendererCollection represents and provides methods to manipulate a list
 * of renderers (i.e., vtkRenderer and subclasses). The list is ordered and
 * duplicate entries are not prevented.
 *
 * @sa
 * vtkRenderer vtkCollection
 */

#ifndef vtkRendererCollection_h
#define vtkRendererCollection_h

#include "vtkCollection.h"
#include "vtkRenderer.h"            // Needed for static cast
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkRendererCollection : public vtkCollection
{
public:
  static vtkRendererCollection* New();
  vtkTypeMacro(vtkRendererCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a Renderer to the bottom of the list.
   */
  void AddItem(vtkRenderer* a) { this->vtkCollection::AddItem(a); }

  /**
   * Get the next Renderer in the list.
   * Return NULL when at the end of the list.
   */
  vtkRenderer* GetNextItem() { return static_cast<vtkRenderer*>(this->GetNextItemAsObject()); }

  /**
   * Forward the Render() method to each renderer in the list.
   */
  void Render();

  /**
   * Get the first Renderer in the list.
   * Return NULL when at the end of the list.
   */
  vtkRenderer* GetFirstRenderer();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkRenderer* GetNextRenderer(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkRenderer*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkRendererCollection() = default;
  ~vtkRendererCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkRendererCollection(const vtkRendererCollection&) = delete;
  void operator=(const vtkRendererCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
