/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTransformCollection
 * @brief   maintain a list of transforms
 *
 *
 * vtkTransformCollection is an object that creates and manipulates lists of
 * objects of type vtkTransform.
 *
 * @sa
 * vtkCollection vtkTransform
*/

#ifndef vtkTransformCollection_h
#define vtkTransformCollection_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkTransform.h" // Needed for inline methods

class VTKCOMMONTRANSFORMS_EXPORT vtkTransformCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkTransformCollection,vtkCollection);
  static vtkTransformCollection *New();

  /**
   * Add a Transform to the list.
   */
  void AddItem(vtkTransform *);

  /**
   * Get the next Transform in the list. Return nullptr when the end of the
   * list is reached.
   */
  vtkTransform *GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkTransform *GetNextTransform(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkTransform *>(this->GetNextItemAsObject(cookie));};

protected:
  vtkTransformCollection() {}
  ~vtkTransformCollection() override {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
  {
      this->vtkCollection::AddItem(o);
  }

private:
  vtkTransformCollection(const vtkTransformCollection&) = delete;
  void operator=(const vtkTransformCollection&) = delete;
};

//----------------------------------------------------------------------------
inline void vtkTransformCollection::AddItem(vtkTransform *t)
{
  this->vtkCollection::AddItem(t);
}

//----------------------------------------------------------------------------
inline vtkTransform *vtkTransformCollection::GetNextItem()
{
  return static_cast<vtkTransform *>(this->GetNextItemAsObject());
}

#endif
// VTK-HeaderTest-Exclude: vtkTransformCollection.h
