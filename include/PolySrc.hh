/*=========================================================================

  Program:   Visualization Library
  Module:    PolySrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolySource - abstract class whose subclasses generate polygonal data
// .SECTION Description
// vlPolySource is an abstract class whose subclasses generate polygonal data.

#ifndef __vlPolySource_h
#define __vlPolySource_h

#include "Source.hh"
#include "PolyData.hh"

class vlPolySource : public vlSource, public vlPolyData 
{
public:
  char *GetClassName() {return "vlPolySource";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Modified();
  unsigned long int GetMTime();
  void Update();
};

#endif


