/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk2DHistogramItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk2DHistogramItem - 2D histogram item.
//
// .SECTION Description
//

#ifndef __vtk2DHistogramItem_h
#define __vtk2DHistogramItem_h

#include "vtkPlot.h"
#include "vtkSmartPointer.h"  // Needed for SP ivars
#include "vtkVector.h"        // Needed for vtkRectf

class vtkImageData;
class vtkScalarsToColors;

class VTK_CHARTS_EXPORT vtk2DHistogramItem : public vtkPlot
{
public:
  vtkTypeMacro(vtk2DHistogramItem, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a new object.
  static vtk2DHistogramItem *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the item, called whenever it needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the input, we are expecting a vtkImageData with just one component,
  // this would normally be a float or a double. It will be passed to the other
  // functions as a double to generate a color.
  virtual void SetInput(vtkImageData *data, vtkIdType z = 0);
  virtual void SetInput(vtkTable*) { }
  virtual void SetInput(vtkTable*, const vtkStdString&, const vtkStdString&) { }

  // Description:
  // Get the input table used by the plot.
  vtkImageData * GetInputImageData();

  // Description:
  // Set the color transfer funtion that will be used to generate the 2D
  // histogram.
  void SetTransferFunction(vtkScalarsToColors *transfer);

  // Description:
  // Get the color transfer function that is used to generate the histogram.
  vtkScalarsToColors * GetTransferFunction();

  virtual void GetBounds(double bounds[4]);

  virtual void SetPosition(const vtkRectf& pos);
  virtual vtkRectf GetPosition();

//BTX
protected:
  vtk2DHistogramItem();
  ~vtk2DHistogramItem();

  // Description:
  // Where all the magic happens...
  void GenerateHistogram();

  vtkSmartPointer<vtkImageData> Input;
  vtkSmartPointer<vtkImageData> Output;
  vtkSmartPointer<vtkScalarsToColors> TransferFunction;
  vtkRectf Position;

private:
  vtk2DHistogramItem(const vtk2DHistogramItem &); // Not implemented.
  void operator=(const vtk2DHistogramItem &); // Not implemented.

//ETX
};

#endif //__vtk2DHistogramItem_h
