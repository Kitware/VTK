/*=========================================================================

  Program:   Visualization Library
  Module:    ContourF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Contours arbitrary input
//
#ifndef __vlContourFilter_h
#define __vlContourFilter_h

#include "DS2PolyF.hh"

#define MAX_CONTOURS 256

class vlContourFilter : public vlDataSetToPolyFilter
{
public:
  vlContourFilter();
  ~vlContourFilter() {};
  char *GetClassName() {return "vlContourFilter";};

  void SetValue(int i, float value);
  vlGetVectorMacro(Values,float);

  void GenerateValues(int numContours, float range[2]);

protected:
  void Execute();
  float Values[MAX_CONTOURS];
  int NumberOfContours;
  float Range[2];
};

#endif


