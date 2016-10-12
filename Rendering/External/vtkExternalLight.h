/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalLight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExternalLight
 * @brief   a virtual light object for tweaking existing lights
 * in an external 3D rendering context
 *
 *
 * vtkExternalLight is a virtual light object for tweaking existing lights in
 * an external 3D rendering context. It provides a mechanism to adjust and
 * control parameters of existing lights in an external OpenGL context.
 *
 * It provides methods to locate and point the light,
 * and set its brightness and color. In addition to the
 * basic infinite distance point light source attributes, you can also specify
 * the light attenuation values and cone angle. These attributes are only used
 * if the light is a positional light.
 *
 * By default, vtkExternalLight overrides specific light parameters as set by
 * the user. Setting the #ReplaceMode to ALL_PARAMS, will set all
 * the light parameter values to the ones set in vtkExternalLight.
 *
 * @warning
 * Use the vtkExternalLight object to tweak parameters of lights created in the
 * external context. This class does NOT create new lights in the scene.
 *
 * @par Example:
 * Usage example for vtkExternalLight in conjunction with
 * vtkExternalOpenGLRenderer and \ref ExternalVTKWidget
 * \code{.cpp}
 *    vtkNew<vtkExternalLight> exLight;
 *    exLight->SetLightIndex(GL_LIGHT0); // GL_LIGHT0 identifies the external light
 *    exLight->SetDiffuseColor(1.0, 0.0, 0.0); // Changing diffuse color
 *    vtkNew<ExternalVTKWidget> exWidget;
 *    vtkExternalOpenGLRenderer* ren = vtkExternalOpenGLRenderer::SafeDownCast(exWidget->AddRenderer());
 *    ren->AddExternalLight(exLight.GetPointer());
 * \endcode
 *
 * @sa
 * vtkExternalOpenGLRenderer \ref ExternalVTKWidget
*/

#ifndef vtkExternalLight_h
#define vtkExternalLight_h

#include "vtkRenderingExternalModule.h" // For export macro
#include "vtkLight.h"

class VTKRENDERINGEXTERNAL_EXPORT vtkExternalLight : public vtkLight
{
public:
  vtkTypeMacro(vtkExternalLight, vtkLight);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Create an external light object with the focal point at the origin and its
   * position set to (0,0,1). The light is a Headlight, its color is white
   * (black ambient, white diffuse, white specular),
   * intensity=1, the light is turned on, positional lighting is off,
   * ConeAngle=30, AttenuationValues=(1,0,0), Exponent=1 and the
   * TransformMatrix is NULL. The light index is GL_LIGHT0, which means the
   * existing light with index GL_LIGHT0 will be affected by this light.
   */
  static vtkExternalLight *New();

  enum ReplaceModes
  {
    INDIVIDUAL_PARAMS   = 0, // default
    ALL_PARAMS          = 1
  };

  //@{
  /**
   * Set/Get light index
   * This should be the OpenGL light identifier. (e.g.: GL_LIGHT0)
   * (Default: GL_LIGHT0)
   */
  vtkSetMacro(LightIndex, int);
  vtkGetMacro(LightIndex, int);
  //@}

  //@{
  /**
   * Set/Get replace mode
   * This determines how this ExternalLight will be used to tweak parameters on
   * an existing light in the rendering context.
   * (Default: INDIVIDUAL_PARAMS)

   * \li \c vtkExternalLight::INDIVIDUAL_PARAMS : Replace parameters
   * specifically set by the user by calling the parameter
   * Set method. (e.g. SetDiffuseColor())

   * \li \c vtkExternalLight::ALL_PARAMS : Replace all
   * parameters of the light with the parameters in vtkExternalLight object.
   */
  vtkSetMacro(ReplaceMode, int);
  vtkGetMacro(ReplaceMode, int);
  //@}

  /**
   * Override Set method to keep a record of changed value
   */
  void SetPosition(double, double, double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetFocalPoint(double, double, double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetAmbientColor(double, double, double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetDiffuseColor(double, double, double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetSpecularColor(double, double, double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetIntensity(double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetConeAngle(double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetAttenuationValues(double, double, double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetExponent(double);

  /**
   * Override Set method to keep a record of changed value
   */
  void SetPositional(int);

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(PositionSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(FocalPointSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(AmbientColorSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(DiffuseColorSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(SpecularColorSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(IntensitySet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(ConeAngleSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(AttenuationValuesSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(ExponentSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  vtkGetMacro(PositionalSet, bool);
  //@}

protected:
  vtkExternalLight();
  ~vtkExternalLight();

  int LightIndex;
  int ReplaceMode;

  bool PositionSet;
  bool FocalPointSet;
  bool AmbientColorSet;
  bool DiffuseColorSet;
  bool SpecularColorSet;
  bool IntensitySet;
  bool ConeAngleSet;
  bool AttenuationValuesSet;
  bool ExponentSet;
  bool PositionalSet;

private:
  vtkExternalLight(const vtkExternalLight&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExternalLight&) VTK_DELETE_FUNCTION;
};

#endif // vtkExternalLight_h
