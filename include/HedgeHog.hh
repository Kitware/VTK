/*=========================================================================

  Program:   Visualization Library
  Module:    HedgeHog.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlHedgeHog - create oriented lines from vector data
// .SECTION Description
// vlHedgeHog creates oriented lines from the input data set. Line length
// is controlled by vector magnitude times scale factor. Vectors are
// colored by scalar data, if available.

#ifndef __vlHedgeHog_h
#define __vlHedgeHog_h

#include "DS2PolyF.hh"

class vlHedgeHog : public vlDataSetToPolyFilter
{
public:
  vlHedgeHog() : ScaleFactor(1.0) {};
  ~vlHedgeHog() {};
  char *GetClassName() {return "vlHedgeHog";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set scale factor to control size of oriented lines.
  vlSetMacro(ScaleFactor,float);
  vlGetMacro(ScaleFactor,float);

protected:
  void Execute();
  float ScaleFactor;

};

#endif


