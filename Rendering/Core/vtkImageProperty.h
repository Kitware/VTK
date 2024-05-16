// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageProperty
 * @brief   image display properties
 *
 * vtkImageProperty is an object that allows control of the display
 * of an image slice.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * vtkImage vtkImageMapper3D vtkImageSliceMapper vtkImageResliceMapper
 */

#ifndef vtkImageProperty_h
#define vtkImageProperty_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkScalarsToColors;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkImageProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkImageProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a property with no lookup table.
   */
  static vtkImageProperty* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(vtkImageProperty* p);

  ///@{
  /**
   * The window value for window/level.
   */
  vtkSetMacro(ColorWindow, double);
  vtkGetMacro(ColorWindow, double);
  ///@}

  ///@{
  /**
   * The level value for window/level.
   */
  vtkSetMacro(ColorLevel, double);
  vtkGetMacro(ColorLevel, double);
  ///@}

  ///@{
  /**
   * Specify a lookup table for the data.  If the data is
   * to be displayed as greyscale, or if the input data is
   * already RGB, there is no need to set a lookup table.
   */
  virtual void SetLookupTable(vtkScalarsToColors* lut);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);
  ///@}

  ///@{
  /**
   * Use the range that is set in the lookup table, instead
   * of setting the range from the Window/Level settings.
   * This is off by default.
   */
  vtkSetMacro(UseLookupTableScalarRange, vtkTypeBool);
  vtkGetMacro(UseLookupTableScalarRange, vtkTypeBool);
  vtkBooleanMacro(UseLookupTableScalarRange, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The opacity of the image, where 1.0 is opaque and 0.0 is
   * transparent.  If the image has an alpha component, then
   * the alpha component will be multiplied by this value.
   * The default is 1.0.
   */
  vtkSetClampMacro(Opacity, double, 0.0, 1.0);
  vtkGetMacro(Opacity, double);
  ///@}

  ///@{
  /**
   * The ambient lighting coefficient.  The default is 1.0.
   */
  vtkSetClampMacro(Ambient, double, 0.0, 1.0);
  vtkGetMacro(Ambient, double);
  ///@}

  ///@{
  /**
   * The diffuse lighting coefficient.  The default is 0.0.
   */
  vtkSetClampMacro(Diffuse, double, 0.0, 1.0);
  vtkGetMacro(Diffuse, double);
  ///@}

  ///@{
  /**
   * The interpolation type (default: VTK_LINEAR_INTERPOLATION).
   */
  vtkSetClampMacro(InterpolationType, int, VTK_NEAREST_INTERPOLATION, VTK_CUBIC_INTERPOLATION);
  vtkGetMacro(InterpolationType, int);
  void SetInterpolationTypeToNearest() { this->SetInterpolationType(VTK_NEAREST_INTERPOLATION); }
  void SetInterpolationTypeToLinear() { this->SetInterpolationType(VTK_LINEAR_INTERPOLATION); }
  void SetInterpolationTypeToCubic() { this->SetInterpolationType(VTK_CUBIC_INTERPOLATION); }
  virtual const char* GetInterpolationTypeAsString();
  ///@}

  ///@{
  /**
   * Set the layer number.  This is ignored unless the image is part
   * of a vtkImageStack.  The default layer number is zero.
   */
  vtkSetMacro(LayerNumber, int);
  int GetLayerNumber() { return this->LayerNumber; }
  ///@}

  ///@{
  /**
   * Make a checkerboard pattern where the black squares are transparent.
   * The pattern is aligned with the camera, and centered by default.
   */
  vtkSetMacro(Checkerboard, vtkTypeBool);
  vtkBooleanMacro(Checkerboard, vtkTypeBool);
  vtkGetMacro(Checkerboard, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The spacing for checkerboarding.  This is in real units, not pixels.
   */
  vtkSetVector2Macro(CheckerboardSpacing, double);
  vtkGetVector2Macro(CheckerboardSpacing, double);
  ///@}

  ///@{
  /**
   * The phase offset for checkerboarding, in units of spacing.  Use a
   * value between -1 and +1, where 1 is an offset of one squares.
   */
  vtkSetVector2Macro(CheckerboardOffset, double);
  vtkGetVector2Macro(CheckerboardOffset, double);
  ///@}

  ///@{
  /**
   * Use an opaque backing polygon, which will be visible where the image
   * is translucent.  When images are in a stack, the backing polygons
   * for all images will be drawn before any of the images in the stack,
   * in order to allow the images in the stack to be composited.
   */
  vtkSetMacro(Backing, vtkTypeBool);
  vtkBooleanMacro(Backing, vtkTypeBool);
  vtkGetMacro(Backing, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the color of the backing polygon.  The default color is black.
   */
  vtkSetVector3Macro(BackingColor, double);
  vtkGetVector3Macro(BackingColor, double);
  ///@}

  /**
   * Get the MTime for this property.  If the lookup table is set,
   * the mtime will include the mtime of the lookup table.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkImageProperty();
  ~vtkImageProperty() override;

  vtkScalarsToColors* LookupTable;
  double ColorWindow;
  double ColorLevel;
  vtkTypeBool UseLookupTableScalarRange;
  int InterpolationType;
  int LayerNumber;
  double Opacity;
  double Ambient;
  double Diffuse;
  vtkTypeBool Checkerboard;
  double CheckerboardSpacing[2];
  double CheckerboardOffset[2];
  vtkTypeBool Backing;
  double BackingColor[3];

private:
  vtkImageProperty(const vtkImageProperty&) = delete;
  void operator=(const vtkImageProperty&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
