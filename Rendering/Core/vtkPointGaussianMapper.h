/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointGaussianMapper
 * @brief   draw PointGaussians using imposters
 *
 *
 * An  mapper that uses imposters to draw gaussian splats or other shapes if
 * custom shader code is set. Supports transparency and picking as well. It
 * draws all the points and does not require cell arrays.  If cell arrays are
 * provided it will only draw the points used by the Verts cell array. The shape
 * of the imposter is a triangle.
*/

#ifndef vtkPointGaussianMapper_h
#define vtkPointGaussianMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPolyDataMapper.h"

class vtkPiecewiseFunction;

class VTKRENDERINGCORE_EXPORT vtkPointGaussianMapper : public vtkPolyDataMapper
{
public:
  static vtkPointGaussianMapper* New();
  vtkTypeMacro(vtkPointGaussianMapper, vtkPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the optional scale transfer function. This is only
   * used when a ScaleArray is also specified.
   */
  void SetScaleFunction(vtkPiecewiseFunction *);
  vtkGetObjectMacro(ScaleFunction,vtkPiecewiseFunction);
  //@}

  //@{
  /**
   * The size of the table used in computing scale, used when
   * converting a vtkPiecewiseFunction to a table
   */
  vtkSetMacro(ScaleTableSize, int);
  vtkGetMacro(ScaleTableSize, int);
  //@}

  //@{
  /**
   * Convenience method to set the array to scale with.
   */
  vtkSetStringMacro(ScaleArray);
  vtkGetStringMacro(ScaleArray);
  //@}

  //@{
  /**
   * Set the default scale factor of the point gaussians.  This
   * defaults to 1.0. All radius computations will be scaled by the factor
   * including the ScaleArray. If a vtkPiecewideFunction is used the
   * scaling happens prior to the function lookup.
   * A scale factor of 0.0 indicates that the splats should be rendered
   * as simple points.
   */
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);
  //@}

  //@{
  /**
   * Treat the points/splats as emissive light sources. The default is true.
   */
  vtkSetMacro(Emissive, int);
  vtkGetMacro(Emissive, int);
  vtkBooleanMacro(Emissive, int);
  //@}

  //@{
  /**
   * Set/Get the optional opacity transfer function. This is only
   * used when an OpacityArray is also specified.
   */
  void SetScalarOpacityFunction(vtkPiecewiseFunction *);
  vtkGetObjectMacro(ScalarOpacityFunction,vtkPiecewiseFunction);
  //@}

  //@{
  /**
   * The size of the table used in computing opacities, used when
   * converting a vtkPiecewiseFunction to a table
   */
  vtkSetMacro(OpacityTableSize, int);
  vtkGetMacro(OpacityTableSize, int);
  //@}

  //@{
  /**
   * Method to set the optional opacity array.  If specified this
   * array will be used to generate the opacity values.
   */
  vtkSetStringMacro(OpacityArray);
  vtkGetStringMacro(OpacityArray);
  //@}

  //@{
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
  //@}

  //@{
  /**
   * When drawing triangles as opposed too point mode
   * (triangles are for splats shaders that are bigger than a pixel)
   * this controls how large the triangle will be. By default it
   * is large enough to contain a cicle of radius 3.0*scale which works
   * well for gaussian splats as after 3.0 standard deviations the
   * opacity is near zero. For custom shader codes a different
   * value can be used. Generally you should use the lowest value you can
   * as it will result in fewer fragments. For example if your custom
   * shader only draws a disc of radius 1.0*scale, then set this to 1.0
   * to avoid sending many fragments to the shader that will just get
   * discarded.
   */
  vtkSetMacro(TriangleScale,float);
  vtkGetMacro(TriangleScale,float);
  //@}

protected:
  vtkPointGaussianMapper();
  ~vtkPointGaussianMapper() VTK_OVERRIDE;

  char *ScaleArray;
  char *OpacityArray;
  char *SplatShaderCode;

  vtkPiecewiseFunction *ScaleFunction;
  int ScaleTableSize;

  vtkPiecewiseFunction *ScalarOpacityFunction;
  int OpacityTableSize;

  double ScaleFactor;
  int Emissive;

  float TriangleScale;

private:
  vtkPointGaussianMapper(const vtkPointGaussianMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointGaussianMapper&) VTK_DELETE_FUNCTION;
};

#endif
