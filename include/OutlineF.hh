/*=========================================================================

  Program:   Visualization Library
  Module:    OutlineF.hh
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
// Outlines arbitrary input
//
#ifndef __vlOutlineFilter_h
#define __vlOutlineFilter_h

#include "DS2PolyF.hh"

class vlOutlineFilter : public vlDataSetToPolyFilter
{
public:
  vlOutlineFilter();
  ~vlOutlineFilter() {};
  char *GetClassName() {return "vlOutlineFilter";};

protected:
  void Execute();
};

#endif


