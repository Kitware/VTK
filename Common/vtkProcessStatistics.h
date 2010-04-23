/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessStatistics - get statistics such as cpu and memory usage
// .SECTION Description

#ifndef __vtkProcessStatistics_h
#define __vtkProcessStatistics_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkProcessStatistics : public vtkObject
{
public:
  // Description:
  // Construct the ProcessStatistics with eight points.
  static vtkProcessStatistics *New();

  vtkTypeMacro(vtkProcessStatistics,vtkObject);

  int    GetProcessSizeInBytes();
  double GetProcessCPUTimeInMilliseconds();

protected:
  vtkProcessStatistics();
  ~vtkProcessStatistics() {};

  
private:
  vtkProcessStatistics(const vtkProcessStatistics&);  // Not implemented.
  void operator=(const vtkProcessStatistics&);  // Not implemented.
};

#endif


