/*=========================================================================

  Program:   Visualization Library
  Module:    P2PF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// PolyToPolyFilter are filters that take PolyData in and generate PolyData
//
#ifndef __vlPolyToPolyFilter_h
#define __vlPolyToPolyFilter_h

#include "PolyF.hh"
#include "PolyData.hh"

class vlPolyToPolyFilter : public vlPolyFilter, public vlPolyData 
{
public:
  void Update();
  char *GetClassName() {return "vlPolyToPolyFilter";};
};

#endif


