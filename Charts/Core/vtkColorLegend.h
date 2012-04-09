/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorLegend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkColorLegend - Legend item to display vtkScalarsToColors.
// .SECTION Description
// vtkColorLegend is an item that will display the vtkScalarsToColors
// using a 1D texture, and a vtkAxis to show both the color and numerical range.

#ifndef __vtkColorLegend_h
#define __vtkColorLegend_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkChartLegend.h"
#include "vtkSmartPointer.h" // For SP ivars
#include "vtkVector.h"       // For vtkRectf

class vtkAxis;
class vtkImageData;
class vtkScalarsToColors;
class vtkCallbackCommand;

class VTKCHARTSCORE_EXPORT vtkColorLegend: public vtkChartLegend
{
public:
  vtkTypeMacro(vtkColorLegend, vtkChartLegend);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkColorLegend* New();

  // Description:
  // Bounds of the item, by default (0, 1, 0, 1) but it mainly depends on the
  // range of the vtkScalarsToColors function.
  virtual void GetBounds(double bounds[4]);

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint the texture into a rectangle defined by the bounds. If
  // MaskAboveCurve is true and a shape has been provided by a subclass, it
  // draws the texture into the shape
  virtual bool Paint(vtkContext2D *painter);

  virtual void SetTransferFunction(vtkScalarsToColors* transfer);
  virtual vtkScalarsToColors * GetTransferFunction();

  virtual void SetPosition(const vtkRectf& pos);
  virtual vtkRectf GetPosition();

  // Description:
  // Request the space the legend requires to be drawn. This is returned as a
  // vtkRect4f, with the corner being the offset from Point, and the width/
  // height being the total width/height required by the axis. In order to
  // ensure the numbers are correct, Update() should be called first.
  vtkRectf GetBoundingRect(vtkContext2D* painter);

protected:
  vtkColorLegend();
  virtual ~vtkColorLegend();

  // Description:
  // Need to be reimplemented by subclasses, ComputeTexture() is called at
  // paint time if the texture is not up to date compared to vtkColorLegend
  virtual void ComputeTexture();

  // Description:
  // Called whenever the ScalarsToColors function(s) is modified. It internally
  // calls Modified(). Can be reimplemented by subclasses.
  virtual void ScalarsToColorsModified(vtkObject* caller, unsigned long eid,
                                       void* calldata);
  static void OnScalarsToColorsModified(vtkObject* caller, unsigned long eid,
                                        void *clientdata, void* calldata);

  vtkScalarsToColors*                 TransferFunction;
  vtkSmartPointer<vtkImageData>       ImageData;
  vtkSmartPointer<vtkAxis>            Axis;
  vtkSmartPointer<vtkCallbackCommand> Callback;
  bool                                Interpolate;
  vtkRectf                            Position;

private:
  vtkColorLegend(const vtkColorLegend &); // Not implemented.
  void operator=(const vtkColorLegend &);   // Not implemented.
};

#endif
