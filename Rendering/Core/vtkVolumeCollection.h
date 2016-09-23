/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeCollection
 * @brief   a list of volumes
 *
 * vtkVolumeCollection represents and provides methods to manipulate a
 * list of volumes (i.e., vtkVolume and subclasses). The list is unsorted
 * and duplicate entries are not prevented.
 *
 * @sa
 * vtkCollection vtkVolume
*/

#ifndef vtkVolumeCollection_h
#define vtkVolumeCollection_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPropCollection.h"

#include "vtkVolume.h"  // Needed for static cast

class VTKRENDERINGCORE_EXPORT vtkVolumeCollection : public vtkPropCollection
{
 public:
  static vtkVolumeCollection *New();
  vtkTypeMacro(vtkVolumeCollection, vtkPropCollection);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add a Volume to the list.
   */
  void AddItem(vtkVolume *a)
    { this->vtkCollection::AddItem(a); }

  /**
   * Get the next Volume in the list. Return NULL when at the end of the
   * list.
   */
  vtkVolume *GetNextVolume()
    { return static_cast<vtkVolume *>(this->GetNextItemAsObject()); }


  /**
   * Access routine provided for compatibility with previous
   * versions of VTK.  Please use the GetNextVolume() variant
   * where possible.
   */
  vtkVolume *GetNextItem()
    { return this->GetNextVolume(); }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkVolume *GetNextVolume(vtkCollectionSimpleIterator &cookie)
    { return static_cast<vtkVolume *>(this->GetNextItemAsObject(cookie)); }

protected:
  vtkVolumeCollection() {}
  ~vtkVolumeCollection() {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
    { this->vtkCollection::AddItem(o); }
  void AddItem(vtkProp *o)
    { this->vtkPropCollection::AddItem(o); }

private:
  vtkVolumeCollection(const vtkVolumeCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeCollection&) VTK_DELETE_FUNCTION;
};

#endif
