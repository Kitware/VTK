/*=========================================================================

  Program:   Visualization Library
  Module:    SbrProp.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlSbrProperty - HP starbase property
// .SECTION Description
// vlSbrProperty is a concrete implementation of the abstract class vlProperty.
// vlSbrProperty interfaces to the Hewlett-Packard starbase rendering library.

#ifndef __vlSbrProperty_hh
#define __vlSbrProperty_hh

#include "Property.hh"

class vlSbrRenderer;

class vlSbrProperty : public vlProperty
{
 public:
  char *GetClassName() {return "vlSbrProperty";};

  void Render(vlRenderer *ren);
  void Render(vlSbrRenderer *ren);
};

#endif
