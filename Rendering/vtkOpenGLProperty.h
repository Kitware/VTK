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
// .NAME vtkOpenGLProperty - OpenGL property
// .SECTION Description
// vtkOpenGLProperty is a concrete implementation of the abstract class 
// vtkProperty. vtkOpenGLProperty interfaces to the OpenGL rendering library.

#ifndef __vtkOpenGLProperty_h
#define __vtkOpenGLProperty_h

#include "vtkProperty.h"

class vtkOpenGLRenderer;
class vtkShader2;
class vtkShader2Collection;
class vtkShaderProgram2;
class vtkGLSLShaderDeviceAdapter2;

class VTK_RENDERING_EXPORT vtkOpenGLProperty : public vtkProperty
{
public:
  static vtkOpenGLProperty *New();
  vtkTypeRevisionMacro(vtkOpenGLProperty,vtkProperty);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkActor *a, vtkRenderer *ren);

  // Description:
  // Implement base class method.
  void BackfaceRender(vtkActor *a, vtkRenderer *ren);
  
  //BTX
  // Description:
  // This method is called after the actor has been rendered.
  // Don't call this directly. This method cleans up
  // any shaders allocated.
  virtual void PostRender(vtkActor *a,
                          vtkRenderer *r);
  
  // Description:
  // Release any graphics resources that are being consumed by this
  // property. The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *win);
  
  // Description:
  // Get the collection of property shaders. If Material is not set/or not
  // loaded properly, this will return null.
  vtkGetObjectMacro(Shader2Collection,vtkShader2Collection);
  void SetShader2Collection(vtkShader2Collection *);
  
  // Description:
  // Get the object that can pass vertex attribute to a vtkShaderProgram2.
  vtkGetObjectMacro(ShaderDeviceAdapter2,vtkGLSLShaderDeviceAdapter2);
  
  // Description:
  // Get the vtkShaderProgram2 object in use.
  vtkGetObjectMacro(CurrentShaderProgram2,vtkShaderProgram2);
  //ETX
  
protected:
  vtkOpenGLProperty();
  ~vtkOpenGLProperty();

  // Description:
  // Load OpenGL extensions for multi texturing.
  void LoadMultiTexturingExtensions(vtkRenderer* ren);
  
  // Description:
  // Read this->Material from new style shaders.
  virtual void ReadFrameworkMaterial();
  
  vtkShader2Collection *Shader2Collection;
  
  vtkShaderProgram2 *CachedShaderProgram2; // owned
  vtkShaderProgram2 *LastCachedShaderProgram2; // just a ref
  vtkShaderProgram2 *PropProgram; // owned
  
  vtkShader2 *DefaultMainVS;
  vtkShader2 *DefaultMainFS;
  vtkShader2 *DefaultPropVS;
  vtkShader2 *DefaultPropFS;
  bool UseDefaultMainVS;
  bool UseDefaultMainFS;
  bool UseDefaultPropVS;
  bool UseDefaultPropFS;
  vtkGLSLShaderDeviceAdapter2 *ShaderDeviceAdapter2;
  
  vtkShaderProgram2 *CurrentShaderProgram2; // point to PropProgram or CachedShaderProgram2
  
private:
  vtkOpenGLProperty(const vtkOpenGLProperty&);  // Not implemented.
  void operator=(const vtkOpenGLProperty&);  // Not implemented.
};

#endif
