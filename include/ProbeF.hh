/*=========================================================================

  Program:   Visualization Library
  Module:    ProbeF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlProbeFilter - compute data values at specified point locations
// .SECTION Description
// vlProbeFilter is a filter that computes point attributes (e.g., scalars,
// vectors, etc.) at point positions in the input. The point positions
// are obtained from the points in the source object.

#ifndef __vlProbeFilter_h
#define __vlProbeFilter_h

#include "DS2DSF.hh"

class vlProbeFilter : public vlDataSetToDataSetFilter
{
public:
  // vlProbeFilter();
  // ~vlProbeFilter();
  char *GetClassName() {return "vlProbeFilter";};
  // void PrintSelf(ostream& os, vlIndent indent);

  void Update();
  void Initialize();

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used.
  vlSetObjectMacro(Source,vlDataSet);
  vlGetObjectMacro(Source,vlDataSet);

protected:
  void Execute();
  vlDataSet *Source;

};

#endif


