/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBrownianPoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBrownianPoints - assign random vector to points
// .SECTION Description
// vtkBrownianPoints is a filter object that assigns a random vector (i.e.,
// magnitude and direction) to each point. The minimum and maximum speed
// values can be controlled by the user.

#ifndef __vtkBrownianPoints_h
#define __vtkBrownianPoints_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkBrownianPoints : public vtkDataSetToDataSetFilter
{
public:
  // Description:
  // Create instance with minimum speed 0.0, maximum speed 1.0.
  static vtkBrownianPoints *New();

  vtkTypeRevisionMacro(vtkBrownianPoints,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the minimum speed value.
  vtkSetClampMacro(MinimumSpeed,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(MinimumSpeed,float);

  // Description:
  // Set the maximum speed value.
  vtkSetClampMacro(MaximumSpeed,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(MaximumSpeed,float);

protected:
  vtkBrownianPoints();
  ~vtkBrownianPoints() {};

  void Execute();
  float MinimumSpeed;
  float MaximumSpeed;
private:
  vtkBrownianPoints(const vtkBrownianPoints&);  // Not implemented.
  void operator=(const vtkBrownianPoints&);  // Not implemented.
};

#endif


