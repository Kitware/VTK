/*=========================================================================

  Program:   Visualization Library
  Module:    XglrProp.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlXglrProperty - Suns XGL property
// .SECTION Description
// vlXglrProperty is a concrete implementation of the abstract class 
// vlProperty. vlXglrProperty interfaces to Suns XGL rendering library.

#ifndef __vlXglrProperty_hh
#define __vlXglrProperty_hh

#include "Property.hh"

class vlXglrRenderer;

class vlXglrProperty : public vlProperty
{
 public:
  char *GetClassName() {return "vlXglrProperty";};

  void Render(vlRenderer *ren);
  void Render(vlXglrRenderer *ren);
};

#endif
