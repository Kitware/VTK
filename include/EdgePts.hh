/*=========================================================================

  Program:   Visualization Library
  Module:    EdgePts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlEdgePoints - generate points on iso-surface
// .SECTION Description
// vlEdgePoints is a filter that takes as input any dataset and 
// generates for output a set of points that lie on an iso-surface. The 
// points are created by interpolation along cells edges whose end-points are 
// below and above the contour value.
// .SECTION Caveats
// vlEdgePoints can be considered a "poor man's" dividing cubes algorithm
// (see vlDividingCubes). Points are generated only on the edges of cells, 
// not in the interior, and at lower density than dividing cubes. However, it 
// is more general than dividing cubes since it treats any type of dataset.

#ifndef __vlEdgePoints_h
#define __vlEdgePoints_h

#include "DS2PolyF.hh"

class vlEdgePoints : public vlDataSetToPolyFilter
{
public:
  vlEdgePoints();
  ~vlEdgePoints();
  char *GetClassName() {return "vlEdgePoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set/get the contour value.
  vlSetMacro(Value,float);
  vlGetMacro(Value,float);

protected:
  void Execute();

  float Value;
};

#endif


