/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataArrayCollection
 * @brief   maintain an ordered list of dataarray objects
 *
 * vtkDataArrayCollection is an object that creates and manipulates lists of
 * datasets. See also vtkCollection and subclasses.
*/

#ifndef vtkDataArrayCollection_h
#define vtkDataArrayCollection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkDataArray.h" // Needed for inline methods

class VTKCOMMONCORE_EXPORT vtkDataArrayCollection : public vtkCollection
{
public:
  static vtkDataArrayCollection *New();
  vtkTypeMacro(vtkDataArrayCollection,vtkCollection);

  /**
   * Add a dataarray to the bottom of the list.
   */
  void AddItem(vtkDataArray *ds)
  {
      this->vtkCollection::AddItem(ds);
  }

  /**
   * Get the next dataarray in the list.
   */
  vtkDataArray *GetNextItem() {
    return static_cast<vtkDataArray *>(this->GetNextItemAsObject());};

  /**
   * Get the ith dataarray in the list.
   */
  vtkDataArray *GetItem(int i) {
    return static_cast<vtkDataArray *>(this->GetItemAsObject(i));};

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkDataArray *GetNextDataArray(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkDataArray *>(this->GetNextItemAsObject(cookie));};

protected:
  vtkDataArrayCollection() {}
  ~vtkDataArrayCollection() VTK_OVERRIDE {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkDataArrayCollection(const vtkDataArrayCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataArrayCollection&) VTK_DELETE_FUNCTION;
};


#endif
// VTK-HeaderTest-Exclude: vtkDataArrayCollection.h
