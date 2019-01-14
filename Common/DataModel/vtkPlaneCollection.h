/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPlaneCollection
 * @brief   maintain a list of planes
 *
 * vtkPlaneCollection is an object that creates and manipulates
 * lists of objects of type vtkPlane.
 * @sa
 * vtkCollection
*/

#ifndef vtkPlaneCollection_h
#define vtkPlaneCollection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkPlane.h" // Needed for inline methods

class VTKCOMMONDATAMODEL_EXPORT vtkPlaneCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkPlaneCollection,vtkCollection);
  static vtkPlaneCollection *New();

  /**
   * Add a plane to the list.
   */
  void AddItem(vtkPlane *);

  /**
   * Get the next plane in the list.
   */
  vtkPlane *GetNextItem();

  /**
   * Get the ith plane in the list.
   */
  vtkPlane *GetItem(int i) {
    return static_cast<vtkPlane *>(this->GetItemAsObject(i));};

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkPlane *GetNextPlane(vtkCollectionSimpleIterator &cookie);

protected:
  vtkPlaneCollection() {}
  ~vtkPlaneCollection() override {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkPlaneCollection(const vtkPlaneCollection&) = delete;
  void operator=(const vtkPlaneCollection&) = delete;
};

inline void vtkPlaneCollection::AddItem(vtkPlane *f)
{
  this->vtkCollection::AddItem(f);
}

inline vtkPlane *vtkPlaneCollection::GetNextItem()
{
 return static_cast<vtkPlane *>(this->GetNextItemAsObject());
}

#endif
// VTK-HeaderTest-Exclude: vtkPlaneCollection.h
