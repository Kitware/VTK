// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLightCollection
 * @brief   an ordered list of lights
 *
 * vtkLightCollection represents and provides methods to manipulate a list of
 * lights (i.e., vtkLight and subclasses). The list is ordered and duplicate
 * entries are not prevented.
 *
 * @sa
 * vtkCollection vtkLight
 */

#ifndef vtkLightCollection_h
#define vtkLightCollection_h

#include "vtkCollection.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkLight;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkLightCollection : public vtkCollection
{
public:
  static vtkLightCollection* New();
  vtkTypeMacro(vtkLightCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a light to the bottom of the list.
   */
  void AddItem(vtkLight* a);

  /**
   * Get the next light in the list. NULL is returned when the collection is
   * exhausted.
   */
  vtkLight* GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkLight* GetNextLight(vtkCollectionSimpleIterator& cookie);

protected:
  vtkLightCollection() = default;
  ~vtkLightCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkLightCollection(const vtkLightCollection&) = delete;
  void operator=(const vtkLightCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
