/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkShader
// .SECTION Description
// vtkShader is a base class for interfacing VTK to hardware shader
// libraries. vtkShader interprets a vtkXMLDataElement that describes a
// particular shader. Descendants of this class inherit this functionality and
// additionally interface to specific shader libraries like NVidia's Cg and
// OpenGL2.0 (GLSL) to perform operations, on individual shaders.
//
// During each render, the vtkShaderProgram calls Compile(),
// PassShaderVariables(), Bind() and after the actor has been rendered,
// calls Unbind(), in that order.
// .SECTION See Also
// vtkCgShader vtkGLSLShader
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at
// Sandia National Labs.

#ifndef __vtkShader_h
#define __vtkShader_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkActor;
class vtkCamera;
class vtkLight;
class vtkProperty;
class vtkRenderer;
class vtkShaderInternals;
class vtkTexture;
class vtkWindow;
class vtkXMLDataElement;
class vtkXMLShader;

class VTKRENDERINGCORE_EXPORT vtkShader : public vtkObject
{
public:
  vtkTypeMacro(vtkShader, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Called to compile the shader code.
  // The subclasses must only compile the code in this method.
  // Returns if the compile was successful.
  // Subclasses should compile the code only if it was not
  // already compiled.
  virtual int Compile() =0;

  // Description:
  // Called to pass VTK actor/property/light values and other
  // Shader variables over to the shader. This is called by the ShaderProgram
  // during each render.
  virtual void PassShaderVariables(vtkActor* actor, vtkRenderer* ren);

  // Description:
  // In this method the shader can enable/bind itself. This is applicable
  // only to Cg, since in GLSL, individual shaders in a program can't be
  // enabled/bound.
  virtual void  Bind() { }

  // Description:
  // Called to unbind the shader. As with Bind(), this is only applicable
  // to Cg.
  virtual void Unbind() { }

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) { }

  // Description:
  // Get/Set the XMLShader representation for this shader.
  // A shader is not valid without a XMLShader.
  void SetXMLShader(vtkXMLShader*);
  vtkGetObjectMacro(XMLShader, vtkXMLShader);

  // Description:
  // Indicates if a variable by the given name exists.
  int HasShaderVariable(const char* name);

  // Description:
  // Methods to add shader variables to this shader.
  // The shader variable type must match with that declared in
  // the Material xml, otherwise, the variable is not made available
  // to the shader.
  void AddShaderVariable(const char* name, int num_of_elements,
    const int *values);
  void AddShaderVariable(const char* name, int num_of_elements,
    const float *values);
  void AddShaderVariable(const char* name, int num_of_elements,
    const double *values);

  // Description:
  // Get number of elements in a Shader variable. Return 0 if
  // failed to find the shader variable.
  int GetShaderVariableSize(const char* name);

  // Description:
  // Returns the type of a Shader variable with the given name.
  // Return 0 on error.
  int GetShaderVariableType(const char* name);

  // Description:
  // Methods to get the value of shader variables with the given name.
  // Values must be at least the size of the shader variable (obtained
  // by GetShaderVariableSize(). Returns if the operation was successful.
  int GetShaderVariable(const char* name, int* values);
  int GetShaderVariable(const char* name, float* values);
  int GetShaderVariable(const char* name, double* values);

  // Description:
  // Returns the scope of the shader i.e. if it's a vertex or fragment shader.
  // (vtkXMLShader::SCOPE_VERTEX or vtkXMLShader::SCOPE_FRAGMENT).
  int GetScope();
protected:
  vtkShader();
  ~vtkShader();

  vtkXMLShader* XMLShader;
  vtkShaderInternals* Internals;

  //BTX
  enum MatrixOrders
    {
    RowMajor,
    ColumnMajor
    };
  //ETX

  // Description:
  // Runs through the XML element children to locate uniform
  // variable elements and process them.
  virtual void SetShaderParameters(vtkActor*, vtkRenderer*,
                                   vtkXMLDataElement*);


  // Description:
  // Processes <Uniform /> elements.
  void SetUniformParameter(vtkActor*, vtkRenderer*, vtkXMLDataElement*);

  // Description:
  // Processes <CameraUniform />
  void SetCameraParameter(vtkActor*, vtkRenderer*, vtkXMLDataElement*);

  // Description:
  // Processes <PropertyUniform />
  void SetPropertyParameter(vtkActor*, vtkRenderer*, vtkXMLDataElement*);

  // Description:
  // Processes <LightUniform />
  void SetLightParameter(vtkActor*, vtkRenderer*, vtkXMLDataElement*);

  // Description:
  // Process <MatrixUniform />
  void SetMatrixParameter(vtkActor*, vtkRenderer*, vtkXMLDataElement*);

  // Description:
  // Process <SamplerUniform />
  void SetSamplerParameter(vtkActor*, vtkRenderer*, vtkXMLDataElement*);

  // Description:
  // Process <ApplicationUniform />
  void SetApplicationParameter(vtkXMLDataElement*);

  // Description:
  // Equivalent to cgGLSetParameter and glUniform.
  // Subclasses must override these and perform GLSL or Cg calls.
  virtual void SetUniformParameter(const char* name, int numValues,
    const int* value) =0;
  virtual void SetUniformParameter(const char* name, int numValues,
    const float* value)=0;
  virtual void SetUniformParameter(const char* name, int numValues,
    const double* value)=0;

  // Description:
  // Equivalent to cgGLSetMatrixParameterfc and glUniformMatrix.
  // Subclasses must override these and perform GLSL or Cg calls.
  virtual void SetMatrixParameter(const char* name, int numValues,
    int order, const float* value)=0;
  virtual void SetMatrixParameter(const char* name, int numValues,
    int order, const double* value)=0;
  virtual void SetMatrixParameter(const char* name, const char* state_matix_type,
    const char* transform_type)=0;


  // Description:
  // Establishes the given texture as the uniform sampler to perform lookups on.
  // The textureIndex argument corresponds to the indices of the textures in a
  // vtkProperty.  Subclass may have to cast the texture to vtkOpenGLTexture to
  // obtain the GLuint for texture this texture.  Subclasses must override these
  // and perform GLSL or Cg calls.
  virtual void SetSamplerParameter(const char* name, vtkTexture* texture,
                                   int textureIndex)=0;

  vtkTimeStamp PassShaderVariablesTime;
private:
  vtkShader(const vtkShader&); // Not Implemented
  void operator=(const vtkShader&); // Not Implemented
};
#endif //__vtkShader_h
