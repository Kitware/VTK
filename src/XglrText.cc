/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrText.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "XglrRenW.hh"
#include "XglrRen.hh"
#include "XglrText.hh"

// shared increasing counter
long vtkXglrTexture::GlobalIndex = 0;

// Description:
// Initializes an instance, generates a unique index.
vtkXglrTexture::vtkXglrTexture()
{
  this->GlobalIndex++;
  this->Index = this->GlobalIndex;
}

// Description:
// Implement base class method.
void vtkXglrTexture::Load(vtkTexture *txt, vtkRenderer *ren)
{
  this->Load(txt, (vtkXglrRenderer *)ren);
}

// Description:
// Actual Texture load method.
void vtkXglrTexture::Load(vtkTexture *txt, vtkXglrRenderer *ren)
{
  // currently a nop
}
