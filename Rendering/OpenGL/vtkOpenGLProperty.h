/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLProperty
 * @brief   OpenGL property
 *
 * vtkOpenGLProperty is a concrete implementation of the abstract class
 * vtkProperty. vtkOpenGLProperty interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLProperty_h
#define vtkOpenGLProperty_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkProperty.h"

class vtkGLSLShaderDeviceAdapter2;
class vtkOpenGLRenderer;
class vtkOpenGLRenderWindow;
class vtkShader2;
class vtkShader2Collection;
class vtkShaderDeviceAdapter2;
class vtkShaderProgram2;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLProperty : public vtkProperty
{
public:
  static vtkOpenGLProperty *New();
  vtkTypeMacro(vtkOpenGLProperty, vtkProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Implement base class method.
   */
  void Render(vtkActor *a, vtkRenderer *ren) VTK_OVERRIDE;

  /**
   * Implement base class method.
   */
  void BackfaceRender(vtkActor *a, vtkRenderer *ren) VTK_OVERRIDE;

  /**
   * This method is called after the actor has been rendered.
   * Don't call this directly. This method cleans up
   * any shaders allocated.
   */
  void PostRender(vtkActor *a,
                          vtkRenderer *r) VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this
   * property. The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *win) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the shader program of the vtkProp. It can be set directly or
   * by defining a Material.
   */
  vtkGetObjectMacro(PropProgram, vtkShaderProgram2);
  void SetPropProgram(vtkShaderProgram2 *);
  //@}

  /**
   * Get the object that can pass vertex attribute to a vtkShaderProgram2.
   */
  vtkShaderDeviceAdapter2* GetShaderDeviceAdapter2() VTK_OVERRIDE;

  //@{
  /**
   * Get the vtkShaderProgram2 object in use.
   */
  vtkGetObjectMacro(CurrentShaderProgram2, vtkShaderProgram2);
  //@}

  //@{
  /**
   * Provide values to initialize shader variables.
   * Useful to initialize shader variables that change over time
   * (animation, GUI widgets inputs, etc. )
   * - \p name - hardware name of the uniform variable
   * - \p numVars - number of variables being set
   * - \p x - values
   */
  void AddShaderVariable(const char *name, int numVars, int *x) VTK_OVERRIDE;
  void AddShaderVariable(const char *name, int numVars, float *x) VTK_OVERRIDE;
  void AddShaderVariable(const char *name, int numVars, double *x) VTK_OVERRIDE;
  //@}

  /**
   * Helper method to set OpenGL material properties.
   */
  static void SetMaterialProperties(unsigned int face,
    double ambient, const double ambient_color[3],
    double diffuse, const double diffuse_color[3],
    double specular, const double specular_color[3], double specular_power,
    double opacity, vtkOpenGLRenderWindow* context);

protected:
  vtkOpenGLProperty();
  ~vtkOpenGLProperty() VTK_OVERRIDE;

  /**
   * Method called in vtkOpenGLProperty::Render() to render shaders and/or
   * related entities like shader variables. Returns true if any shaders were
   * rendered.
   */
  bool RenderShaders(vtkActor* actor, vtkRenderer* renderer);

  /**
   * Method called in vtkOpenGLProperty::Render() to render textures.
   * Last argument is the value returned from RenderShaders() call.
   */
  bool RenderTextures(vtkActor* actor, vtkRenderer* renderer,
    bool using_shader_program2);

  /**
   * Load OpenGL extensions for multi texturing.
   */
  void LoadMultiTexturingExtensions(vtkRenderer* ren);

  // Owned. Result of merging the shader program of the renderer
  // and the PropProgram.
  vtkShaderProgram2 *CachedShaderProgram2;

  vtkShaderProgram2 *LastRendererShaderProgram2; // just a ref
  vtkShaderProgram2 *LastPropProgram; // just a ref
  vtkShaderProgram2 *PropProgram; // owned

  // Point to CachedShaderProgram2 if Shading is on and the context
  // supports it.
  vtkShaderProgram2 *CurrentShaderProgram2;

  vtkShader2 *DefaultMainVS;
  vtkShader2 *DefaultMainFS;
  vtkShader2 *DefaultPropVS;
  vtkShader2 *DefaultPropFS;
  bool UseDefaultMainVS;
  bool UseDefaultMainFS;
  bool UseDefaultPropVS;
  bool UseDefaultPropFS;
  vtkGLSLShaderDeviceAdapter2 *ShaderDeviceAdapter2;

private:
  vtkOpenGLProperty(const vtkOpenGLProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLProperty&) VTK_DELETE_FUNCTION;
};

#endif
