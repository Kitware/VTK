/*=========================================================================

  Program:   Visualization Library
  Module:    SbrLgt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlSbrLight_hh
#define __vlSbrLight_hh

#include "Light.hh"
#include "kgl.h"

class vlSbrRenderer;

class vlSbrLight : public vlLight
{
protected:
  
public:
  char *GetClassName() {return "vlSbrLight";};
  void Render(vlRenderer *ren,int light_index); // overides base 
  void Render(vlSbrRenderer *ren,int light_index); // real function 
  
};

#endif

