/*=========================================================================

  Program:   Visualization Library
  Module:    BrownPts.hh
  Language:  C++
  Date:      9/14/94
  Version:   1.1

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlBrownianPoints - assign random vector to points
// .SECTION Description
// vlBrownianPoints is a filter object that assigns a random vector (i.e.,
// magnitude and direction) to each point. The minimum and maximum speed
// values can be controlled by the user.

#ifndef __vlBrownianPoints_h
#define __vlBrownianPoints_h

#include "DS2DSF.hh"

class vlBrownianPoints : public vlDataSetToDataSetFilter
{
public:
  vlBrownianPoints();
  ~vlBrownianPoints() {};
  char *GetClassName() {return "vlBrownianPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the minimum speed value.
  vlSetClampMacro(MinimumSpeed,float,0.0,LARGE_FLOAT);
  vlGetMacro(MinimumSpeed,float);

  // Description:
  // Set the maximum speed value.
  vlSetClampMacro(MaximumSpeed,float,0.0,LARGE_FLOAT);
  vlGetMacro(MaximumSpeed,float);

protected:
  void Execute();
  float MinimumSpeed;
  float MaximumSpeed;
};

#endif


