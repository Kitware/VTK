/*=========================================================================

  Program:   Visualization Library
  Module:    ThreshTC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlThresholdTextureCoords - compute 1D, 2D, or 3D texture coordinates based on scalar threshold
// .SECTION Description
// vlThresholdTextureCoords is a filter that generates texture coordinates for
// any input dataset type given a threshold criterion. The criterion can take 
// three forms: greater than a particular value, less than a particular value,
// or between two values. If the threshold criterion is satisfied, the texture
// coordinate component is set to 1.0. Otherwise, it is set to 0.0.
// .SECTION See Also
// vlThreshold, vlThresholdPoints

#ifndef __vlThresholdTextureCoords_h
#define __vlThresholdTextureCoords_h

#include "DS2DSF.hh"

class vlThresholdTextureCoords : public vlDataSetToDataSetFilter
{
public:
  vlThresholdTextureCoords();
  ~vlThresholdTextureCoords() {};
  char *GetClassName() {return "vlThresholdTextureCoords";};
  void PrintSelf(ostream& os, vlIndent indent);

  void ThresholdByLower(float lower);
  void ThresholdByUpper(float upper);
  void ThresholdBetween(float lower, float upper);
  
  vlGetMacro(UpperThreshold,float);
  vlGetMacro(LowerThreshold,float);

  vlSetClampMacro(TextureDimension,int,1,3);
  vlGetMacro(TextureDimension,int);

protected:
  // Usual data generation method
  void Execute();

  float LowerThreshold;
  float UpperThreshold;

  int TextureDimension;

  //BTX
  int (vlThresholdTextureCoords::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
};

#endif


