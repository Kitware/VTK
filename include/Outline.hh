/*=========================================================================

  Program:   Visualization Library
  Module:    Outline.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlOutlineSource - create wireframe wireframe outline around bounding box
// .SECTION Description
// vlOutlineSource creates a wireframe outline around a user specified 
// bounding box.

#ifndef __vlOutlineSource_h
#define __vlOutlineSource_h

#include "PolySrc.hh"

class vlOutlineSource : public vlPolySource 
{
public:
  vlOutlineSource();
  ~vlOutlineSource() {};
  char *GetClassName() {return "vlOutlineSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify the bounding box for this object.
  vlSetVectorMacro(Bounds,float,6);
  vlGetVectorMacro(Bounds,float,6);

protected:
  void Execute();
  float Bounds[6];
};

#endif


