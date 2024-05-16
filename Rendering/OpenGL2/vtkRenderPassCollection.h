// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderPassCollection
 * @brief   an ordered list of RenderPasses
 *
 * vtkRenderPassCollection represents a list of RenderPasses
 * (i.e., vtkRenderPass and subclasses) and provides methods to manipulate the
 * list. The list is ordered and duplicate entries are not prevented.
 *
 * @sa
 * vtkRenderPass vtkCollection
 */

#ifndef vtkRenderPassCollection_h
#define vtkRenderPassCollection_h

#include "vtkCollection.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderPass;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkRenderPassCollection : public vtkCollection
{
public:
  static vtkRenderPassCollection* New();
  vtkTypeMacro(vtkRenderPassCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add an RenderPass to the bottom of the list.
   */
  void AddItem(vtkRenderPass* pass);

  /**
   * Get the next RenderPass in the list.
   */
  vtkRenderPass* GetNextRenderPass();

  /**
   * Get the last RenderPass in the list.
   */
  vtkRenderPass* GetLastRenderPass();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkRenderPass* GetNextRenderPass(vtkCollectionSimpleIterator& cookie);

protected:
  vtkRenderPassCollection();
  ~vtkRenderPassCollection() override;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o);

  vtkRenderPassCollection(const vtkRenderPassCollection&) = delete;
  void operator=(const vtkRenderPassCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
