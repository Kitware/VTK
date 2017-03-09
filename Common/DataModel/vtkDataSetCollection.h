/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataSetCollection
 * @brief   maintain an unordered list of dataset objects
 *
 * vtkDataSetCollection is an object that creates and manipulates ordered
 * lists of datasets. See also vtkCollection and subclasses.
*/

#ifndef vtkDataSetCollection_h
#define vtkDataSetCollection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkDataSet.h" // Needed for inline methods.

class VTKCOMMONDATAMODEL_EXPORT vtkDataSetCollection : public vtkCollection
{
public:
  static vtkDataSetCollection *New();
  vtkTypeMacro(vtkDataSetCollection,vtkCollection);

  /**
   * Add a dataset to the bottom of the list.
   */
  void AddItem(vtkDataSet *ds)
  {
      this->vtkCollection::AddItem(ds);
  }

  //@{
  /**
   * Get the next dataset in the list.
   */
  vtkDataSet *GetNextItem() {
    return static_cast<vtkDataSet *>(this->GetNextItemAsObject());};
  vtkDataSet *GetNextDataSet() {
    return static_cast<vtkDataSet *>(this->GetNextItemAsObject());};
  //@}

  //@{
  /**
   * Get the ith dataset in the list.
   */
  vtkDataSet *GetItem(int i) {
    return static_cast<vtkDataSet *>(this->GetItemAsObject(i));};
  vtkDataSet *GetDataSet(int i) {
    return static_cast<vtkDataSet *>(this->GetItemAsObject(i));};
  //@}

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkDataSet *GetNextDataSet(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkDataSet *>(this->GetNextItemAsObject(cookie));};

protected:
  vtkDataSetCollection() {}
  ~vtkDataSetCollection() VTK_OVERRIDE {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkDataSetCollection(const vtkDataSetCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataSetCollection&) VTK_DELETE_FUNCTION;
};


#endif
// VTK-HeaderTest-Exclude: vtkDataSetCollection.h
