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
#ifndef __vlRendererCollection_hh
#define __vlRendererColleciton_hh

#include "Collect.hh"
#include "Renderer.hh"

class vlRendererCollection : public vlCollection
{
 public:
  void AddItem(vlRenderer *a) 
    {this->vlCollection::AddItem((vlObject *)a);};
  void RemoveItem(vlRenderer *a) 
    {this->vlCollection::RemoveItem((vlObject *)a);};
  int IsItemPresent(vlRenderer *a) 
    {return this->vlCollection::IsItemPresent((vlObject *)a);};
  vlRenderer *GetItem(int num) 
    { return (vlRenderer *)(this->vlCollection::GetItem(num));};
  char *GetClassName() {return "vlRendererCollection";};

  void Render();
};

#endif
