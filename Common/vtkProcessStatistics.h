/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessStatistics.h
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
// .NAME vtkProcessStatistics - get statistics such as cpu and memory usage
// .SECTION Description

#ifndef __vtkProcessStatistics_h
#define __vtkProcessStatistics_h

#include "vtkObject.h"
#include "vtkLine.h"
#include "vtkPixel.h"

class VTK_COMMON_EXPORT vtkProcessStatistics : public vtkObject
{
public:
  // Description:
  // Construct the ProcessStatistics with eight points.
  static vtkProcessStatistics *New();

  vtkTypeRevisionMacro(vtkProcessStatistics,vtkObject);

  int   GetProcessSizeInBytes();
  float GetProcessCPUTimeInMilliseconds();

protected:
  vtkProcessStatistics();
  ~vtkProcessStatistics() {};

  
private:
  vtkProcessStatistics(const vtkProcessStatistics&);  // Not implemented.
  void operator=(const vtkProcessStatistics&);  // Not implemented.
};

#endif


