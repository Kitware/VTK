// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTextProperty
 * @brief   represent text properties.
 *
 * vtkTextProperty is an object that represents text properties.
 * The primary properties that can be set are color, opacity, font size,
 * font family horizontal and vertical justification, bold/italic/shadow
 * styles.
 * @sa
 * vtkTextMapper vtkTextActor vtkLegendBoxActor vtkCaptionActor2D
 */

#ifndef vtkTextProperty_h
#define vtkTextProperty_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkTextProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkTextProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a new text property with font size 12, bold off, italic off,
   * and Arial font.
   */
  static vtkTextProperty* New();

  ///@{
  /**
   * Set the color of the text.
   */
  vtkSetVector3Macro(Color, double);
  vtkGetVector3Macro(Color, double);
  ///@}

  ///@{
  /**
   * Set/Get the text's opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent.
   */
  vtkSetClampMacro(Opacity, double, 0., 1.);
  vtkGetMacro(Opacity, double);
  ///@}

  ///@{
  /**
   * The background color.
   */
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);
  ///@}

  ///@{
  /**
   * The background opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent.
   */
  vtkSetClampMacro(BackgroundOpacity, double, 0., 1.);
  vtkGetMacro(BackgroundOpacity, double);
  ///@}

  ///@{
  /**
   * Convenience method to set the background color and the opacity at once
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetBackgroundRGBA(double rgba[4]);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetBackgroundRGBA(double r, double g, double b, double a);

  /**
   * Convenience method to get the background color and the opacity at once
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void GetBackgroundRGBA(double rgba[4]);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void GetBackgroundRGBA(double& r, double& g, double& b, double& a);
  ///@}

  ///@{
  /**
   * The frame color.
   */
  vtkSetVector3Macro(FrameColor, double);
  vtkGetVector3Macro(FrameColor, double);
  ///@}

  ///@{
  /**
   * Enable/disable text frame.
   */
  vtkSetMacro(Frame, vtkTypeBool);
  vtkGetMacro(Frame, vtkTypeBool);
  vtkBooleanMacro(Frame, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the width of the frame. The width is expressed in pixels.
   * The default is 1 pixel.
   */
  vtkSetClampMacro(FrameWidth, int, 0, VTK_INT_MAX);
  vtkGetMacro(FrameWidth, int);
  ///@}

  ///@{
  /**
   * Set/Get the font family. Supports legacy three font family system.
   * If the symbolic constant VTK_FONT_FILE is returned by GetFontFamily(), the
   * string returned by GetFontFile() must be an absolute filepath
   * to a local FreeType compatible font.
   */
  vtkGetStringMacro(FontFamilyAsString);
  vtkSetStringMacro(FontFamilyAsString);
  void SetFontFamily(int t);
  int GetFontFamily();
  int GetFontFamilyMinValue() { return VTK_ARIAL; }
  void SetFontFamilyToArial();
  void SetFontFamilyToCourier();
  void SetFontFamilyToTimes();
  static int GetFontFamilyFromString(const char* f);
  static const char* GetFontFamilyAsString(int f);
  ///@}

  ///@{
  /**
   * The absolute filepath to a local file containing a freetype-readable font
   * if GetFontFamily() return VTK_FONT_FILE. The result is undefined for other
   * values of GetFontFamily().
   */
  vtkGetFilePathMacro(FontFile);
  vtkSetFilePathMacro(FontFile);
  ///@}

  ///@{
  /**
   * Set/Get the font size (in points).
   */
  vtkSetClampMacro(FontSize, int, 0, VTK_INT_MAX);
  vtkGetMacro(FontSize, int);
  ///@}

  ///@{
  /**
   * Enable/disable text bolding.
   */
  vtkSetMacro(Bold, vtkTypeBool);
  vtkGetMacro(Bold, vtkTypeBool);
  vtkBooleanMacro(Bold, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable text italic.
   */
  vtkSetMacro(Italic, vtkTypeBool);
  vtkGetMacro(Italic, vtkTypeBool);
  vtkBooleanMacro(Italic, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable text shadow.
   */
  vtkSetMacro(Shadow, vtkTypeBool);
  vtkGetMacro(Shadow, vtkTypeBool);
  vtkBooleanMacro(Shadow, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the shadow offset, i.e. the distance from the text to
   * its shadow, in the same unit as FontSize.
   */
  vtkSetVector2Macro(ShadowOffset, int);
  vtkGetVectorMacro(ShadowOffset, int, 2);
  ///@}

  /**
   * Get the shadow color. It is computed from the Color ivar
   */
  void GetShadowColor(double color[3]);

  ///@{
  /**
   * Set/Get the horizontal justification to left (default), centered,
   * or right.
   */
  vtkSetClampMacro(Justification, int, VTK_TEXT_LEFT, VTK_TEXT_RIGHT);
  vtkGetMacro(Justification, int);
  void SetJustificationToLeft() { this->SetJustification(VTK_TEXT_LEFT); }
  void SetJustificationToCentered() { this->SetJustification(VTK_TEXT_CENTERED); }
  void SetJustificationToRight() { this->SetJustification(VTK_TEXT_RIGHT); }
  const char* GetJustificationAsString();
  ///@}

  ///@{
  /**
   * Set/Get the vertical justification to bottom (default), middle,
   * or top.
   */
  vtkSetClampMacro(VerticalJustification, int, VTK_TEXT_BOTTOM, VTK_TEXT_TOP);
  vtkGetMacro(VerticalJustification, int);
  void SetVerticalJustificationToBottom() { this->SetVerticalJustification(VTK_TEXT_BOTTOM); }
  void SetVerticalJustificationToCentered() { this->SetVerticalJustification(VTK_TEXT_CENTERED); }
  void SetVerticalJustificationToTop() { this->SetVerticalJustification(VTK_TEXT_TOP); }
  const char* GetVerticalJustificationAsString();
  ///@}

  ///@{
  /**
   * If this property is on, text is aligned to drawn pixels not to font matrix.
   * If the text does not include descents, the bounding box will not extend below
   * the baseline. This option can be used to get centered labels. It does not
   * work well if the string changes as the string position will move around.
   */
  vtkSetMacro(UseTightBoundingBox, vtkTypeBool);
  vtkGetMacro(UseTightBoundingBox, vtkTypeBool);
  vtkBooleanMacro(UseTightBoundingBox, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the text's orientation (in degrees).
   */
  vtkSetMacro(Orientation, double);
  vtkGetMacro(Orientation, double);
  ///@}

  ///@{
  /**
   * Set/Get the (extra) spacing between lines,
   * expressed as a text height multiplication factor.
   */
  vtkSetMacro(LineSpacing, double);
  vtkGetMacro(LineSpacing, double);
  ///@}

  ///@{
  /**
   * Set/Get the vertical offset (measured in pixels).
   */
  vtkSetMacro(LineOffset, double);
  vtkGetMacro(LineOffset, double);
  ///@}

  ///@{
  /**
   * Set/Get the horizontal offset between cells.
   * Only used by MatplotlibMathTextUtilities
   */
  vtkSetMacro(CellOffset, double);
  vtkGetMacro(CellOffset, double);
  ///@}

  ///@{
  /**
   * Set/Get the visibility of the interior lines between cells.
   * Default is false.
   */
  vtkSetMacro(InteriorLinesVisibility, bool);
  vtkGetMacro(InteriorLinesVisibility, bool);
  ///@}

  ///@{
  /**
   * Set the width (in pixels) of the interior lines between cells.
   * Default is 1.
   */
  vtkSetMacro(InteriorLinesWidth, int);
  vtkGetMacro(InteriorLinesWidth, int);
  ///@}

  ///@{
  /**
   * Set the color of the interior lines between cells.
   * Default is black (0.0, 0.0, 0.0).
   */
  vtkSetVector3Macro(InteriorLinesColor, double);
  vtkGetVector3Macro(InteriorLinesColor, double);
  ///@}

  /**
   * Shallow copy of a text property.
   */
  void ShallowCopy(vtkTextProperty* tprop);

protected:
  vtkTextProperty();
  ~vtkTextProperty() override;

  double Color[3];
  double Opacity;
  double BackgroundColor[3];
  double BackgroundOpacity;
  vtkTypeBool Frame;
  double FrameColor[3];
  int FrameWidth;
  char* FontFamilyAsString;
  char* FontFile;
  int FontSize;
  vtkTypeBool Bold;
  vtkTypeBool Italic;
  vtkTypeBool Shadow;
  int ShadowOffset[2];
  int Justification;
  int VerticalJustification;
  vtkTypeBool UseTightBoundingBox;
  double Orientation;
  double LineOffset;
  double LineSpacing;
  double CellOffset;
  bool InteriorLinesVisibility = false;
  int InteriorLinesWidth = 1;
  double InteriorLinesColor[3] = { 0.0, 0.0, 0.0 };

private:
  vtkTextProperty(const vtkTextProperty&) = delete;
  void operator=(const vtkTextProperty&) = delete;
};

inline const char* vtkTextProperty::GetFontFamilyAsString(int f)
{
  if (f == VTK_ARIAL)
  {
    return "Arial";
  }
  else if (f == VTK_COURIER)
  {
    return "Courier";
  }
  else if (f == VTK_TIMES)
  {
    return "Times";
  }
  else if (f == VTK_FONT_FILE)
  {
    return "File";
  }
  return "Unknown";
}

inline void vtkTextProperty::SetFontFamily(int t)
{
  this->SetFontFamilyAsString(this->GetFontFamilyAsString(t));
}

inline void vtkTextProperty::SetFontFamilyToArial()
{
  this->SetFontFamily(VTK_ARIAL);
}

inline void vtkTextProperty::SetFontFamilyToCourier()
{
  this->SetFontFamily(VTK_COURIER);
}

inline void vtkTextProperty::SetFontFamilyToTimes()
{
  this->SetFontFamily(VTK_TIMES);
}

inline int vtkTextProperty::GetFontFamilyFromString(const char* f)
{
  if (strcmp(f, GetFontFamilyAsString(VTK_ARIAL)) == 0)
  {
    return VTK_ARIAL;
  }
  else if (strcmp(f, GetFontFamilyAsString(VTK_COURIER)) == 0)
  {
    return VTK_COURIER;
  }
  else if (strcmp(f, GetFontFamilyAsString(VTK_TIMES)) == 0)
  {
    return VTK_TIMES;
  }
  else if (strcmp(f, GetFontFamilyAsString(VTK_FONT_FILE)) == 0)
  {
    return VTK_FONT_FILE;
  }
  return VTK_UNKNOWN_FONT;
}

inline int vtkTextProperty::GetFontFamily()
{
  return GetFontFamilyFromString(this->FontFamilyAsString);
}

inline const char* vtkTextProperty::GetJustificationAsString()
{
  if (this->Justification == VTK_TEXT_LEFT)
  {
    return "Left";
  }
  else if (this->Justification == VTK_TEXT_CENTERED)
  {
    return "Centered";
  }
  else if (this->Justification == VTK_TEXT_RIGHT)
  {
    return "Right";
  }
  return "Unknown";
}

inline const char* vtkTextProperty::GetVerticalJustificationAsString()
{
  if (this->VerticalJustification == VTK_TEXT_BOTTOM)
  {
    return "Bottom";
  }
  else if (this->VerticalJustification == VTK_TEXT_CENTERED)
  {
    return "Centered";
  }
  else if (this->VerticalJustification == VTK_TEXT_TOP)
  {
    return "Top";
  }
  return "Unknown";
}

VTK_ABI_NAMESPACE_END
#endif
