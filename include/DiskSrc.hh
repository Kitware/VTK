/*=========================================================================

  Program:   Visualization Library
  Module:    DiskSrc.hh
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
// .NAME vlDiskSource - create a disk with hole in center
// .SECTION Description
// vlDiskSource creates a polygonal disk with a hole in the center. The 
// disk has zero height. The user can specify the inner and outer radius
// of the disk, and the radial and circumferential resolution of the 
// polygonal representation.

#ifndef __vlDiskSource_h
#define __vlDiskSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlDiskSource : public vlPolySource 
{
public:
  vlDiskSource();
  char *GetClassName() {return "vlDiskSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(InnerRadius,float,0.0,LARGE_FLOAT)
  vlGetMacro(InnerRadius,float);

  vlSetClampMacro(OuterRadius,float,0.0,LARGE_FLOAT)
  vlGetMacro(OuterRadius,float);

  vlSetClampMacro(RadialResolution,int,1,LARGE_INTEGER)
  vlGetMacro(RadialResolution,int);

  vlSetClampMacro(CircumferentialResolution,int,3,LARGE_INTEGER)
  vlGetMacro(CircumferentialResolution,int);

protected:
  void Execute();
  float InnerRadius;
  float OuterRadius;
  int RadialResolution;
  int CircumferentialResolution;

};

#endif


