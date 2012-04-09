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

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkChartXY.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkColorLegend;
class vtkPlotHistogram2D;
class vtkImageData;
class vtkScalarsToColors;

class VTKCHARTSCORE_EXPORT vtkChartHistogram2D : public vtkChartXY
{
public:
  vtkTypeMacro(vtkChartHistogram2D, vtkChartXY);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D histogram chart
  static vtkChartHistogram2D* New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  virtual void SetInputData(vtkImageData *data, vtkIdType z = 0);
  virtual void SetTransferFunction(vtkScalarsToColors *function);

  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Get the plot at the specified index, returns null if the index is invalid.
  virtual vtkPlot* GetPlot(vtkIdType index);

protected:
  vtkChartHistogram2D();
  ~vtkChartHistogram2D();

  vtkSmartPointer<vtkPlotHistogram2D> Histogram;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  class Private;
  Private* Storage;

  virtual bool UpdateLayout(vtkContext2D *painter);

private:
  vtkChartHistogram2D(const vtkChartHistogram2D &); // Not implemented.
  void operator=(const vtkChartHistogram2D &);   // Not implemented.
};

#endif //__vtkChart2DHistogram_h
