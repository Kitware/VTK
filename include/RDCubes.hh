/*=========================================================================

  Program:   Visualization Library
  Module:    RDCubes.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkRecursiveDividingCubes - create points lying on iso-surface (using recursive approach)
// .SECTION Description
// vtkRecursiveDividingCubes is a filter that generates points laying on a 
// surface of constant scalar value (i.e., an iso-surface). Dense point 
// clouds (i.e., at screen resolution) will appear as a surface. Less dense 
// clouds can be used as a source to generate streamlines or to generate 
// "transparent" surfaces.
//    This implementation differs from vtkDividingCubes in that it uses a 
// recursive procedure. For non-cubical voxels, this can result in generating 
// more points than the procedural implementation of vtkDividingCubes. This is
// because the recursive procedure divides voxels by multiples of powers of 
// two. This can over constrain subdivision. One of the advantages of the 
// recursive technique is that the recursion is terminated earlier which in
// some cases can be more efficient.
// .SECTION See Also
// vtkDividingCubes, vtkContourFilter, vtkMarchingCubes

#ifndef __vtkRecursiveDividingCubes_h
#define __vtkRecursiveDividingCubes_h

#include "SPt2Poly.hh"

class vtkRecursiveDividingCubes : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkRecursiveDividingCubes();
  ~vtkRecursiveDividingCubes() {};
  char *GetClassName() {return "vtkRecursiveDividingCubes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set iso-surface value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

  // Description:
  // Specify sub-voxel size at which to generate point..
  vtkSetClampMacro(Distance,float,1.0e-06,LARGE_FLOAT);
  vtkGetMacro(Distance,float);

  // Description:
  // Every "Increment" point is added to the list of points. This parameter, if
  // set to a large value, can be used to limit the number of points while
  // retaining good accuracy.
  vtkSetClampMacro(Increment,int,1,LARGE_INTEGER);
  vtkGetMacro(Increment,int);

protected:
  void Execute();
  void SubDivide(float origin[3], float h[3], float values[8]);
  void AddPoint(float x[3]);

  float Value;
  float Distance;
  int Increment;

  // working variable
  int Count;
};

#endif


