/*=========================================================================

  Program:   Visualization Library
  Module:    ProbeF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Probe source points into input data.
//
#ifndef __vlProbeFilter_h
#define __vlProbeFilter_h

#include "DS2DSF.hh"

class vlProbeFilter : public vlDataSetToDataSetFilter
{
public:
  vlProbeFilter();
  ~vlProbeFilter();
  char *GetClassName() {return "vlProbeFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Initialize();

  vlSetObjectMacro(Source,vlDataSet);
  vlGetObjectMacro(Source,vlDataSet);

protected:
  void Execute();
  vlDataSet *Source;

};

#endif


