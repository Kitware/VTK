// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointGaussianMapper
 * @brief   draw PointGaussians using imposters
 *
 *
 * A mapper that uses imposters to draw gaussian splats or other shapes if
 * custom shader code is set. Supports transparency and picking as well. It
 * draws all the points and does not require cell arrays. If cell arrays are
 * provided it will only draw the points used by the Verts cell array. The shape
 * of the imposter is a triangle.
 */

#ifndef vtkPointGaussianMapper_h
#define vtkPointGaussianMapper_h

#include "vtkPolyDataMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPiecewiseFunction;

class VTKRENDERINGCORE_EXPORT vtkPointGaussianMapper : public vtkPolyDataMapper
{
public:
  static vtkPointGaussianMapper* New();
  vtkTypeMacro(vtkPointGaussianMapper, vtkPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the optional scale transfer function. This is only
   * used when a ScaleArray is also specified.
   */
  void SetScaleFunction(vtkPiecewiseFunction*);
  vtkGetObjectMacro(ScaleFunction, vtkPiecewiseFunction);
  ///@}

  ///@{
  /**
   * The size of the table used in computing scale, used when
   * converting a vtkPiecewiseFunction to a table
   */
  vtkSetMacro(ScaleTableSize, int);
  vtkGetMacro(ScaleTableSize, int);
  ///@}

  ///@{
  /**
   * Convenience method to set the array to scale with.
   */
  vtkSetStringMacro(ScaleArray);
  vtkGetStringMacro(ScaleArray);
  ///@}

  ///@{
  /**
   * Convenience method to set the component of the array to scale with.
   */
  vtkSetMacro(ScaleArrayComponent, int);
  vtkGetMacro(ScaleArrayComponent, int);
  ///@}

  ///@{
  /**
   * If enabled, the gaussian can be stretched and oriented on some direction.
   * A 3x3 covariance matrix is built using the scale array and the rotation array.
   * Since a 3D vector is expected, the value in ScaleArrayComponent is ignored.
   * Default value is false.
   */
  vtkSetMacro(Anisotropic, bool);
  vtkGetMacro(Anisotropic, bool);
  vtkBooleanMacro(Anisotropic, bool);
  ///@}

  ///@{
  /**
   * Set the default scale factor of the point gaussians.  This
   * defaults to 1.0. All radius computations will be scaled by the factor
   * including the ScaleArray. If a vtkPiecewideFunction is used the
   * scaling happens prior to the function lookup.
   * A scale factor of 0.0 indicates that the splats should be rendered
   * as simple points.
   */
  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);
  ///@}

  ///@{
  /**
   * Set the array containing the rotation of each point.
   * The array must contain quaternions (4 components).
   * Must be specified is Anisotropic is true.
   * Default value is nullptr.
   */
  vtkSetStringMacro(RotationArray);
  vtkGetStringMacro(RotationArray);
  ///@}

  ///@{
  /**
   * Treat the points/splats as emissive light sources. The default is true.
   */
  vtkSetMacro(Emissive, vtkTypeBool);
  vtkGetMacro(Emissive, vtkTypeBool);
  vtkBooleanMacro(Emissive, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the optional opacity transfer function. This is only
   * used when an OpacityArray is also specified.
   */
  void SetScalarOpacityFunction(vtkPiecewiseFunction*);
  vtkGetObjectMacro(ScalarOpacityFunction, vtkPiecewiseFunction);
  ///@}

  ///@{
  /**
   * The size of the table used in computing opacities, used when
   * converting a vtkPiecewiseFunction to a table
   */
  vtkSetMacro(OpacityTableSize, int);
  vtkGetMacro(OpacityTableSize, int);
  ///@}

  ///@{
  /**
   * Method to set the optional opacity array.  If specified this
   * array will be used to generate the opacity values.
   */
  vtkSetStringMacro(OpacityArray);
  vtkGetStringMacro(OpacityArray);
  ///@}

  ///@{
  /**
   * Convenience method to set the component of the array to opacify with.
   */
  vtkSetMacro(OpacityArrayComponent, int);
  vtkGetMacro(OpacityArrayComponent, int);
  ///@}

  ///@{
  /**
   * Method to override the fragment shader code for the splat.  You can
   * set this to draw other shapes. For the OPenGL2 backend some of
   * the variables you can use and/or modify include,
   * opacity - 0.0 to 1.0
   * diffuseColor - vec3
   * ambientColor - vec3
   * offsetVCVSOutput - vec2 offset in view coordinates from the splat center
   */
  vtkSetStringMacro(SplatShaderCode);
  vtkGetStringMacro(SplatShaderCode);
  ///@}

  ///@{
  /**
   * When drawing splats as opposed to point mode (splats are bigger than a pixel)
   * this controls how large the splat bound primitive will be. By default it
   * is large enough to contain a circle of radius 3.0*scale which works
   * well for gaussian splats as after 3.0 standard deviations the
   * opacity is near zero. For custom shader codes a different
   * value can be used. Generally you should use the lowest value you can
   * as it will result in fewer fragments. For example if your custom
   * shader only draws a disc of radius 1.0*scale, then set this to 1.0
   * to avoid sending many fragments to the shader that will just get
   * discarded.
   */
  vtkSetMacro(BoundScale, float);
  vtkGetMacro(BoundScale, float);
  ///@}

  ///@{
  /**
   * Once the 2D covariance matrix is computed, it's possible to add a custom
   * low pass matrix to apply a convolution to the splat.
   * It's useful to make sure the splat is at least one pixel wide for example.
   * The 2x2 matrix to apply is stored as a 3D vector because it's symmetric.
   * The first element is the first diagonal value, the second element is the
   * off diagonal value, and the third element is the second diagonal value.
   * Default is zero, meaning no convolution is applied.
   */
  vtkSetVector3Macro(LowpassMatrix, float);
  vtkGetVector3Macro(LowpassMatrix, float);
  ///@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override { return true; }

protected:
  vtkPointGaussianMapper();
  ~vtkPointGaussianMapper() override;

  char* ScaleArray;
  int ScaleArrayComponent;
  char* OpacityArray;
  int OpacityArrayComponent;
  char* SplatShaderCode;

  vtkPiecewiseFunction* ScaleFunction;
  int ScaleTableSize;

  vtkPiecewiseFunction* ScalarOpacityFunction;
  int OpacityTableSize;

  double ScaleFactor;
  vtkTypeBool Emissive;

  float BoundScale;

private:
  vtkPointGaussianMapper(const vtkPointGaussianMapper&) = delete;
  void operator=(const vtkPointGaussianMapper&) = delete;

  char* RotationArray = nullptr;
  float LowpassMatrix[3] = { 0.f, 0.f, 0.f };
  bool Anisotropic = false;
};

VTK_ABI_NAMESPACE_END
#endif
