/*=========================================================================

  Program:   Visualization Library
  Module:    RenderC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlRendererCollection - a list of renderers
// .SECTION Description
// vlRendererCollection represents and provides methods to manipulate list of
// renderers (i.e., vlRenderer and subclasses). The list is unsorted and 
// duplicate entries are not prevented.

#ifndef __vlRendererCollection_hh
#define __vlRendererColleciton_hh

#include "Collect.hh"
#include "Renderer.hh"

class vlRendererCollection : public vlCollection
{
 public:
  char *GetClassName() {return "vlRendererCollection";};

  void AddItem(vlRenderer *a);
  void RemoveItem(vlRenderer *a);
  int IsItemPresent(vlRenderer *a);
  vlRenderer *GetItem(int num);
  void Render();
};

// Description:
// Add an renderer to the list.
inline void vlRendererCollection::AddItem(vlRenderer *a) 
{
  this->vlCollection::AddItem((vlObject *)a);
}

// Description:
// Remove an renderer from the list.
inline void vlRendererCollection::RemoveItem(vlRenderer *a) 
{
  this->vlCollection::RemoveItem((vlObject *)a);
}

// Description:
// Determine whether a particular renderer is present. Returns its position
// in the list.
inline int vlRendererCollection::IsItemPresent(vlRenderer *a) 
{
  return this->vlCollection::IsItemPresent((vlObject *)a);
}

// Description:
// Get an renderer in the list at a particular location (1<=num<=number items).
inline vlRenderer *vlRendererCollection::GetItem(int num) 
{
  return (vlRenderer *)(this->vlCollection::GetItem(num));
}

#endif
