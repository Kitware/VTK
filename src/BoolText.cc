/*=========================================================================

  Program:   Visualization Library
  Module:    BoolText.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "BoolText.hh"

vlBooleanTexture::vlBooleanTexture()
{
}

void vlBooleanTexture::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlBooleanTexture::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    os << indent << "Thickness: " << this->Thickness << "\n";
    }
}

