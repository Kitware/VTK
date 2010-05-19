/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShaderProgram.h

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

// .NAME vtkGLSLShaderProgram - GLSL Shader Program
// .SECTION Description
// vtkGLSLShaderProgram is a concerete implementation of vtkShaderProgram.
// It's main function is to 'Link' a vertex and a fragment shader together
// and install them into the rendering pipeline by calling OpenGL2.0.
//
// Initialization of shader parameters is delegated to instances of vtkShader
// (vtkGLSLShader in this case).
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at 
// Sandia National Labs.

#ifndef __vtkGLSLShaderProgram_h
#define __vtkGLSLShaderProgram_h

#include "vtkShaderProgram.h"

class vtkGLSLShader;
class vtkXMLDataElement;
class vtkRenderWindow;

class VTK_RENDERING_EXPORT vtkGLSLShaderProgram : public vtkShaderProgram
{
public:
  vtkTypeMacro(vtkGLSLShaderProgram, vtkShaderProgram);
  static vtkGLSLShaderProgram *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  // .Description:
  // Take shader from its source (file and/or string) and load, compile, and
  // install in hardware. Also, initialize uniform variables originating from
  // the following sources: XML material file, vtkProperty, vtkLight,
  // vtkCamera, and application-specified uniform variables.
  virtual void Render(vtkActor *actor, vtkRenderer *renderer);
  
  // Description:
  // Called to unload the shaders after the actor has been rendered.
  virtual void PostRender(vtkActor*, vtkRenderer*);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);
  
  // Description:
  // Return the OpenGL program id.
  unsigned int GetProgram()
    {return this->Program;}
  
protected:
  vtkGLSLShaderProgram();
  virtual ~vtkGLSLShaderProgram();

  // Description:
  // Creates and returns a new vtkGLSLShader.
  virtual vtkShader* NewShader();

  virtual void LoadExtensions(vtkRenderWindow*);

  unsigned int Program;
  int IsProgram();
  int IsLinked();
  int IsAttached(vtkGLSLShader* shader);
  virtual void Link();

  vtkSetStringMacro( Info );
  vtkGetStringMacro( Info );
  char* Info;
  void GetProgramInfo();
  void GetInfoLog();

private:
  vtkGLSLShaderProgram(const vtkGLSLShaderProgram&); // Not Implemented
  void operator=(const vtkGLSLShaderProgram&); // Not Implemented
};
#endif //__vtkGLSLShaderProgram_h
