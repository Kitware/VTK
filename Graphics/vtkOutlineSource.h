/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineSource.h
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
// .NAME vtkOutlineSource - create wireframe outline around bounding box
// .SECTION Description
// vtkOutlineSource creates a wireframe outline around a user-specified 
// bounding box.

#ifndef __vtkOutlineSource_h
#define __vtkOutlineSource_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkOutlineSource : public vtkPolyDataSource 
{
public:
  static vtkOutlineSource *New();
  vtkTypeRevisionMacro(vtkOutlineSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the bounding box for this object.
  vtkSetVector6Macro(Bounds,float);
  vtkGetVectorMacro(Bounds,float,6);

protected:
  vtkOutlineSource();
  ~vtkOutlineSource() {};

  void Execute();
  float Bounds[6];
private:
  vtkOutlineSource(const vtkOutlineSource&);  // Not implemented.
  void operator=(const vtkOutlineSource&);  // Not implemented.
};

#endif


