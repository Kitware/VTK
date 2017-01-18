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

/**
 * @class   vtkScalarsToColorsItem
 * @brief   Abstract class for ScalarsToColors items.
 *
 * vtkScalarsToColorsItem implements item bounds and painting for inherited
 * classes that provide a texture (ComputeTexture()) and optionally a shape
 * @sa
 * vtkControlPointsItem
 * vtkLookupTableItem
 * vtkColorTransferFunctionItem
 * vtkCompositeTransferFunctionItem
 * vtkPiecewiseItemFunctionItem
*/

#ifndef vtkScalarsToColorsItem_h
#define vtkScalarsToColorsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot.h"

class vtkCallbackCommand;
class vtkImageData;
class vtkPoints2D;

class VTKCHARTSCORE_EXPORT vtkScalarsToColorsItem: public vtkPlot
{
public:
  vtkTypeMacro(vtkScalarsToColorsItem, vtkPlot);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Bounds of the item, use the UserBounds if valid otherwise compute
   * the bounds of the item (based on the transfer function range).
   */
  void GetBounds(double bounds[4]) VTK_OVERRIDE;

  //@{
  /**
   * Set custom bounds, except if bounds are invalid, bounds will be
   * automatically computed based on the range of the control points
   * Invalid bounds by default.
   */
  vtkSetVector4Macro(UserBounds, double);
  vtkGetVector4Macro(UserBounds, double)
  //@}

  /**
   * Paint the texture into a rectangle defined by the bounds. If
   * MaskAboveCurve is true and a shape has been provided by a subclass, it
   * draws the texture into the shape
   */
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;

  //@{
  /**
   * Get a pointer to the vtkPen object that controls the drawing of the edge
   * of the shape if any.
   * PolyLinePen type is vtkPen::NO_PEN by default.
   */
  vtkGetObjectMacro(PolyLinePen, vtkPen);
  //@}

  //@{
  /**
   * Don't fill in the part above the transfer function.
   * If true texture is not visible above the shape provided by subclasses,
   * otherwise the whole rectangle defined by the bounds is filled with the
   * transfer function.
   * Note: only 2D transfer functions (RGB tf + alpha tf ) support the feature.
   */
  vtkSetMacro(MaskAboveCurve, bool);
  vtkGetMacro(MaskAboveCurve, bool);
  //@}

protected:
  vtkScalarsToColorsItem();
  ~vtkScalarsToColorsItem() VTK_OVERRIDE;

  /**
   * Bounds of the item, by default (0, 1, 0, 1) but it depends on the
   * range of the ScalarsToColors function.
   * Need to be reimplemented by subclasses if the range is != [0,1]
   */
  virtual void ComputeBounds(double* bounds);

  /**
   * Need to be reimplemented by subclasses, ComputeTexture() is called at
   * paint time if the texture is not up to date compared to vtkScalarsToColorsItem
   * Return false if no texture is generated.
   */
  virtual void ComputeTexture() = 0;

  vtkGetMacro(TextureWidth, int);

  //@{
  /**
   * Called whenever the ScalarsToColors function(s) is modified. It internally
   * calls Modified(). Can be reimplemented by subclasses
   */
  virtual void ScalarsToColorsModified(vtkObject* caller, unsigned long eid, void* calldata);
  static void OnScalarsToColorsModified(vtkObject* caller, unsigned long eid, void *clientdata, void* calldata);
  //@}

  double              UserBounds[4];

  int                 TextureWidth;
  vtkImageData*       Texture;
  bool                Interpolate;
  vtkPoints2D*        Shape;
  vtkCallbackCommand* Callback;

  vtkPen*             PolyLinePen;
  bool                MaskAboveCurve;
private:
  vtkScalarsToColorsItem(const vtkScalarsToColorsItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkScalarsToColorsItem &) VTK_DELETE_FUNCTION;
};

#endif
