/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TransF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTransformFilter - transform points and associated normals and vectors
// .SECTION Description
// vtkTransformFilter is a filter to transform point coordinates and 
// associated point normals and vectors. Other point data is passed
// through the filter.
//   (An alternative method of transformation is to use vtkActors methods
// to scale, rotate, and translate objects. The difference between the
// two methods is that vtkActor's transformation simply effects where
// objects are rendered (via the graphics pipeline), whereas
// vtkTransformFilter actually modifies point coordinates in the 
// visualization pipeline. This is necessary for some objects 
// (e.g., vtkProbeFilter) that require point coordinates as input).
// .EXAMPLE XFormSph.cc

#ifndef __vtkTransformFilter_h
#define __vtkTransformFilter_h

#include "PtS2PtSF.hh"
#include "Trans.hh"

class vtkTransformFilter : public vtkPointSetToPointSetFilter
{
public:
  vtkTransformFilter() : Transform(NULL) {};
  ~vtkTransformFilter() {};
  char *GetClassName() {return "vtkTransformFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long int GetMTime();

  // Description:
  // Specify the transform object used to transform points.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);

protected:
  void Execute();
  vtkTransform *Transform;
};

#endif


