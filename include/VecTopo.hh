/*=========================================================================

  Program:   Visualization Library
  Module:    VecTopo.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkVectorTopology - mark points where the vector field vanishes (singularities exist).
// .SECTION Description
// vtkVectorTopology is a filter that marks points where the vector field 
// vanishes. At these points various important flow features are found, 
// including regions of circulation, separation, etc. The region around these
// areas are good places to start streamlines. (The vector field vanishes in 
// cells where the x-y-z vector components each pass through zero).
//    The output of this filter is a set of vertices. These vertices mark the 
// vector field singularities. You can use an object like vtkGlyph3D to place
// markers at these points, or use the vertices to initiate streamlines.
//    The Distance instance variable controls the accuracy of placement of the
// vertices. Smaller values result in greater execution times.
//    The input to this filter is any dataset type. The position of the 
// vertices is found by sampling the cell in parametric space. Sampling is
// repeated until the Distance criterion is satisfied.
// .SECTION See Also
// vtkGlyph3D, vtkStreamLine

#ifndef __vtkVectorTopology_h
#define __vtkVectorTopology_h

#include "DS2PolyF.hh"

#define MAX_CONTOURS 256

class vtkVectorTopology : public vtkDataSetToPolyFilter
{
public:
  vtkVectorTopology();
  ~vtkVectorTopology();
  char *GetClassName() {return "vtkVectorTopology";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify distance from singularity to generate point.
  vtkSetClampMacro(Distance,float,1.0e-06,LARGE_FLOAT);
  vtkGetMacro(Distance,float);

protected:
  void Execute();

  float Distance;
};

#endif


