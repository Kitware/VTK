/*=========================================================================

  Program:   Visualization Library
  Module:    RenderC.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include <iostream.h>
#include "RenderC.hh"

void vlRendererCollection::Render()
{
  int i;
  vlRenderer *elem;

  for (i = 1; i <= this->GetNumberOfItems(); i++)
    {
    elem = this->GetItem(i);
    elem->Render();
    }
}

