/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Threshld.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkThreshold - extracts cells where scalar value of every point in cell satisfies threshold criterion
// .SECTION Description
// vtkThreshold is a filter that extracts cells from any dataset type that 
// satisfy a threshold criterion. A cell satisfies the criterion if the 
// scalar value of every point satisfies the criterion. The criterion can 
// take three forms: greater than a particular value, less than a particular 
// value, or between two values. The output of this filter is an unstructured 
// grid.
// .SECTION See Also
// vtkThresholdPoints, vtkThresholdTextureCoords

#ifndef __vtkThreshold_h
#define __vtkThreshold_h

#include "DS2UGrid.hh"

class vtkThreshold : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkThreshold();
  ~vtkThreshold() {};
  char *GetClassName() {return "vtkThreshold";};
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
  int (vtkThreshold::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
};

#endif


