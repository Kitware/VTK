/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RenderC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkRendererCollection - a list of renderers
// .SECTION Description
// vtkRendererCollection represents and provides methods to manipulate list of
// renderers (i.e., vtkRenderer and subclasses). The list is unsorted and 
// duplicate entries are not prevented.

#ifndef __vtkRendererCollection_hh
#define __vtkRendererCollection_hh

#include "Collect.hh"
#include "Renderer.hh"

class vtkRendererCollection : public vtkCollection
{
 public:
  char *GetClassName() {return "vtkRendererCollection";};

  void AddItem(vtkRenderer *a);
  void RemoveItem(vtkRenderer *a);
  int IsItemPresent(vtkRenderer *a);
  vtkRenderer *GetNextItem();
  void Render();
};

// Description:
// Add an renderer to the list.
inline void vtkRendererCollection::AddItem(vtkRenderer *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove an renderer from the list.
inline void vtkRendererCollection::RemoveItem(vtkRenderer *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular renderer is present. Returns its position
// in the list.
inline int vtkRendererCollection::IsItemPresent(vtkRenderer *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next renderer in the list.
inline vtkRenderer *vtkRendererCollection::GetNextItem() 
{
  return (vtkRenderer *)(this->vtkCollection::GetNextItem());
}

#endif
