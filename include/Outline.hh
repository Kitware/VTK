/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Outline.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkOutlineSource - create wireframe wireframe outline around bounding box
// .SECTION Description
// vtkOutlineSource creates a wireframe outline around a user specified 
// bounding box.

#ifndef __vtkOutlineSource_h
#define __vtkOutlineSource_h

#include "PolySrc.hh"

class vtkOutlineSource : public vtkPolySource 
{
public:
  vtkOutlineSource();
  ~vtkOutlineSource() {};
  char *GetClassName() {return "vtkOutlineSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the bounding box for this object.
  vtkSetVectorMacro(Bounds,float,6);
  vtkGetVectorMacro(Bounds,float,6);

protected:
  void Execute();
  float Bounds[6];
};

#endif


