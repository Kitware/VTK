/*=========================================================================

  Program:   Visualization Library
  Module:    Threshld.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlThreshold - extracts cells where scalar value of every point in cell satisfies threshold criterion
// .SECTION Description
// vlThreshold is a filter that extracts cells from a dataset that satisfy a
// threshold criterion. A cell satisfies the criterion if the scalar value
// of every point satisfies the criterion. The criterion can take three forms:
// greater than a particular value, less than a particular value, between a
// particular value.

#ifndef __vlThreshold_h
#define __vlThreshold_h

#include "DS2UGrid.hh"

class vlThreshold : public vlDataSetToUnstructuredGridFilter
{
public:
  vlThreshold();
  ~vlThreshold() {};
  char *GetClassName() {return "vlThreshold";};
  void PrintSelf(ostream& os, vlIndent indent);

  void ThresholdByLower(float lower);
  void ThresholdByUpper(float upper);
  void ThresholdBetween(float lower, float upper);
  
  vlGetMacro(UpperThreshold,float);
  vlGetMacro(LowerThreshold,float);

protected:
  // Usual data generation method
  void Execute();

  float LowerThreshold;
  float UpperThreshold;

  //BTX
  int (vlThreshold::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
};

#endif


