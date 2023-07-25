// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImplicitTextureCoords
 * @brief   generate 1D, 2D, or 3D texture coordinates based on implicit function(s)
 *
 * vtkImplicitTextureCoords is a filter to generate 1D, 2D, or 3D texture
 * coordinates from one, two, or three implicit functions, respectively.
 * In combinations with a vtkBooleanTexture map (or another texture map of
 * your own creation), the texture coordinates can be used to highlight
 *(via color or intensity) or cut (via transparency) dataset geometry without
 * any complex geometric processing. (Note: the texture coordinates are
 * referred to as r-s-t coordinates.)
 *
 * The texture coordinates are automatically normalized to lie between (0,1).
 * Thus, no matter what the implicit functions evaluate to, the resulting
 * texture coordinates lie between (0,1), with the zero implicit function
 * value mapped to the 0.5 texture coordinates value. Depending upon the
 * maximum negative/positive implicit function values, the full (0,1) range
 * may not be occupied (i.e., the positive/negative ranges are mapped using
 * the same scale factor).
 *
 * A boolean variable InvertTexture is available to flip the texture
 * coordinates around 0.5 (value 1.0 becomes 0.0, 0.25->0.75). This is
 * equivalent to flipping the texture map (but a whole lot easier).
 *
 * @warning
 * You can use the transformation capabilities of vtkImplicitFunction to
 * orient, translate, and scale the implicit functions. Also, the dimension of
 * the texture coordinates is implicitly defined by the number of implicit
 * functions defined.
 *
 * @sa
 * vtkImplicitFunction vtkTexture vtkBooleanTexture vtkTransformTexture
 */

#ifndef vtkImplicitTextureCoords_h
#define vtkImplicitTextureCoords_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersTextureModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;

class VTKFILTERSTEXTURE_EXPORT vtkImplicitTextureCoords : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkImplicitTextureCoords, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create object with texture dimension=2 and no r-s-t implicit functions
   * defined and FlipTexture turned off.
   */
  static vtkImplicitTextureCoords* New();

  ///@{
  /**
   * Specify an implicit function to compute the r texture coordinate.
   */
  virtual void SetRFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(RFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Specify an implicit function to compute the s texture coordinate.
   */
  virtual void SetSFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(SFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Specify an implicit function to compute the t texture coordinate.
   */
  virtual void SetTFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(TFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * If enabled, this will flip the sense of inside and outside the implicit
   * function (i.e., a rotation around the r-s-t=0.5 axis).
   */
  vtkSetMacro(FlipTexture, vtkTypeBool);
  vtkGetMacro(FlipTexture, vtkTypeBool);
  vtkBooleanMacro(FlipTexture, vtkTypeBool);
  ///@}

protected:
  vtkImplicitTextureCoords();
  ~vtkImplicitTextureCoords() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkImplicitFunction* RFunction;
  vtkImplicitFunction* SFunction;
  vtkImplicitFunction* TFunction;
  vtkTypeBool FlipTexture;

private:
  vtkImplicitTextureCoords(const vtkImplicitTextureCoords&) = delete;
  void operator=(const vtkImplicitTextureCoords&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
