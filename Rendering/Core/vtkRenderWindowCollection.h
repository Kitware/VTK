/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRenderWindowCollection
 * @brief   a list of RenderWindows
 *
 * vtkRenderWindowCollection represents and provides methods to manipulate a
 * list of RenderWindows. The list is unsorted and duplicate entries are
 * not prevented.
 *
 * @sa
 * vtkRenderWindow vtkCollection
*/

#ifndef vtkRenderWindowCollection_h
#define vtkRenderWindowCollection_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCollection.h"
#include "vtkRenderWindow.h" // Needed for static cast

class VTKRENDERINGCORE_EXPORT vtkRenderWindowCollection : public vtkCollection
{
 public:
  static vtkRenderWindowCollection *New();
  vtkTypeMacro(vtkRenderWindowCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add a RenderWindow to the list.
   */
  void AddItem(vtkRenderWindow *a)
  {
      this->vtkCollection::AddItem(a);
  }

  /**
   * Get the next RenderWindow in the list. Return NULL when at the end of the
   * list.
   */
  vtkRenderWindow *GetNextItem()
  {
      return static_cast<vtkRenderWindow *>(this->GetNextItemAsObject());
  }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkRenderWindow *GetNextRenderWindow(vtkCollectionSimpleIterator &cookie)
  {
      return static_cast<vtkRenderWindow *>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkRenderWindowCollection() {}
  ~vtkRenderWindowCollection() {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
  {
      this->vtkCollection::AddItem(o);
  }

private:
  vtkRenderWindowCollection(const vtkRenderWindowCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRenderWindowCollection&) VTK_DELETE_FUNCTION;
};
#endif
