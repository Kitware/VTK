/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DiskSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDiskSource - create a disk with hole in center
// .SECTION Description
// vtkDiskSource creates a polygonal disk with a hole in the center. The 
// disk has zero height. The user can specify the inner and outer radius
// of the disk, and the radial and circumferential resolution of the 
// polygonal representation.

#ifndef __vtkDiskSource_h
#define __vtkDiskSource_h

#include "PolySrc.hh"

class vtkDiskSource : public vtkPolySource 
{
public:
  vtkDiskSource();
  char *GetClassName() {return "vtkDiskSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify radius of hole in disc.
  vtkSetClampMacro(InnerRadius,float,0.0,LARGE_FLOAT)
  vtkGetMacro(InnerRadius,float);

  // Description:
  // Specify radius of disc.
  vtkSetClampMacro(OuterRadius,float,0.0,LARGE_FLOAT)
  vtkGetMacro(OuterRadius,float);

  // Description:
  // Set the number of points in radius direction.
  vtkSetClampMacro(RadialResolution,int,1,LARGE_INTEGER)
  vtkGetMacro(RadialResolution,int);

  // Description:
  // Set the number of points in circumferential direction.
  vtkSetClampMacro(CircumferentialResolution,int,3,LARGE_INTEGER)
  vtkGetMacro(CircumferentialResolution,int);

protected:
  void Execute();
  float InnerRadius;
  float OuterRadius;
  int RadialResolution;
  int CircumferentialResolution;

};

#endif


