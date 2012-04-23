/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTableToStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPTableToStructuredGrid - vtkTableToStructuredGrid specialization
// which handles distribution of the input table.
// .SECTION Description
// vtkPTableToStructuredGrid is vtkTableToStructuredGrid specialization
// which handles distribution of the input table.
// For starters, this assumes that the input table is only available on the root
// node.

#ifndef __vtkPTableToStructuredGrid_h
#define __vtkPTableToStructuredGrid_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTableToStructuredGrid.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPTableToStructuredGrid : public vtkTableToStructuredGrid
{
public:
  static vtkPTableToStructuredGrid* New();
  vtkTypeMacro(vtkPTableToStructuredGrid, vtkTableToStructuredGrid);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the controller.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

//BTX
protected:
  vtkPTableToStructuredGrid();
  ~vtkPTableToStructuredGrid();

  // Description:
  // Convert input vtkTable to vtkStructuredGrid.
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  vtkMultiProcessController* Controller;
private:
  vtkPTableToStructuredGrid(const vtkPTableToStructuredGrid&); // Not implemented.
  void operator=(const vtkPTableToStructuredGrid&); // Not implemented.
//ETX
};

#endif


