/*=========================================================================

  Program:   Visualization Library
  Module:    TransF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTransformFilter - transform points and associated normals and vectors
// .SECTION Description
// vlTransformFilter is a filter to transform point coordinates and 
// associated point normals and vectors. Other point data is passed
// through the filter.
//   (An alternative method of transformation is to use vlActors methods
// to scale, rotate, and translate objects. The difference between the
// two methods is that vlActor's transformation simply effects where
// objects are rendered (via the graphics pipeline), whereas
// vlTransformFilter actually modifies point coordinates in the 
// visualization pipeline. This is necessary for some objects 
// (e.g., vlProbeFilter) that require point coordinates as input).
// .EXAMPLE XFormSph.cc

#ifndef __vlTransformFilter_h
#define __vlTransformFilter_h

#include "PtS2PtSF.hh"
#include "Trans.hh"

class vlTransformFilter : public vlPointSetToPointSetFilter
{
public:
  vlTransformFilter() : Transform(NULL) {};
  ~vlTransformFilter() {};
  char *GetClassName() {return "vlTransformFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  unsigned long int GetMTime();

  // Description:
  // Specify the transform object used to transform points.
  vlSetObjectMacro(Transform,vlTransform);
  vlGetObjectMacro(Transform,vlTransform);

protected:
  void Execute();
  vlTransform *Transform;
};

#endif


