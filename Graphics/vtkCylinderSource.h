/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinderSource.h
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
// .NAME vtkCylinderSource - generate a cylinder centered at origin
// .SECTION Description
// vtkCylinderSource creates a polygonal cylinder centered at Center;
// The axis of the cylinder is aligned along the global y-axis.
// The height and radius of the cylinder can be specified, as well as the
// number of sides. It is also possible to control whether the cylinder is
// open-ended or capped.

#ifndef __vtkCylinderSource_h
#define __vtkCylinderSource_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkCylinderSource : public vtkPolyDataSource 
{
public:
  static vtkCylinderSource *New();
  vtkTypeRevisionMacro(vtkCylinderSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the height of the cylinder.
  vtkSetClampMacro(Height,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(Height,float);

  // Description:
  // Set the radius of the cylinder.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(Radius,float);

  // Description:
  // Set/Get cylinder center
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the number of facets used to define cylinder.
  vtkSetClampMacro(Resolution,int,0,VTK_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Turn on/off whether to cap cylinder with polygons.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

protected:
  vtkCylinderSource(int res=6);
  ~vtkCylinderSource() {};

  void Execute();
  float Height;
  float Radius;
  float Center[3];
  int Resolution;
  int Capping;

private:
  vtkCylinderSource(const vtkCylinderSource&);  // Not implemented.
  void operator=(const vtkCylinderSource&);  // Not implemented.
};

#endif


