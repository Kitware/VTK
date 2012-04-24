/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCellDataToPointData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPCellDataToPointData - Compute point arrays from cell arrays.
// .SECTION Description
// Like it super class, this filter averages the cell data around
// a point to get new point data.  This subclass requests a layer of
// ghost cells to make the results invariant to pieces.  There is a
// "PieceInvariant" flag that lets the user change the behavior
// of the filter to that of its superclass.

#ifndef __vtkPCellDataToPointData_h
#define __vtkPCellDataToPointData_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkCellDataToPointData.h"

class VTKFILTERSPARALLEL_EXPORT vtkPCellDataToPointData : public vtkCellDataToPointData
{
public:
  vtkTypeMacro(vtkPCellDataToPointData,vtkCellDataToPointData);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPCellDataToPointData *New();

  // Description:
  // To get piece invariance, this filter has to request an
  // extra ghost level.  By default piece invariance is on.
  vtkSetMacro(PieceInvariant, int);
  vtkGetMacro(PieceInvariant, int);
  vtkBooleanMacro(PieceInvariant, int);

protected:
  vtkPCellDataToPointData();
  ~vtkPCellDataToPointData() {};

  // Usual data generation method
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  int PieceInvariant;
private:
  vtkPCellDataToPointData(const vtkPCellDataToPointData&);  // Not implemented.
  void operator=(const vtkPCellDataToPointData&);  // Not implemented.
};

#endif
