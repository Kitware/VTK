/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataAlgorithm - Superclass for polygonal data algorithms.
// .SECTION Description
// vtkPolyDataAlgorithm

#ifndef __vtkPolyDataAlgorithm_h
#define __vtkPolyDataAlgorithm_h

#include "vtkAlgorithm.h"

class vtkPolyData;

class VTK_FILTERING_EXPORT vtkPolyDataAlgorithm : public vtkAlgorithm
{
public:
  static vtkPolyDataAlgorithm *New();
  vtkTypeRevisionMacro(vtkPolyDataAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkPolyData* GetOutput();
  vtkPolyData* GetOutput(int);

  // Description:
  // Set an input of this algorithm.
  void SetInput(vtkPolyData*);
  void SetInput(int, vtkPolyData*);

  // Description:
  // Add an input of this algorithm.
  void AddInput(vtkPolyData*);
  void AddInput(int, vtkPolyData*);

protected:
  vtkPolyDataAlgorithm();
  ~vtkPolyDataAlgorithm();

private:
  vtkPolyDataAlgorithm(const vtkPolyDataAlgorithm&);  // Not implemented.
  void operator=(const vtkPolyDataAlgorithm&);  // Not implemented.
};

#endif
