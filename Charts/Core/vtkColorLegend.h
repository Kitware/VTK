// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkColorLegend
 * @brief   Legend item to display vtkScalarsToColors.
 *
 * vtkColorLegend is an item that will display the vtkScalarsToColors
 * using a 1D texture, and a vtkAxis to show both the color and numerical range.
 */

#ifndef vtkColorLegend_h
#define vtkColorLegend_h

#include "vtkChartLegend.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkSmartPointer.h"     // For SP ivars
#include "vtkVector.h"           // For vtkRectf
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkAxis;
class vtkContextMouseEvent;
class vtkImageData;
class vtkScalarsToColors;
class vtkCallbackCommand;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkColorLegend : public vtkChartLegend
{
public:
  vtkTypeMacro(vtkColorLegend, vtkChartLegend);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkColorLegend* New();

  /**
   * Enum of legend orientation types
   */
  enum
  {
    VERTICAL = 0,
    HORIZONTAL
  };

  /**
   * Bounds of the item, by default (0, 1, 0, 1) but it mainly depends on the
   * range of the vtkScalarsToColors function.
   */
  virtual void GetBounds(double bounds[4]);

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint the texture into a rectangle defined by the bounds. If
   * MaskAboveCurve is true and a shape has been provided by a subclass, it
   * draws the texture into the shape
   */
  bool Paint(vtkContext2D* painter) override;

  ///@{
  /**
   * Set/Get the transfer function that is used to draw the scalar bar
   * within this legend.
   */
  virtual void SetTransferFunction(vtkScalarsToColors* transfer);
  virtual vtkScalarsToColors* GetTransferFunction();
  ///@}

  /**
   * Set the point this legend is anchored to.
   */
  void SetPoint(float x, float y) override;

  /**
   * Set the size of the scalar bar drawn by this legend.
   */
  virtual void SetTextureSize(float w, float h);

  /**
   * Set the origin, width, and height of the scalar bar drawn by this legend.
   * This method overrides the anchor point, as well as any horizontal and
   * vertical alignment that has been set for this legend.  If this is a
   * problem for you, use SetPoint() and SetTextureSize() instead.
   */
  virtual void SetPosition(const vtkRectf& pos);

  /**
   * Returns the origin, width, and height of the scalar bar drawn by this
   * legend.
   */
  virtual vtkRectf GetPosition();

  /**
   * Request the space the legend requires to be drawn. This is returned as a
   * vtkRect4f, with the corner being the offset from Point, and the width/
   * height being the total width/height required by the axis. In order to
   * ensure the numbers are correct, Update() should be called first.
   */
  vtkRectf GetBoundingRect(vtkContext2D* painter) override;

  ///@{
  /**
   * Set/get the orientation of the legend.
   * Valid orientations are VERTICAL (default) and HORIZONTAL.
   */
  virtual void SetOrientation(int orientation);
  vtkGetMacro(Orientation, int);
  ///@}

  ///@{
  /**
   * Get/set the title text of the legend.
   */
  virtual void SetTitle(const vtkStdString& title);
  virtual vtkStdString GetTitle();
  ///@}

  ///@{
  /**
   * Toggle whether or not a border should be drawn around this legend.
   * The default behavior is to not draw a border.
   */
  vtkSetMacro(DrawBorder, bool);
  vtkGetMacro(DrawBorder, bool);
  vtkBooleanMacro(DrawBorder, bool);
  ///@}

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

protected:
  vtkColorLegend();
  ~vtkColorLegend() override;

  /**
   * Need to be reimplemented by subclasses, ComputeTexture() is called at
   * paint time if the texture is not up to date compared to vtkColorLegend
   */
  virtual void ComputeTexture();

  ///@{
  /**
   * Called whenever the ScalarsToColors function(s) is modified. It internally
   * calls Modified(). Can be reimplemented by subclasses.
   */
  virtual void ScalarsToColorsModified(vtkObject* caller, unsigned long eid, void* calldata);
  static void OnScalarsToColorsModified(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  ///@}

  /**
   * Moves the axis whenever the position of this legend changes.
   */
  void UpdateAxisPosition();

  vtkScalarsToColors* TransferFunction;
  vtkSmartPointer<vtkImageData> ImageData;
  vtkSmartPointer<vtkAxis> Axis;
  vtkSmartPointer<vtkCallbackCommand> Callback;
  bool Interpolate;
  bool CustomPositionSet;
  bool DrawBorder;
  vtkRectf Position;
  int Orientation;

private:
  vtkColorLegend(const vtkColorLegend&) = delete;
  void operator=(const vtkColorLegend&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
