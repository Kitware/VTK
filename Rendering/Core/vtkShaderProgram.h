/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderProgram.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkShaderProgram
// .SECTION Description
// vtkShaderProgram is a superclass for managing Hardware Shaders
// defined in the XML Material file and interfacing VTK to those shaders.
// It's concrete descendants are responsible for installing vertex and
// fragment programs to the graphics hardware.
//
// .SECTION Shader Operations are shader library operations that are performed
// on individual shaders, that is, without consideration of the partner shader.
//
// .SECTION Program Operations are shader library operations that treat the
// vertex and fragment shader as a single unit.
//
// .SECTION Design
// This class is a Strategy pattern for 'Program' operations, which treat
// vertex/fragment shader pairs as a single 'Program', as required by some
// shader libraries (GLSL). Typically, 'Shader' operations are delegated
// to instances of vtkShader (managed by descendants of this class)
// while 'Program' operations are handled by descendants of this class,
// vtkCgShaderProgram, vtkGLSLShaderProgram.
//
// .SECTION See Also
// vtkCgShaderProgram, vtkGLSLShaderProgram
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at
// Sandia National Labs.

#ifndef __vtkShaderProgram_h
#define __vtkShaderProgram_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkActor;
class vtkCollection;
class vtkCollectionIterator;
class vtkRenderer;
class vtkRenderWindow;
class vtkShader;
class vtkWindow;
class vtkXMLMaterial;
class vtkShaderDeviceAdapter;

// manages all shaders defined in the XML file
// especially the part about sending things to the card
class VTKRENDERINGCORE_EXPORT vtkShaderProgram : public vtkObject
{
public:
  vtkTypeMacro(vtkShaderProgram, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  // .Description:
  // Accessors for the Material.
  vtkGetObjectMacro( Material, vtkXMLMaterial);
  virtual void SetMaterial( vtkXMLMaterial* );

  // .Description:
  // Add shaders. Returns the index of the shader.
  int AddShader(vtkShader* shader);

  // Description:
  // Remove a shader at the given index.
  void RemoveShader(int index);

  // Description:
  // Removes the given shader.
  void RemoveShader(vtkShader* shader);

  // Description:
  // Returns a new iterator to iterate over the shaders.
  vtkCollectionIterator* NewShaderIterator();

  // Description:
  // Returns the number of shaders available in this
  // shader program.
  int GetNumberOfShaders();

  // .Description
  // This static function creates concrete shaders of a specific type. This is
  // used to create a shader of the langauge specified in the XML file.
  static vtkShaderProgram* CreateShaderProgram( int type );

  // .Description
  // Read the material file to get necessary shader info. Synchronize with
  // delegate shaders.
  virtual void ReadMaterial();

  // .Description
  // Load, compile, install and initialize shaders. These operations may
  // be delegated to the shaders themselves or handled in descendants of
  // this class.
  virtual void Render( vtkActor*, vtkRenderer* )=0;

  //Description
  // Provide values to initialize shader variables. This is a conduit to initialize
  // shader variables that change over time, useful for animation, gui widget inputs,
  // etc.
  // name - hardware name of the uniform variable
  // numVars - number of variables being set
  // x - values
  virtual void AddShaderVariable(const char* name, int numVars, int* x);
  virtual void AddShaderVariable(const char* name, int numVars, float* x);
  virtual void AddShaderVariable(const char* name, int numVars, double* x);

  // Description:
  // Called to unload the shaders after the actor has been rendered.
  virtual void PostRender(vtkActor*, vtkRenderer*);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the vtkShaderDeviceAdapter which can be used to execute this
  // shader program.
  vtkGetObjectMacro(ShaderDeviceAdapter, vtkShaderDeviceAdapter);

protected:
  vtkShaderProgram();
  ~vtkShaderProgram();

  vtkXMLMaterial* Material;
  vtkCollection* ShaderCollection;
  vtkCollectionIterator* ShaderCollectionIterator;

  vtkSetMacro(GLExtensionsLoaded, int);
  vtkGetMacro(GLExtensionsLoaded, int);
  int GLExtensionsLoaded;
  virtual void LoadExtensions(vtkRenderWindow*) {}

  // Subclasses must set the shader device apater of the right type.
  void SetShaderDeviceAdapter(vtkShaderDeviceAdapter*);

  // Description:;
  // Must be overloaded by subclasses to create the shader of appropriate type.
  virtual vtkShader* NewShader() =0;
private:
  vtkShaderProgram(const vtkShaderProgram&); // Not Implemented
  void operator=(const vtkShaderProgram&); // Not Implemented

  vtkShaderDeviceAdapter* ShaderDeviceAdapter;
};
#endif //__vtkShaderProgram_h
