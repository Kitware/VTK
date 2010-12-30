/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChart2DHistogram.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkChart2DHistogram - Chart for 2D histograms.
//
// .SECTION Description
// This defines the interface for a 2D histogram chart.

#ifndef __vtkChart2DHistogram_h
#define __vtkChart2DHistogram_h

#include "vtkChartXY.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkColorLegend;
class vtk2DHistogramItem;
class vtkImageData;
class vtkScalarsToColors;

class VTK_CHARTS_EXPORT vtkChart2DHistogram : public vtkChartXY
{
public:
  vtkTypeMacro(vtkChart2DHistogram, vtkChartXY);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D histogram chart
  static vtkChart2DHistogram* New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  virtual void SetInput(vtkImageData *data, vtkIdType z = 0);
  virtual void SetTransferFunction(vtkScalarsToColors *function);

  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

protected:
  vtkChart2DHistogram();
  ~vtkChart2DHistogram();

  vtkSmartPointer<vtk2DHistogramItem> Histogram;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  class Private;
  Private* Storage;

  virtual bool UpdateLayout(vtkContext2D *painter);

private:
  vtkChart2DHistogram(const vtkChart2DHistogram &); // Not implemented.
  void operator=(const vtkChart2DHistogram &);   // Not implemented.
};

#endif //__vtkChart2DHistogram_h
