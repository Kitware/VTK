/*=========================================================================

  Program:   Visualization Library
  Module:    SGridSrc.hh
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
// Abstract class for specifying behavior of structured grid sources
//
#ifndef __vlStructuredGridSource_h
#define __vlStructuredGridSource_h

#include "Source.hh"
#include "SGrid.hh"

class vlStructuredGridSource : public vlSource, public vlStructuredGrid 
{
public:
  char *GetClassName() {return "vlStructuredGridSource";};
  void PrintSelf(ostream& os, vlIndent indent);
  void Update();
};

#endif


