/*=========================================================================

  Program:   Visualization Library
  Module:    Tetra.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Computational class for tetras.
//
#ifndef __vlTetra_h
#define __vlTetra_h

#include "Cell.hh"

class vlTetra : public vlCell
{
public:
  vlTetra() {};
  char *GetClassName() {return "vlTetra";};

  float DistanceToPoint(float *x);

};

#endif


