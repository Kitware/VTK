/*=========================================================================

  Program:   Visualization Library
  Module:    LightC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlLightC_hh
#define __vlLightC_hh

#include "Collect.hh"
#include "Light.hh"

class vlLightCollection : public vlCollection
{
 public:
  void AddItem(vlLight *a) {this->vlCollection::AddItem((vlObject *)a);};
  void RemoveItem(vlLight *a) 
    {this->vlCollection::RemoveItem((vlObject *)a);};
  int IsItemPresent(vlLight *a) 
    {return this->vlCollection::IsItemPresent((vlObject *)a);};
  vlLight *GetItem(int num) 
    { return (vlLight *)(this->vlCollection::GetItem(num));};
  char *GetClassName() {return "vlLightCollection";};
};

#endif

