/*=========================================================================

  Program:   Visualization Library
  Module:    Axes.hh
  Language:  C++
  Date:      7/20/94
  Version:   1.2

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlAxes - create an x-y-z axes
// .SECTION Description
// vlAxes creates three lines that form an x-y-z axes. The origin of the 
// axes is user specified (0,0,0 is default), and the size is specified 
// with a scale factor. Three scalar values are generated for the three 
// lines and can be used (via color map) to indicate a particular 
// coordinate axis.

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


