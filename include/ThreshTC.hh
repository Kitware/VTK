/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ThreshTC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkThresholdTextureCoords - compute 1D, 2D, or 3D texture coordinates based on scalar threshold
// .SECTION Description
// vtkThresholdTextureCoords is a filter that generates texture coordinates for
// any input dataset type given a threshold criterion. The criterion can take 
// three forms: greater than a particular value, less than a particular value,
// or between two values. If the threshold criterion is satisfied, the texture
// coordinate component is set to 1.0. Otherwise, it is set to 0.0.
// .SECTION See Also
// vtkThreshold, vtkThresholdPoints

#ifndef __vtkThresholdTextureCoords_h
#define __vtkThresholdTextureCoords_h

#include "DS2DSF.hh"

class vtkThresholdTextureCoords : public vtkDataSetToDataSetFilter
{
public:
  vtkThresholdTextureCoords();
  ~vtkThresholdTextureCoords() {};
  char *GetClassName() {return "vtkThresholdTextureCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void ThresholdByLower(float lower);
  void ThresholdByUpper(float upper);
  void ThresholdBetween(float lower, float upper);
  
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

  vtkSetClampMacro(TextureDimension,int,1,3);
  vtkGetMacro(TextureDimension,int);

protected:
  // Usual data generation method
  void Execute();

  float LowerThreshold;
  float UpperThreshold;

  int TextureDimension;

  //BTX
  int (vtkThresholdTextureCoords::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
};

#endif


