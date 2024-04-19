// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkButtonSource
 * @brief   abstract class for creating various button types
 *
 * vtkButtonSource is an abstract class that defines an API for creating
 * "button-like" objects in VTK. A button is a geometry with a rectangular
 * region that can be textured. The button is divided into two regions: the
 * texture region and the shoulder region. The points in both regions are
 * assigned texture coordinates. The texture region has texture coordinates
 * consistent with the image to be placed on it.  All points in the shoulder
 * regions are assigned a texture coordinate specified by the user.  In this
 * way the shoulder region can be colored by the texture.
 *
 * Creating a vtkButtonSource requires specifying its center point.
 * (Subclasses have other attributes that must be set to control
 * the shape of the button.) You must also specify how to control
 * the shape of the texture region; i.e., whether to size the
 * texture region proportional to the texture dimensions or whether
 * to size the texture region proportional to the button. Also, buttons
 * can be created single sided are mirrored to create two-sided buttons.
 *
 * @sa
 * vtkEllipticalButtonSource vtkRectangularButtonSource
 *
 * @warning
 * The button is defined in the x-y plane. Use vtkTransformPolyDataFilter
 * or vtkGlyph3D to orient the button in a different direction.
 */

#ifndef vtkButtonSource_h
#define vtkButtonSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_TEXTURE_STYLE_FIT_IMAGE 0
#define VTK_TEXTURE_STYLE_PROPORTIONAL 1

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkButtonSource : public vtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkButtonSource, vtkPolyDataAlgorithm);

  ///@{
  /**
   * Specify a point defining the origin (center) of the button.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVectorMacro(Center, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the style of the texture region: whether to size it
   * according to the x-y dimensions of the texture, or whether to make
   * the texture region proportional to the width/height of the button.
   */
  vtkSetClampMacro(TextureStyle, int, VTK_TEXTURE_STYLE_FIT_IMAGE, VTK_TEXTURE_STYLE_PROPORTIONAL);
  vtkGetMacro(TextureStyle, int);
  void SetTextureStyleToFitImage() { this->SetTextureStyle(VTK_TEXTURE_STYLE_FIT_IMAGE); }
  void SetTextureStyleToProportional() { this->SetTextureStyle(VTK_TEXTURE_STYLE_PROPORTIONAL); }
  ///@}

  ///@{
  /**
   * Set/get the texture dimension. This needs to be set if the texture
   * style is set to fit the image.
   */
  vtkSetVector2Macro(TextureDimensions, int);
  vtkGetVector2Macro(TextureDimensions, int);
  ///@}

  ///@{
  /**
   * Set/Get the default texture coordinate to set the shoulder region to.
   */
  vtkSetVector2Macro(ShoulderTextureCoordinate, double);
  vtkGetVector2Macro(ShoulderTextureCoordinate, double);
  ///@}

  ///@{
  /**
   * Indicate whether the button is single or double sided. A double sided
   * button can be viewed from two sides...it looks sort of like a "pill."
   * A single-sided button is meant to viewed from a single side; it looks
   * like a "clam-shell."
   */
  vtkSetMacro(TwoSided, vtkTypeBool);
  vtkGetMacro(TwoSided, vtkTypeBool);
  vtkBooleanMacro(TwoSided, vtkTypeBool);
  ///@}

protected:
  vtkButtonSource();
  ~vtkButtonSource() override = default;

  double Center[3];
  double ShoulderTextureCoordinate[2];
  int TextureStyle;
  int TextureDimensions[2];
  vtkTypeBool TwoSided;

private:
  vtkButtonSource(const vtkButtonSource&) = delete;
  void operator=(const vtkButtonSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
