/*=========================================================================

  Program:   Visualization Library
  Module:    GlrProp.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlGlrProperty_hh
#define __vlGlrProperty_hh

#include "Property.hh"
#include "gl.h"

class vlGlrRenderer;

class vlGlrProperty : public vlProperty
{
 public:
  char *GetClassName() {return "vlGlrProperty";};
  void Render(vlRenderer *ren); // overides base 
  void Render(vlGlrRenderer *ren);
};

#endif
