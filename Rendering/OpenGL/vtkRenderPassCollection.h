/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderPassCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRenderPassCollection
 * @brief   an ordered list of RenderPasses
 *
 * vtkRenderPassCollection represents a list of RenderPasses
 * (i.e., vtkRenderPass and subclasses) and provides methods to manipulate the
 * list. The list is ordered and duplicate entries are not prevented.
 *
 * @sa
 * vtkRenderPass vtkCollection
*/

#ifndef vtkRenderPassCollection_h
#define vtkRenderPassCollection_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkCollection.h"

class vtkRenderPass;

class VTKRENDERINGOPENGL_EXPORT vtkRenderPassCollection : public vtkCollection
{
 public:
  static vtkRenderPassCollection *New();
  vtkTypeMacro(vtkRenderPassCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add an RenderPass to the bottom of the list.
   */
  void AddItem(vtkRenderPass *pass);

  /**
   * Get the next RenderPass in the list.
   */
  vtkRenderPass *GetNextRenderPass();

  /**
   * Get the last RenderPass in the list.
   */
  vtkRenderPass *GetLastRenderPass();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkRenderPass *GetNextRenderPass(vtkCollectionSimpleIterator &cookie);

protected:
  vtkRenderPassCollection();
  ~vtkRenderPassCollection() VTK_OVERRIDE;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o);

private:
  vtkRenderPassCollection(const vtkRenderPassCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRenderPassCollection&) VTK_DELETE_FUNCTION;
};

#endif
