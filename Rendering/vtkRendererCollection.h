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
// .NAME vtkRendererCollection - a list of renderers
// .SECTION Description
// vtkRendererCollection represents and provides methods to manipulate a list 
// of renderers (i.e., vtkRenderer and subclasses). The list is unsorted and 
// duplicate entries are not prevented.

// .SECTION see also
// vtkRenderer vtkCollection

#ifndef __vtkRendererCollection_h
#define __vtkRendererCollection_h

#include "vtkCollection.h"
#include "vtkRenderer.h" // Needed for static cast

class VTK_RENDERING_EXPORT vtkRendererCollection : public vtkCollection
{
 public:
  static vtkRendererCollection *New();
  vtkTypeMacro(vtkRendererCollection,vtkCollection);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a Renderer to the list.
  void AddItem(vtkRenderer *a)
    {
    this->vtkCollection::AddItem(a);
    }
  
  
  // Description:
  // Get the next Renderer in the list. Return NULL when at the end of the 
  // list.
  vtkRenderer *GetNextItem() {
    return static_cast<vtkRenderer *>(this->GetNextItemAsObject());};

  // Description:
  // Forward the Render() method to each renderer in the list.
  void Render();

  // Description:
  // Get the first Renderer in the list. Return NULL when at the end of the 
  // list.
  vtkRenderer *GetFirstRenderer();

  //BTX
  // Description: 
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth. 
  vtkRenderer *GetNextRenderer(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkRenderer *>(this->GetNextItemAsObject(cookie));};
  //ETX

protected:  
  vtkRendererCollection() {};
  ~vtkRendererCollection() {};

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkRendererCollection(const vtkRendererCollection&);  // Not implemented.
  void operator=(const vtkRendererCollection&);  // Not implemented.
};


#endif
