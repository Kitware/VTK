/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDiskSource - create a disk with hole in center
// .SECTION Description
// vtkDiskSource creates a polygonal disk with a hole in the center. The 
// disk has zero height. The user can specify the inner and outer radius
// of the disk, and the radial and circumferential resolution of the 
// polygonal representation. 
// .SECTION See Also
// vtkLinearExtrusionFilter

#ifndef __vtkDiskSource_h
#define __vtkDiskSource_h

#include "vtkPolyDataSource.h"
#include "vtkPolyData.h"

class VTK_GRAPHICS_EXPORT vtkDiskSource : public vtkPolyDataSource 
{
public:
  static vtkDiskSource *New();
  vtkTypeRevisionMacro(vtkDiskSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify inner radius of hole in disc.
  vtkSetClampMacro(InnerRadius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(InnerRadius,float);

  // Description:
  // Specify outer radius of disc.
  vtkSetClampMacro(OuterRadius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(OuterRadius,float);

  // Description:
  // Set the number of points in radius direction.
  vtkSetClampMacro(RadialResolution,int,1,VTK_LARGE_INTEGER)
  vtkGetMacro(RadialResolution,int);

  // Description:
  // Set the number of points in circumferential direction.
  vtkSetClampMacro(CircumferentialResolution,int,3,VTK_LARGE_INTEGER)
  vtkGetMacro(CircumferentialResolution,int);

protected:
  vtkDiskSource();
  ~vtkDiskSource() {};

  void Execute();
  float InnerRadius;
  float OuterRadius;
  int RadialResolution;
  int CircumferentialResolution;

private:
  vtkDiskSource(const vtkDiskSource&);  // Not implemented.
  void operator=(const vtkDiskSource&);  // Not implemented.
};

#endif


