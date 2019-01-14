/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCullerCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCullerCollection
 * @brief   an ordered list of Cullers
 *
 * vtkCullerCollection represents and provides methods to manipulate a list
 * of Cullers (i.e., vtkCuller and subclasses). The list is ordered and
 * duplicate entries are not prevented.
 *
 * @sa
 * vtkCuller vtkCollection
*/

#ifndef vtkCullerCollection_h
#define vtkCullerCollection_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCollection.h"
#include "vtkCuller.h" // for inline functions

class VTKRENDERINGCORE_EXPORT vtkCullerCollection : public vtkCollection
{
 public:
  static vtkCullerCollection *New();
  vtkTypeMacro(vtkCullerCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add an Culler to the bottom of the list.
   */
  void AddItem(vtkCuller *a)
  {
      this->vtkCollection::AddItem(a);
  }

  /**
   * Get the next Culler in the list.
   */
  vtkCuller *GetNextItem()
  {
      return static_cast<vtkCuller *>(this->GetNextItemAsObject());
  }

  /**
   * Get the last Culler in the list.
   */
  vtkCuller *GetLastItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkCuller *GetNextCuller(vtkCollectionSimpleIterator &cookie)
  {
      return static_cast<vtkCuller *>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkCullerCollection() {}
  ~vtkCullerCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
  {
      this->vtkCollection::AddItem(o);
  }

private:
  vtkCullerCollection(const vtkCullerCollection&) = delete;
  void operator=(const vtkCullerCollection&) = delete;
};


inline vtkCuller *vtkCullerCollection::GetLastItem()
{
  if ( this->Bottom == nullptr )
  {
    return nullptr;
  }
  else
  {
    return static_cast<vtkCuller *>(this->Bottom->Item);
  }
}

#endif





