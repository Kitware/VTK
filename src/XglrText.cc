/*=========================================================================

  Program:   Visualization Library
  Module:    XglrText.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "XglrRenW.hh"
#include "XglrRen.hh"
#include "XglrText.hh"

// shared increasing counter
long vlXglrTexture::GlobalIndex = 0;

// Description:
// Initializes an instance, generates a unique index.
vlXglrTexture::vlXglrTexture()
{
  this->GlobalIndex++;
  this->Index = this->GlobalIndex;
}

// Description:
// Implement base class method.
void vlXglrTexture::Load(vlRenderer *ren)
{
  this->Load((vlXglrRenderer *)ren);
}

// Description:
// Actual Texture load method.
void vlXglrTexture::Load(vlXglrRenderer *ren)
{
  // currently a nop
}
