/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ProbeF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkProbeFilter - compute data values at specified point locations
// .SECTION Description
// vtkProbeFilter is a filter that computes point attributes (e.g., scalars,
// vectors, etc.) at point positions in the input. The point positions
// are obtained from the points in the source object.

#ifndef __vtkProbeFilter_h
#define __vtkProbeFilter_h

#include "DS2DSF.hh"

class vtkProbeFilter : public vtkDataSetToDataSetFilter
{
public:
  vtkProbeFilter();
  ~vtkProbeFilter() {};
  char *GetClassName() {return "vtkProbeFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Update();
  void Initialize();

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used.
  vtkSetObjectMacro(Source,vtkDataSet);
  vtkGetObjectMacro(Source,vtkDataSet);

protected:
  void Execute();
  vtkDataSet *Source;

};

#endif


