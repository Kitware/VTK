/*=========================================================================

  Program:   Visualization Library
  Module:    Stripper.hh
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
// Creates triangle strips from input polygons / triangle strips
//
#ifndef __vlStripper_h
#define __vlStripper_h

#include "P2PF.hh"

class vlStripper : public vlPolyToPolyFilter
{
public:
  vlStripper();
  ~vlStripper() {};
  char *GetClassName() {return "vlStripper";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(MaximumStripLength,int,4,MAX_CELL_SIZE);
  vlGetMacro(MaximumStripLength,int);

protected:
  // Usual data generation method
  void Execute();

  int MaximumStripLength;
};

#endif


