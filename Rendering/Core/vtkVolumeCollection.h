// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVolumeCollection
 * @brief   an ordered list of volumes
 *
 * vtkVolumeCollection represents and provides methods to manipulate a
 * list of volumes (i.e., vtkVolume and subclasses). The list is ordered
 * and duplicate entries are not prevented.
 *
 * @sa
 * vtkCollection vtkVolume
 */

#ifndef vtkVolumeCollection_h
#define vtkVolumeCollection_h

#include "vtkPropCollection.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

#include "vtkVolume.h" // Needed for static cast

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkVolumeCollection : public vtkPropCollection
{
public:
  static vtkVolumeCollection* New();
  vtkTypeMacro(vtkVolumeCollection, vtkPropCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a Volume to the bottom of the list.
   */
  void AddItem(vtkVolume* a) { this->vtkCollection::AddItem(a); }

  /**
   * Get the next Volume in the list. Return NULL when at the end of the
   * list.
   */
  vtkVolume* GetNextVolume() { return static_cast<vtkVolume*>(this->GetNextItemAsObject()); }

  /**
   * Access routine provided for compatibility with previous
   * versions of VTK.  Please use the GetNextVolume() variant
   * where possible.
   */
  vtkVolume* GetNextItem() { return this->GetNextVolume(); }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkVolume* GetNextVolume(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkVolume*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkVolumeCollection() = default;
  ~vtkVolumeCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }
  void AddItem(vtkProp* o) { this->vtkPropCollection::AddItem(o); }

  vtkVolumeCollection(const vtkVolumeCollection&) = delete;
  void operator=(const vtkVolumeCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
