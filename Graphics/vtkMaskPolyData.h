/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPolyData.h
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
// .NAME vtkMaskPolyData - sample subset of input polygonal data
// .SECTION Description
// vtkMaskPolyData is a filter that sub-samples input polygonal data. The user
// specifies every nth item, with an initial offset to begin sampling.

#ifndef __vtkMaskPolyData_h
#define __vtkMaskPolyData_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkMaskPolyData : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkMaskPolyData *New();
  vtkTypeRevisionMacro(vtkMaskPolyData,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on every nth entity (cell).
  vtkSetClampMacro(OnRatio,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Start with this entity (cell).
  vtkSetClampMacro(Offset,vtkIdType,0,VTK_LARGE_ID);
  vtkGetMacro(Offset,vtkIdType);

protected:
  vtkMaskPolyData();
  ~vtkMaskPolyData() {};

  void Execute();
  int OnRatio; // every OnRatio entity is on; all others are off.
  vtkIdType Offset;  // offset (or starting point id)
private:
  vtkMaskPolyData(const vtkMaskPolyData&);  // Not implemented.
  void operator=(const vtkMaskPolyData&);  // Not implemented.
};

#endif


