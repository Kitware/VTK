/*=========================================================================

  Program:   Visualization Library
  Module:    Axes.hh
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
// Create a x-y-z axes
//
#ifndef __vlAxes_h
#define __vlAxes_h

#include "PolySrc.hh"

class vlAxes : public vlPolySource 
{
public:
  vlAxes();
  char *GetClassName() {return "vlAxes";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float);

  vlSetMacro(ScaleFactor,float);
  vlGetMacro(ScaleFactor,float);

protected:
  void Execute();

  float Origin[3];
  float ScaleFactor;
};

#endif


