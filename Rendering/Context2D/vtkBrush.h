// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkBrush
 * @brief   provides a brush that fills shapes drawn by vtkContext2D.
 *
 *
 * The vtkBrush defines the fill (or pattern) of shapes that are drawn by
 * vtkContext2D. The color is stored as four unsigned chars (RGBA), where the
 * opacity defaults to 255, but can be modified separately to the other
 * components. Ideally we would use a lightweight color class to store and pass
 * around colors.
 */

#ifndef vtkBrush_h
#define vtkBrush_h

#include "vtkColor.h" // Needed for vtkColor4ub
#include "vtkObject.h"
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKRENDERINGCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkBrush : public vtkObject
{
public:
  vtkTypeMacro(vtkBrush, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkBrush* New();

  /**
   * Set the color of the brush with three component doubles (RGB), ranging from
   * 0.0 to 1.0.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetColorF(double color[3]);

  /**
   * Set the color of the brush with three component doubles (RGB), ranging from
   * 0.0 to 1.0.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetColorF(double r, double g, double b);

  /**
   * Set the color of the brush with four component doubles (RGBA), ranging from
   * 0.0 to 1.0.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetColorF(double r, double g, double b, double a);

  /**
   * Set the opacity with a double, ranging from 0.0 (transparent) to 1.0
   * (opaque).
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetOpacityF(double a);

  /**
   * Get the opacity ranging from 0.0 (transparent) to 1.0(opaque).
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  double GetOpacityF();

  /**
   * Set the color of the brush with three component unsigned chars (RGB),
   * ranging from 0 to 255.
   */
  void SetColor(unsigned char color[3]);

  /**
   * Set the color of the brush with three component unsigned chars (RGB),
   * ranging from 0 to 255.
   */
  void SetColor(unsigned char r, unsigned char g, unsigned char b);

  ///@{
  /**
   * Set the color of the brush with four component unsigned chars (RGBA),
   * ranging from 0 to 255.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void SetColor(const vtkColor4ub& color);
  ///@}

  /**
   * Set the opacity with an unsigned char, ranging from 0 (transparent) to 255
   * (opaque).
   */
  void SetOpacity(unsigned char a);

  /**
   * Get the opacity ranging from 0 (transparent) to 255(opaque).
   */
  unsigned char GetOpacity();

  /**
   * Get the color of the brush - expects a double of length 4 to copy into.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void GetColorF(double color[4]);

  /**
   * Get the color of the brush - expects an unsigned char of length 4.
   */
  void GetColor(unsigned char color[4]);

  /**
   * Get the color of the brush - gives a pointer to the underlying data.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  unsigned char* GetColor() { return &this->Color[0]; }

  /**
   * Get the color of the brush.
   */
  vtkColor4ub GetColorObject();

  /**
   * Set the texture that will be used to fill polygons
   * By default, no texture is set. The image will be registered with the brush
   * (ref count is incremented)
   * To disable the texture, set Texture to 0.
   */
  void SetTexture(vtkImageData* image);

  ///@{
  /**
   * Get the texture that is used to fill polygons
   */
  vtkGetObjectMacro(Texture, vtkImageData);
  ///@}

  /**
   * Texture properties
   */
  enum TextureProperty
  {
    Nearest = 0x01,
    Linear = 0x02,
    Stretch = 0x04,
    Repeat = 0x08
  };

  ///@{
  /**
   * Set properties to the texture
   * By default, the texture is linearly stretched.
   * The behavior is undefined when Linear and Nearest are both set
   * The behavior is undefined when Stretch and Repeat are both set
   * The behavior is undefined if TextureProperties is 0
   */
  vtkSetMacro(TextureProperties, int);
  ///@}

  ///@{
  /**
   * Get the properties associated to the texture
   */
  vtkGetMacro(TextureProperties, int);
  ///@}

  /**
   * Make a deep copy of the supplied brush.
   */
  void DeepCopy(vtkBrush* brush);

protected:
  vtkBrush();
  ~vtkBrush() override;

  // Storage of the color in RGBA format (0-255 per channel).
  unsigned char* Color;
  vtkColor4ub BrushColor;
  vtkImageData* Texture;
  int TextureProperties;

private:
  vtkBrush(const vtkBrush&) = delete;
  void operator=(const vtkBrush&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkBrush_h
