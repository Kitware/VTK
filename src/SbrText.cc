/*=========================================================================

  Program:   Visualization Library
  Module:    SbrText.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "SbrRen.hh"
#include "SbrText.hh"

// shared increasing counter
long vlSbrTexture::GlobalIndex = 0;

// Description:
// Initializes an instance, generates a unique index.
vlSbrTexture::vlSbrTexture()
{
  this->GlobalIndex++;
  this->Index = this->GlobalIndex;
}

// Description:
// Implement base class method.
void vlSbrTexture::Load(vlTexture *txt, vlRenderer *ren)
{
  this->Load(txt, (vlSbrRenderer *)ren);
}

// Description:
// Actual Texture load method.
void vlSbrTexture::Load(vlTexture *txt, vlSbrRenderer *ren)
{
  // currently a nop
}
