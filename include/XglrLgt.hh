/*=========================================================================

  Program:   Visualization Library
  Module:    XglrLgt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlXglrLight - Light for Suns XGL
// .SECTION Description
// vlXglrLight is a concrete implementation of the abstract class vlLight.
// vlXglrLight interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vlXglrLight_hh
#define __vlXglrLight_hh

#include "Light.hh"

class vlXglrRenderer;

class vlXglrLight : public vlLight
{
protected:
  
public:
  char *GetClassName() {return "vlXglrLight";};

  void Render(vlRenderer *ren,int light_index);
  void Render(vlXglrRenderer *ren,int light_index);
  
};

#endif

