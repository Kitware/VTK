/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MaskPoly.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkMaskPolyData - sample subset of input polygonal data
// .SECTION Description
// vtkMaskPolyData is a filter that sub-samples input polygonal data. The user
// specifies every nth item, with an initial offset to begin sampling.

#ifndef __vtkMaskPolyData_h
#define __vtkMaskPolyData_h

#include "P2PF.hh"

class vtkMaskPolyData : public vtkPolyToPolyFilter
{
public:
  vtkMaskPolyData();
  ~vtkMaskPolyData() {};
  char *GetClassName() {return "vtkMaskPolyData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on every nth entity
  vtkSetClampMacro(OnRatio,int,1,LARGE_INTEGER);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Start with this point
  vtkSetClampMacro(Offset,int,0,LARGE_INTEGER);
  vtkGetMacro(Offset,int);

protected:
  void Execute();
  int OnRatio; // every OnRatio entity is on; all others are off.
  int Offset;  // offset (or starting point id)
};

#endif


