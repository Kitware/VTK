/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectPolyData.h
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
// .NAME vtkCollectPolyData - Collect distributed polydata.
// .DESCRIPTION
// This filter has code to collect polydat from across processes onto node 0.
// This collection can be controlled by the size of the data.  If the
// final data size will be above the threshold, then it will not be collected.


#ifndef __vtkCollectPolyData_h
#define __vtkCollectPolyData_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkMultiProcessController.h"


class VTK_PARALLEL_EXPORT vtkCollectPolyData : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkCollectPolyData *New();
  vtkTypeRevisionMacro(vtkCollectPolyData, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // By defualt this filter uses the global controller,
  // but this method can be used to set another instead.
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Threshold that determines whether data will be collected.
  // If the total size of the data in kilobytes is less than this threshold, 
  // then the data remains distributed.
  vtkSetMacro(Threshold, unsigned long);
  vtkGetMacro(Threshold, unsigned long);
  
  // Description:
  // This flag is set based on whether the data was collected to process 0 or not.
  vtkGetMacro(Collected, int);

protected:
  vtkCollectPolyData();
  ~vtkCollectPolyData();
  vtkCollectPolyData(const vtkCollectPolyData&);
  void operator=(const vtkCollectPolyData&);

  // Data generation method
  void ComputeInputUpdateExtents(vtkDataObject *output);
  void Execute();
  void ExecuteInformation();

  unsigned long Threshold;
  int Collected;

  vtkMultiProcessController *Controller;
};

#endif
