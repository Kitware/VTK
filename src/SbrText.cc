/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrText.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "SbrRen.hh"
#include "SbrText.hh"

// shared increasing counter
long vtkSbrTexture::GlobalIndex = 0;

// Description:
// Initializes an instance, generates a unique index.
vtkSbrTexture::vtkSbrTexture()
{
  this->GlobalIndex++;
  this->Index = this->GlobalIndex;
}

// Description:
// Implement base class method.
void vtkSbrTexture::Load(vtkTexture *txt, vtkRenderer *ren)
{
  this->Load(txt, (vtkSbrRenderer *)ren);
}

// Description:
// Actual Texture load method.
void vtkSbrTexture::Load(vtkTexture *txt, vtkSbrRenderer *ren)
{
  // currently a nop
}
