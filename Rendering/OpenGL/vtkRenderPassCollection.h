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
// .NAME vtkRenderPassCollection - a list of RenderPasses
// .SECTION Description
// vtkRenderPassCollection represents a list of RenderPasses
// (i.e., vtkRenderPass and subclasses) and provides methods to manipulate the
// list. The list is unsorted and duplicate entries are not prevented.

// .SECTION see also
// vtkRenderPass vtkCollection 

#ifndef __vtkRenderPassCollection_h
#define __vtkRenderPassCollection_h

#include "vtkCollection.h"

class vtkRenderPass;

class VTK_RENDERING_EXPORT vtkRenderPassCollection : public vtkCollection
{
 public:
  static vtkRenderPassCollection *New();
  vtkTypeMacro(vtkRenderPassCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Add an RenderPass to the list.
  void AddItem(vtkRenderPass *pass);

  // Description:
  // Get the next RenderPass in the list.
  vtkRenderPass *GetNextRenderPass();

  // Description:
  // Get the last RenderPass in the list.
  vtkRenderPass *GetLastRenderPass();
    
  //BTX
  // Description: 
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth. 
  vtkRenderPass *GetNextRenderPass(vtkCollectionSimpleIterator &cookie);
  //ETX

protected:
  vtkRenderPassCollection();
  ~vtkRenderPassCollection();

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o);

private:
  vtkRenderPassCollection(const vtkRenderPassCollection&);  // Not implemented.
  void operator=(const vtkRenderPassCollection&);  // Not implemented.
};

#endif
