/*=========================================================================

  Program:   Visualization Library
  Module:    TransPF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTransformPolyFilter - transform points and associated normals and vectors for polygonal dataset
// .SECTION Description
// vlTransformPolyFilter is a filter to transform point coordinates and 
// associated point normals and vectors. Other point data is passed
// through the filter. This filter is specialized for polygonal data. See
// vlTransformFilter for more general data.
//   (An alternative method of transformation is to use vlActors methods
// to scale, rotate, and translate objects. The difference between the
// two methods is that vlActor's transformation simply effects where
// objects are rendered (via the graphics pipeline), whereas
// vlTransformPolyFilter actually modifies point coordinates in the 
// visualization pipeline. This is necessary for some objects 
// (e.g., vlProbeFilter) that require point coordinates as input).

#ifndef __vlTransformPolyFilter_h
#define __vlTransformPolyFilter_h

#include "P2PF.hh"
#include "Trans.hh"

class vlTransformPolyFilter : public vlPolyToPolyFilter
{
public:
  vlTransformPolyFilter() : Transform(NULL) {};
  ~vlTransformPolyFilter() {};
  char *GetClassName() {return "vlTransformPolyFilter";};
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


