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
// .NAME vlSbrLight - HP starbase light
// .SECTION Description
// vlSbrLight is a concrete implementation of the abstract class vlLight.
// vlSbrLight interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vlSbrLight_hh
#define __vlSbrLight_hh

#include "LgtDev.hh"

class vlSbrRenderer;

class vlSbrLight : public vlLightDevice
{
protected:
  
public:
  char *GetClassName() {return "vlSbrLight";};

  void Render(vlLight *lgt, vlRenderer *ren,int light_index);
  void Render(vlLight *lgt, vlSbrRenderer *ren,int light_index);
  
};

#endif

