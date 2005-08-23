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
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShaderProgram.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
// .NAME vtkGLSLShaderProgram
// .SECTION Description
// vtkGLSLShaderProgram is a concerete implementation of vtkShaderProgram.
// It's main function is to 'Link' a vertex and a fragment shader together
// and install them into the rendering pipeline by calling OpenGL2.0.
//
// Initization of shader parameters is delegated to instances of vtkShader
// (vtkGLSLShader in this case).

#ifndef __vtkGLSLShaderProgram_h
#define __vtkGLSLShaderProgram_h

#include "vtkShaderProgram.h"

#if 0
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#else
#include <vtkgl.h>
#endif

class vtkGLSLShader;
class vtkXMLDataElement;
class vtkRenderWindow;

class VTK_RENDERING_EXPORT vtkGLSLShaderProgram : public vtkShaderProgram
{
public:
  vtkTypeRevisionMacro(vtkGLSLShaderProgram, vtkShaderProgram);
  static vtkGLSLShaderProgram *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  // .Description:
  // Take shader from it's source (file and/or string) and load, compile, and
  // install in hardware. Also, initialize uniform variables originating from
  // the following sources: XML material file, vtkProperty, vtkLight, vtkCamera,
  // and application-specified uniform variables.
  virtual void Render(vtkActor *actor, vtkRenderer *renderer);
  
  virtual vtkGLSLShader* GetGLSLVertex();
  virtual vtkGLSLShader* GetGLSLFragment();

  bool IsProgram();
  bool IsLinked();
  bool IsAttached(GLuint handle);

  void GetProgramInfo();
  void GetInfoLog();

protected:
  vtkGLSLShaderProgram();
  virtual ~vtkGLSLShaderProgram();

  //BTX
  GLuint Program;
  GLint Linked;
  //ETX

  virtual void Link();

  vtkSetStringMacro( Info );
  vtkGetStringMacro( Info );
  char* Info;

private:
  vtkGLSLShaderProgram(const vtkGLSLShaderProgram&); // Not Implemented
  void operator=(const vtkGLSLShaderProgram&); // Not Implemented
};
#endif //__vtkGLSLShaderProgram_h
