/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkScatterPlotMatrix - container for a matrix of charts.
//
// .SECTION Description
// This class contains a matrix of charts. These charts will be of type
// vtkChartXY by default, but this can be overridden. The class will manage
// their layout and object lifetime.

#ifndef __vtkScatterPlotMatrix_h
#define __vtkScatterPlotMatrix_h

#include "vtkChartMatrix.h"
#include "vtkSmartPointer.h" // For ivars

class vtkTable;

class VTK_CHARTS_EXPORT vtkScatterPlotMatrix : public vtkChartMatrix
{
public:
  vtkTypeMacro(vtkScatterPlotMatrix, vtkChartMatrix);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a new object.
  static vtkScatterPlotMatrix *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  virtual void Update();

  // Description:
  // Paint event for the chart matrix.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the input table for the scatter plot matrix. This will cause all
  // columns to be plotted against each other - a square scatter plot matrix.
  virtual void SetInput(vtkTable *table);

protected:
  vtkScatterPlotMatrix();
  ~vtkScatterPlotMatrix();

  class PIMPL;
  PIMPL *Private;

  vtkSmartPointer<vtkTable> Input;

private:
  vtkScatterPlotMatrix(const vtkScatterPlotMatrix &); // Not implemented.
  void operator=(const vtkScatterPlotMatrix &); // Not implemented.
};

#endif //__vtkScatterPlotMatrix_h
