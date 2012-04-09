/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCgShaderProgram.h

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

// .NAME vtkCgShaderProgram - Cg Shader Program
// .SECTION Description
// vtkCgShaderProgram allows vtkShaderProperty (later vtkProperty)
// to treat a vertex/fragment shader pair as a single unit for the purpose
// of setting their common material library and encapsulating shader operation:
// shader installation and variable initialization.
//
// Since the interface between Cg shaders is only resolved at runtime (shader
// runtime that is, after they've been installed on the card), Cg does not
// have the concept of a shader Program. This class simply delegates shader
// program functions to its delegate vtkCgShaders.
//
// .Section See Also:
// vtkShaderBase, vtkShader, vtkCgShader, vtkShaderProgram
// .SECTION Thanks
// Shader support in VTK includes key contributions by Gary Templet at
// Sandia National Labs.

#ifndef __vtkCgShaderProgram_h
#define __vtkCgShaderProgram_h

#include "vtkShaderProgram.h"

class vtkActor;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkCgShaderProgram : public vtkShaderProgram
{
public:
  vtkTypeMacro(vtkCgShaderProgram, vtkShaderProgram);
  static vtkCgShaderProgram *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  // .Description:
  // Take shader from its source (file and/or string) and load, compile, and
  // install in hardware. Also, initialize uniform variables originating from
  // the following sources: XML material file, vtkProperty, vtkLight,
  // vtkCamera, and application-specified uniform variables.
  //
  // Delegates to vtkShader.
  virtual void Render(vtkActor*, vtkRenderer*);

protected:
  vtkCgShaderProgram();
  ~vtkCgShaderProgram();

  // Description:
  // Creates and returns a new vtkCgShader.
  virtual vtkShader* NewShader();
private:
  vtkCgShaderProgram(const vtkCgShaderProgram&); // Not Implemented
  void operator=(const vtkCgShaderProgram&); // Not Implemented
};
#endif //__vtkCgShaderProgram_h
