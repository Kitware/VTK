/*=========================================================================

  Program:   Visualization Library
  Module:    PropDev.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlDeviceObject - an object that requires hardware independence.
// .SECTION Description
// vlDeviceObject is the superclass that any device dependent object should
// use.  It allows a device independent object to create a device dependent
// object to execute hardware specific calls.

#ifndef __vlPropertyDevice_hh
#define __vlPropertyDevice_hh

#include "Object.hh"
class vlRenderer;
class vlProperty;

class vlPropertyDevice : public vlObject
{
public:
  char *GetClassName() {return "vlPropertyDevice";};
  virtual void Render(vlProperty *prp, vlRenderer *ren) = 0;
};

#endif
