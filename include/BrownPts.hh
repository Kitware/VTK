/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BrownPts.hh
  Language:  C++
  Date:      9/14/94
  Version:   1.1

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkBrownianPoints - assign random vector to points
// .SECTION Description
// vtkBrownianPoints is a filter object that assigns a random vector (i.e.,
// magnitude and direction) to each point. The minimum and maximum speed
// values can be controlled by the user.

#ifndef __vtkBrownianPoints_h
#define __vtkBrownianPoints_h

#include "DS2DSF.hh"

class vtkBrownianPoints : public vtkDataSetToDataSetFilter
{
public:
  vtkBrownianPoints();
  ~vtkBrownianPoints() {};
  char *GetClassName() {return "vtkBrownianPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the minimum speed value.
  vtkSetClampMacro(MinimumSpeed,float,0.0,LARGE_FLOAT);
  vtkGetMacro(MinimumSpeed,float);

  // Description:
  // Set the maximum speed value.
  vtkSetClampMacro(MaximumSpeed,float,0.0,LARGE_FLOAT);
  vtkGetMacro(MaximumSpeed,float);

protected:
  void Execute();
  float MinimumSpeed;
  float MaximumSpeed;
};

#endif


