/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OutlineF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkOutlineFilter - create wireframe outline for arbitrary data set
// .SECTION Description
// vtkOutlineFilter is a filter that generates a wireframe outline of any 
// data set. The outline consists of the twelve edges of the dataset 
// bounding box.

#ifndef __vtkOutlineFilter_h
#define __vtkOutlineFilter_h

#include "DS2PolyF.hh"

class vtkOutlineFilter : public vtkDataSetToPolyFilter
{
public:
  vtkOutlineFilter() {};
  ~vtkOutlineFilter() {};
  char *GetClassName() {return "vtkOutlineFilter";};

protected:
  void Execute();
};

#endif


