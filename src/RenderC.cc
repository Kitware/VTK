/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RenderC.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include "RenderC.hh"

// Description:
// Forward the Render() method to each renderer in the list.
void vtkRendererCollection::Render()
{
  vtkRenderer *ren;

  for ( this->InitTraversal(); ren = GetNextItem(); )
    {
    ren->Render();
    }
}

