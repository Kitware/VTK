/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdPoints.h
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
// .NAME vtkThresholdPoints - extracts points whose scalar value satisfies threshold criterion
// .SECTION Description
// vtkThresholdPoints is a filter that extracts points from a dataset that 
// satisfy a threshold criterion. The criterion can take three forms:
// 1) greater than a particular value; 2) less than a particular value; or
// 3) between a particular value. The output of the filter is polygonal data.
// .SECTION See Also
// vtkThreshold

#ifndef __vtkThresholdPoints_h
#define __vtkThresholdPoints_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkThresholdPoints : public vtkDataSetToPolyDataFilter
{
public:
  static vtkThresholdPoints *New();
  vtkTypeRevisionMacro(vtkThresholdPoints,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Criterion is cells whose scalars are less than lower threshold.
  void ThresholdByLower(float lower);

  // Description:
  // Criterion is cells whose scalars are less than upper threshold.
  void ThresholdByUpper(float upper);

  // Description:
  // Criterion is cells whose scalars are between lower and upper thresholds.
  void ThresholdBetween(float lower, float upper);

  // Description:
  // Get the upper and lower thresholds.
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

protected:
  vtkThresholdPoints();
  ~vtkThresholdPoints() {};

  // Usual data generation method
  void Execute();

  float LowerThreshold;
  float UpperThreshold;

  //BTX
  int (vtkThresholdPoints::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
private:
  vtkThresholdPoints(const vtkThresholdPoints&);  // Not implemented.
  void operator=(const vtkThresholdPoints&);  // Not implemented.
};

#endif
