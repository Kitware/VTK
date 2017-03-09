/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRendererCollection
 * @brief   an ordered list of renderers
 *
 * vtkRendererCollection represents and provides methods to manipulate a list
 * of renderers (i.e., vtkRenderer and subclasses). The list is ordered and
 * duplicate entries are not prevented.
 *
 * @sa
 * vtkRenderer vtkCollection
*/

#ifndef vtkRendererCollection_h
#define vtkRendererCollection_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCollection.h"
#include "vtkRenderer.h" // Needed for static cast

class VTKRENDERINGCORE_EXPORT vtkRendererCollection : public vtkCollection
{
public:
  static vtkRendererCollection *New();
  vtkTypeMacro(vtkRendererCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add a Renderer to the bottom of the list.
   */
  void AddItem(vtkRenderer *a)
    { this->vtkCollection::AddItem(a); }


  /**
   * Get the next Renderer in the list.
   * Return NULL when at the end of the list.
   */
  vtkRenderer *GetNextItem()
    { return static_cast<vtkRenderer *>(this->GetNextItemAsObject()); }

  /**
   * Forward the Render() method to each renderer in the list.
   */
  void Render();

  /**
   * Get the first Renderer in the list.
   * Return NULL when at the end of the list.
   */
  vtkRenderer *GetFirstRenderer();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkRenderer *GetNextRenderer(vtkCollectionSimpleIterator &cookie)
    { return static_cast<vtkRenderer *>(this->GetNextItemAsObject(cookie)); }

protected:
  vtkRendererCollection() {}
  ~vtkRendererCollection() VTK_OVERRIDE {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
    { this->vtkCollection::AddItem(o); }

  vtkRendererCollection(const vtkRendererCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRendererCollection&) VTK_DELETE_FUNCTION;
};

#endif
