/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TubeF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTubeFilter - filter that generates tubes around lines
// .SECTION Description
// vtkTubeFilter is a filter that generates a tube around each input line. 
// The tubes are made up of triangle strips and rotate around the tube with
// the rotation of the line normals. (If no normals are present, they are
// computed automatically). The radius of the tube can be set to vary with 
// scalar value. If the scalar value is speed (i.e., magnitude of velocity),
// the variation of the tube radius is such that it preserves mass flux in
// incompressible flow. The number of sides for the tube can also be 
// specified.
// .SECTION Caveats
// The number of tube sides must be greater than 3. If you wish to use fewer
// sides (i.e., a ribbon), use vtkRibbonFilter.
//    The input line must not have duplicate points, or normals at points that
// are parallel to the incoming/outgoing line segments. (Duplicate points
// can be removed with vtkCleanPolyData).

#ifndef __vtkTubeFilter_h
#define __vtkTubeFilter_h

#include "P2PF.hh"

class vtkTubeFilter : public vtkPolyToPolyFilter
{
public:
  vtkTubeFilter();
  ~vtkTubeFilter() {};
  char *GetClassName() {return "vtkTubeFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the minimum tube radius (minimum because the tube radius may vary).
  vtkSetClampMacro(Radius,float,0.0,LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Turn on/off the variation of tube radius with scalar value.
  vtkSetMacro(VaryRadius,int);
  vtkGetMacro(VaryRadius,int);
  vtkBooleanMacro(VaryRadius,int);

  // Description:
  // Set the number of sides for the tube. At a minimum, number of sides is 3.
  vtkSetClampMacro(NumberOfSides,int,3,LARGE_INTEGER);
  vtkGetMacro(NumberOfSides,int);

  // Description:
  // Set the maximum tube radius in terms of a multiple of the minimum radius.
  vtkSetMacro(RadiusFactor,float);
  vtkGetMacro(RadiusFactor,float);

protected:
  // Usual data generation method
  void Execute();

  float Radius; //minimum radius of tube
  int VaryRadius; //controls whether radius varies with scalar data
  int NumberOfSides; //number of sides to create tube
  float RadiusFactor; //maxium allowablew radius
};

#endif


