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
// .NAME vlContourFilter - generate iso-surfaces/iso-lines from scalar values
// .SECTION Description
// vlContourFilter is a filter that takes as input any data set and generates
// on output iso-surfaces and/or iso-lines. The exact form of the output 
// depends upon the dimensionality of the input data. Data consisting of 
// 3D cells will generate iso-surfaces, data consisting of 2D cells will 
// generate iso-lines, and data with 1D or 0D cells will generate 
// iso-points. Combinations of output type is possible if the input 
// dimension is mixed.
// .SECTION Caveats
// vlContourFilter uses variations of marching cubes to generate output
// primitives. The output primitives are disjoint - that is, points may
// be generated that are coincident but distinct. Use vlCleanPolyData to
// merge coincident points.

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
  void PrintSelf(ostream& os, vlIndent indent);

  void SetValue(int i, float value);

  // Description:
  // Return pointer to array of contour values (size of numContours).
  vlGetVectorMacro(Values,float,MAX_CONTOURS);

  void GenerateValues(int numContours, float range[2]);

protected:
  void Execute();
  float Values[MAX_CONTOURS];
  int NumberOfContours;
  float Range[2];
};

#endif


