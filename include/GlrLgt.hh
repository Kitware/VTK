/*=========================================================================

  Program:   Visualization Library
  Module:    GlrLgt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlGlrLight - SGI gl light
// .SECTION Description
// vlGlrLight is a concrete implementation of the abstract class vlLight.
// vlGlrLight interfaces to the Silicon Graphics gl rendering library.

#ifndef __vlGlrLight_hh
#define __vlGlrLight_hh

#include "Light.hh"

class vlGlrRenderer;

class vlGlrLight : public vlLight
{
protected:
  
public:
  char *GetClassName() {return "vlGlrLight";};

  void Render(vlRenderer *ren,int light_index);
  void Render(vlGlrRenderer *ren,int light_index);
};

#endif

