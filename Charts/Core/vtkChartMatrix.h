/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkChartMatrix - container for a matrix of charts.
//
// .SECTION Description
// This class contains a matrix of charts. These charts will be of type
// vtkChartXY by default, but this can be overridden. The class will manage
// their layout and object lifetime.

#ifndef __vtkChartMatrix_h
#define __vtkChartMatrix_h

#include "vtkAbstractContextItem.h"
#include "vtkVector.h" // For ivars

class vtkChart;

class VTK_CHARTS_EXPORT vtkChartMatrix : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkChartMatrix, vtkAbstractContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a new object.
  static vtkChartMatrix *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  virtual void Update();

  // Description:
  // Paint event for the chart matrix.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the width and height of the chart matrix. This will cause an immediate
  // resize of the chart matrix, the default size is 0x0 (no charts). No chart
  // objects are created until Allocate is called.
  virtual void SetSize(const vtkVector2i& size);

  // Description:
  // Get the width and height of the chart matrix.
  virtual vtkVector2i GetSize() const { return this->Size; }

  // Description:
  // Set the gutter that should be left between the charts in the matrix.
  virtual void SetGutter(const vtkVector2f& gutter);

  // Description:
  // Set/get the borders of the chart matrix (space in pixels around each chart).
  virtual void SetBorders(int left, int bottom, int right, int top);
  virtual void GetBorders(int borders[4])
  {
    for(int i=0;i<4;i++)
      {
      borders[i]=this->Borders[i];
      }
  }

  // Description:
  // Get the gutter that should be left between the charts in the matrix.
  virtual vtkVector2f GetGutter() const { return this->Gutter; }

  // Description:
  // Allocate the charts, this will cause any null chart to be allocated.
  virtual void Allocate();

  // Description:
  // Set the chart element, note that the chart matrix must be large enough to
  // accommodate the element being set. Note that this class will take ownership
  // of the chart object.
  // \return false if the element cannot be set.
  virtual bool SetChart(const vtkVector2i& position, vtkChart* chart);

  // Description:
  // Get the specified chart element, if the element does not exist NULL will be
  // returned. If the chart element has not yet been allocated it will be at
  // this point.
  virtual vtkChart* GetChart(const vtkVector2i& position);

  // Description:
  // Set the span of a chart in the matrix. This defaults to 1x1, and cannot
  // exceed the remaining space in x or y.
  // \return false If the span is not possible.
  virtual bool SetChartSpan(const vtkVector2i& position,
                            const vtkVector2i& span);

  // Description:
  // Get the span of the specified chart.
  virtual vtkVector2i GetChartSpan(const vtkVector2i& position);

protected:
  vtkChartMatrix();
  ~vtkChartMatrix();

  class PIMPL;
  PIMPL *Private;

  // The number of charts in x and y.
  vtkVector2i Size;

  // The gutter between each chart.
  vtkVector2f Gutter;
  int Borders[4];
  bool LayoutIsDirty;

private:
  vtkChartMatrix(const vtkChartMatrix &); // Not implemented.
  void operator=(const vtkChartMatrix &); // Not implemented.
};

#endif //__vtkChartMatrix_h
