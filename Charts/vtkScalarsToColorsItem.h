/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColorsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkScalarsToColorsItem - Abstract class for ScalarsToColors items.
// .SECTION Description
// vtkScalarsToColorsItem implements item bounds and painting for inherited
// classes that provide a texture (ComputeTexture()) and optionally a shape
// .SECTION See Also
// vtkControlPointsItem
// vtkLookupTableItem
// vtkColorTransferFunctionItem
// vtkCompositeTransferFunctionItem
// vtkPiecewiseItemFunctionItem

#ifndef __vtkScalarsToColorsItem_h
#define __vtkScalarsToColorsItem_h

#include "vtkPlot.h"

class vtkCallbackCommand;
class vtkImageData;
class vtkPoints2D;

class VTK_CHARTS_EXPORT vtkScalarsToColorsItem: public vtkPlot
{
public:
  vtkTypeMacro(vtkScalarsToColorsItem, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Bounds of the item, use the UserBounds if valid otherwise compute
  // the bounds of the item (based on the transfer function range).
  void GetBounds(double bounds[4]);

  // Description:
  // Set custom bounds, except if bounds are invalid, bounds will be
  // automatically computed based on the range of the control points
  // Invalid bounds by default.
  vtkSetVector4Macro(UserBounds, double);
  vtkGetVector4Macro(UserBounds, double)

  // Description:
  // Paint the texture into a rectangle defined by the bounds. If
  // MaskAboveCurve is true and a shape has been provided by a subclass, it
  // draws the texture into the shape
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Get a pointer to the vtkPen object that controls the drawing of the edge
  // of the shape if any.
  // PolyLinePen type is vtkPen::NO_PEN by default.
  vtkGetObjectMacro(PolyLinePen, vtkPen);

  // Description:
  // Don't fill in the part above the transfer function.
  // If true texture is not visible above the shape provided by subclasses,
  // otherwise the whole rectangle defined by the bounds is filled with the
  // transfer function.
  // Note: only 2D transfer functions (RGB tf + alpha tf ) support the feature.
  vtkSetMacro(MaskAboveCurve, bool);
  vtkGetMacro(MaskAboveCurve, bool);

protected:
  vtkScalarsToColorsItem();
  virtual ~vtkScalarsToColorsItem();

  // Description:
  // Bounds of the item, by default (0, 1, 0, 1) but it depends on the
  // range of the ScalarsToColors function.
  // Need to be reimplemented by subclasses if the range is != [0,1]
  virtual void ComputeBounds(double* bounds);

  // Description:
  // Need to be reimplemented by subclasses, ComputeTexture() is called at
  // paint time if the texture is not up to date compared to vtkScalarsToColorsItem
  // Return false if no texture is generated.
  virtual void ComputeTexture() = 0;

  vtkGetMacro(TextureWidth, int);

  // Description:
  // Called whenever the ScalarsToColors function(s) is modified. It internally
  // calls Modified(). Can be reimplemented by subclasses
  virtual void ScalarsToColorsModified(vtkObject* caller, unsigned long eid, void* calldata);
  static void OnScalarsToColorsModified(vtkObject* caller, unsigned long eid, void *clientdata, void* calldata);

  double              UserBounds[4];

  int                 TextureWidth;
  vtkImageData*       Texture;
  bool                Interpolate;
  vtkPoints2D*        Shape;
  vtkCallbackCommand* Callback;

  vtkPen*             PolyLinePen;
  bool                MaskAboveCurve;
private:
  vtkScalarsToColorsItem(const vtkScalarsToColorsItem &); // Not implemented.
  void operator=(const vtkScalarsToColorsItem &);   // Not implemented.
};

#endif
