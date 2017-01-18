/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRIBProperty
 * @brief   RIP Property
 *
 * vtkRIBProperty is a subclass of vtkProperty that allows the user to
 * specify named shaders for use with RenderMan. Both surface
 * and displacement shaders can be specified. Parameters
 * for the shaders can be declared and set.
 *
 * @sa
 * vtkRIBExporter vtkRIBLight
*/

#ifndef vtkRIBProperty_h
#define vtkRIBProperty_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkProperty.h"

class vtkRIBRenderer;

class VTKIOEXPORT_EXPORT vtkRIBProperty : public vtkProperty
{
public:
  static vtkRIBProperty *New();
  vtkTypeMacro(vtkRIBProperty,vtkProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * If true (default) the surface shader uses the usual shader parameters:
   * Ka - Ambient amount
   * Kd - Diffuse amount
   * Ks - Specular amount
   * Roughness
   * SpecularColor
   * Additional surface shader parameters can be added with the
   * Set/AddSurfaceShaderParameter methods.
   * If false, all surface shader parameters must be specified
   */
  vtkSetMacro(SurfaceShaderUsesDefaultParameters, bool);
  vtkGetMacro(SurfaceShaderUsesDefaultParameters, bool);
  vtkBooleanMacro(SurfaceShaderUsesDefaultParameters, bool);
  //@}

  //@{
  /**
   * Specify the name of a surface shader.
   */
  vtkSetStringMacro(SurfaceShader);
  vtkGetStringMacro(SurfaceShader);
  //@}

  //@{
  /**
   * Specify the name of a displacement shader.
   */
  vtkSetStringMacro(DisplacementShader);
  vtkGetStringMacro(DisplacementShader);
  //@}

  //@{
  /**
   * Specify declarations for variables..
   */
  void SetVariable (const char *variable, const char *declaration);
  void AddVariable (const char *variable, const char *declaration);
  //@}

  /**
   * Get variable declarations
   */
  char *GetDeclarations ();

  //@{
  /**
   * Specify parameter values for variables.
   * DEPRECATED: use (Set/Add)SurfaceShaderParameter instead.
   */
  void SetParameter (const char *parameter, const char *value);
  void AddParameter (const char *parameter, const char *value);
  //@}

  //@{
  /**
   * Specify parameter values for surface shader parameters
   */
  void SetSurfaceShaderParameter (const char *parameter, const char *value);
  void AddSurfaceShaderParameter (const char *parameter, const char *value);
  //@}

  //@{
  /**
   * Specify parameter values for displacement shader parameters
   */
  void SetDisplacementShaderParameter (const char *parameter, const char *value);
  void AddDisplacementShaderParameter (const char *parameter, const char *value);
  //@}

  //@{
  /**
   * Get parameters.
   */
  char *GetParameters (); // DEPRECATED: use GetSurfaceShaderParameters instead.
  char *GetSurfaceShaderParameters ();
  char *GetDisplacementShaderParameters ();
  //@}

protected:
  vtkRIBProperty();
  ~vtkRIBProperty() VTK_OVERRIDE;

  void Render(vtkActor *a, vtkRenderer *ren) VTK_OVERRIDE;
  vtkProperty *Property;
  char *SurfaceShader;
  char *DisplacementShader;
  char *Declarations;
  char *SurfaceShaderParameters;
  char *DisplacementShaderParameters;
  bool SurfaceShaderUsesDefaultParameters;

private:
  vtkRIBProperty(const vtkRIBProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRIBProperty&) VTK_DELETE_FUNCTION;
};

#endif
