/*=========================================================================

  Program:   Visualization Library
  Module:    DCubes.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDividingCubes - create points lying on iso-surface
// .SECTION Description
// vlDividingCubes is a filter that generates points laying on a surface
// of constant scalar value (i.e., an iso-surface). Dense point clouds (i.e.,
// at screen resolution) will appear as a surface. Less dense clouds can be 
// used as a source to generate streamlines or to generate "transparent"
// surfaces.

#ifndef __vlDividingCubes_h
#define __vlDividingCubes_h

#include "SPt2Poly.hh"

class vlDividingCubes : public vlStructuredPointsToPolyDataFilter
{
public:
  vlDividingCubes();
  ~vlDividingCubes() {};
  char *GetClassName() {return "vlDividingCubes";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set iso-surface value.
  vlSetMacro(Value,float);
  vlGetMacro(Value,float);

  // Description:
  // Specifysub-voxel size at which to generate point..
  vlSetClampMacro(Distance,float,1.0e-06,LARGE_FLOAT);
  vlGetMacro(Distance,float);

  // Description:
  // Every "Increment" point is added to the list of points. This parameter, if
  // set to a large value, can be used to limit the number of points while
  // retaining good accuracy.
  vlSetClampMacro(Increment,int,1,LARGE_INTEGER);
  vlGetMacro(Increment,int);

protected:
  void Execute();
  void SubDivide(float origin[3], float h[3], vlFloatScalars &values);
  void AddPoint(float x[3]);

  float Value;
  float Distance;
  int Increment;

  // wworking variables
  int Count;
  vlFloatPoints *NewPts;
  vlCellArray *NewVerts;
};

#endif


