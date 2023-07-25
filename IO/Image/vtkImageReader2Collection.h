// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageReader2Collection
 * @brief   maintain a list of image readers
 *
 * vtkImageReader2Collection is an object that creates and manipulates
 * lists of objects of type vtkImageReader2 and its subclasses.
 * @sa
 * vtkCollection vtkPlaneCollection
 */

#ifndef vtkImageReader2Collection_h
#define vtkImageReader2Collection_h

#include "vtkCollection.h"
#include "vtkIOImageModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImageReader2;

class VTKIOIMAGE_EXPORT vtkImageReader2Collection : public vtkCollection
{
public:
  vtkTypeMacro(vtkImageReader2Collection, vtkCollection);
  static vtkImageReader2Collection* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add an image reader to the list.
   */
  void AddItem(vtkImageReader2*);

  /**
   * Get the next image reader in the list.
   */
  vtkImageReader2* GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkImageReader2* GetNextImageReader2(vtkCollectionSimpleIterator& cookie);

protected:
  vtkImageReader2Collection() = default;
  ~vtkImageReader2Collection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkImageReader2Collection(const vtkImageReader2Collection&) = delete;
  void operator=(const vtkImageReader2Collection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
