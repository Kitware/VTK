/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ThrshPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkThresholdPoints - extracts points whose scalar value satisfies threshold criterion
// .SECTION Description
// vtkThresholdPoints is a filter that extracts points from a dataset that 
// satisfy a threshold criterion. The criterion can take three forms:
// 1) greater than a particular value, 2) less than a particular value, and
// 2) between a particular value. The output of the filter is polygonal data.
// .SECTION See Also
// vtkThreshold

#ifndef __vtkThresholdPoints_h
#define __vtkThresholdPoints_h

#include "DS2PolyF.hh"

class vtkThresholdPoints : public vtkDataSetToPolyFilter
{
public:
  vtkThresholdPoints();
  ~vtkThresholdPoints() {};
  char *GetClassName() {return "vtkThresholdPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void ThresholdByLower(float lower);
  void ThresholdByUpper(float upper);
  void ThresholdBetween(float lower, float upper);
  
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

protected:
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
};

#endif


