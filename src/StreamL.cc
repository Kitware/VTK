/*=========================================================================

  Program:   Visualization Library
  Module:    StreamL.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "StreamL.hh"

vlStreamLine::vlStreamLine()
{

}

// Description:
// Specify the start of the streamline in the cell coordinate system. That is,
// cellId and subId (if composite cell), and parametric coordinates.
void vlStreamLine::SetStartLocation(int cellId, int subId, float pcoords[3])
{
}

// Description:
// Get the starting location of the streamline in the cell corrdinate system.
int vlStreamLine::GetStartLocation(int& subId, float pcoords[3])
{
return 89;
}

// Description:
// Specify the start of the streamline in the global coordinate system. Search
// must be performed to find initial cell to strart integration from.
void vlStreamLine::SetStartPosition(float x[3])
{
}

// Description:
// Get the start position in global x-y-z coordinates.
float *vlStreamLine::GetStartPosition()
{
static float foo[3];
return foo;
}

void vlStreamLine::Execute()
{

}

void vlStreamLine::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStreamLine::GetClassName()))
    {
    vlDataSetToPolyFilter::PrintSelf(os,indent);

    os << indent << "";
    }

}


