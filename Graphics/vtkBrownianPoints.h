/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBrownianPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#include "vtkDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkBrownianPoints : public vtkDataSetAlgorithm
{
public:
  // Description:
  // Create instance with minimum speed 0.0, maximum speed 1.0.
  static vtkBrownianPoints *New();

  vtkTypeMacro(vtkBrownianPoints,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the minimum speed value.
  vtkSetClampMacro(MinimumSpeed,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(MinimumSpeed,double);

  // Description:
  // Set the maximum speed value.
  vtkSetClampMacro(MaximumSpeed,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumSpeed,double);

protected:
  vtkBrownianPoints();
  ~vtkBrownianPoints() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double MinimumSpeed;
  double MaximumSpeed;
private:
  vtkBrownianPoints(const vtkBrownianPoints&);  // Not implemented.
  void operator=(const vtkBrownianPoints&);  // Not implemented.
};

#endif
