/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShaderProgram.cxx

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


#include "vtkGLSLShaderProgram.h"

#include "vtkActor.h"
#include "vtkCollectionIterator.h"
#include "vtkGLSLShader.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkXMLDataElement.h"

#include <vtkstd/vector>
// GL/glu.h is needed for the error reporting, this should be removed
// after the initial development phase.
//#include <GL/glu.h>
#include <vtkgl.h>

#if 1
#define printOpenGLError() printOglError(__FILE__, __LINE__)
int printOglError(char *vtkNotUsed(file), int vtkNotUsed(line))
  {
  // Returns 1 if an OpenGL error occurred, 0 otherwise.
  //
  GLenum glErr;
  int    retCode = 0;

  glErr = glGetError();
  while (glErr != GL_NO_ERROR)
    {
    //printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
    cout << "Error in GLSLShaderProgram" << endl;
    retCode = 1;
    glErr = glGetError();
    }
  return retCode;
  }
#endif

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkGLSLShaderProgram, "1.9");
vtkStandardNewMacro(vtkGLSLShaderProgram);

//-----------------------------------------------------------------------------
vtkGLSLShaderProgram::vtkGLSLShaderProgram()
  : Program(0),
    Info(NULL)
{

}

//-----------------------------------------------------------------------------
vtkGLSLShaderProgram::~vtkGLSLShaderProgram()
{
}


//-----------------------------------------------------------------------------
vtkShader* vtkGLSLShaderProgram::NewShader()
{
  return vtkGLSLShader::New();
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::ReleaseGraphicsResources(vtkWindow* w)
{
  if (this->IsProgram())
    {
    vtkgl::DeleteProgram(this->Program);
    this->Program = 0;
    }
  this->Superclass::ReleaseGraphicsResources(w);
}

//-----------------------------------------------------------------------------
unsigned int vtkGLSLShaderProgram::GetProgram()
{
  return this->Program;
}
  
//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::Link()
{

}

//-----------------------------------------------------------------------------
int vtkGLSLShaderProgram::IsProgram()
{
  if( this->Program )
    {
    if( vtkgl::IsProgram( static_cast<GLuint>(this->Program) ) == GL_TRUE )
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
int vtkGLSLShaderProgram::IsLinked()
{
  if( this->IsProgram() == false )
    {
    return false;
    }

  GLint value = 0;
  vtkgl::GetProgramiv(static_cast<GLuint>(this->Program),
                      vtkgl::LINK_STATUS, &value);
  if( value == 1 )
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::GetProgramInfo()
{
  if (!this->Program)
    {
    return;
    }

  vtkstd::string infoString;
  // is this a GLSL Program?
  GLint type = 0;
  vtkgl::GetProgramiv( static_cast<GLuint>(this->Program), 
    vtkgl::OBJECT_TYPE_ARB, &type);
  if( type == vtkgl::PROGRAM_OBJECT_ARB )
    {
    infoString += "GLSL Program. \n";
    }
  else
    {
    infoString += "Not a GLSL Program. \n";
    }

  // is this Program linked?
  GLint linked = 0;
  vtkgl::GetProgramiv( static_cast<GLuint>(this->Program), 
                       vtkgl::LINK_STATUS, &linked );
  infoString += "Linked Status: ";
  char linkedStr[256];
  sprintf( linkedStr, "%d", static_cast<int>(linked) );
  infoString += linkedStr;
  infoString += "\n";
  //<< linked << endl;

  // how many objects are attached?
  GLint numObjects = 0;
  vtkgl::GetProgramiv( static_cast<GLuint>(this->Program), 
    vtkgl::OBJECT_ATTACHED_OBJECTS_ARB, &numObjects );
  char numStr[256];
  sprintf( numStr, "%d", static_cast<int>(numObjects) );
  infoString += "Number of attached objects: ";
  infoString += numStr;
  infoString += "\n";


  // Anything in the info log?
  GLint maxLength = 0;
  vtkgl::GetProgramiv( static_cast<GLuint>(this->Program), 
                       vtkgl::ATTACHED_SHADERS, &maxLength );
  vtkgl::GLchar* info = new vtkgl::GLchar[maxLength];

  GLsizei charsWritten;
  vtkgl::GetProgramInfoLog( static_cast<GLuint>(this->Program), maxLength, 
    &charsWritten, info );

  if( info )
    {
    infoString += static_cast<char*>(info);
    infoString += "\n";
    }

  if( infoString.empty() )
    {
    this->SetInfo( "No Program Info." );
    }
  else
    {
    this->SetInfo( infoString.c_str() );
    }
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::GetInfoLog()
{
  // Anything in the info log?
  int infologLength = 0;
  int charsWritten  = 0;
  vtkgl::GLchar *infoLog = NULL;
  vtkgl::GetProgramiv(static_cast<GLuint>(this->Program), vtkgl::INFO_LOG_LENGTH, 
    reinterpret_cast<GLint*>(&infologLength));
  if(infologLength > 0)
    {
    infoLog = new vtkgl::GLchar[infologLength];
    if(infoLog == NULL)
      {
      printf("ERROR: Could not allocate InfoLog buffer\n");
      return;
      }
    vtkgl::GetProgramInfoLog(static_cast<GLuint>(this->Program), infologLength, 
      reinterpret_cast<GLsizei*>(&charsWritten), infoLog);

    this->SetInfo( infoLog );
    delete [] infoLog;
    }
  else
    {
    this->SetInfo( "No Log Info." );
    }
}

//-----------------------------------------------------------------------------
int vtkGLSLShaderProgram::IsAttached(vtkGLSLShader* glslshader)
{
  unsigned int handle = glslshader->GetHandle();
  int attached = 0;
  // find out what's attached
  GLint numObjects = 0;
  GLint writtenObjects = 0;
  vtkgl::GetProgramiv(static_cast<GLuint>(this->Program), 
                      vtkgl::ATTACHED_SHADERS, &numObjects);
  vtkstd::vector<GLuint> attachedObjects(numObjects);
  if( numObjects > 0 )
    {
    vtkgl::GetAttachedShaders(static_cast<GLuint>(this->Program), numObjects, 
      &writtenObjects, &attachedObjects[0]);
    }

  vtkstd::vector<GLuint>::iterator it = attachedObjects.begin();
  vtkstd::vector<GLuint>::iterator itEnd = attachedObjects.end();
  while( it != itEnd )
    {
    if( static_cast<GLuint>(handle) == *it )
      {
      attached = 1;
      }
    it++;
    }
  return attached;
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::Render(vtkActor *actor, vtkRenderer *renderer)
{
  this->LoadExtensions( renderer->GetRenderWindow() );

  // Get a gl identifier for the shader program if we don't already have one.
  if( vtkgl::IsProgram(static_cast<GLuint>(this->Program)) == GL_FALSE )
    {
    this->Program = static_cast<unsigned int>(vtkgl::CreateProgram());
    }
  
  if( vtkgl::IsProgram(static_cast<GLuint>(this->Program)) == GL_FALSE )
    {
    vtkErrorMacro( "Not able to create a GLSL Program!!!" << endl );
    return;
    }
  
  vtkCollectionIterator* iter = this->ShaderCollectionIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkGLSLShader* shader = vtkGLSLShader::SafeDownCast(
      iter->GetCurrentObject());

    if (!shader)
      {
      vtkErrorMacro("GLSL Shader program cannot contain a non-GLSL shader.");
      continue;
      }
    
    if (shader->Compile())
      {
      if (!this->IsAttached(shader))
        {
        vtkgl::AttachShader(static_cast<GLuint>(this->Program), 
          shader->GetHandle());
        }
      }
    }

  if( !this->IsLinked() )
    {
    // if either a vertex or a fragment program is attached (or both)
    // link the program.
    GLint numObjects = 0;
    vtkgl::GetProgramiv( static_cast<GLuint>(this->Program),
                         vtkgl::ATTACHED_SHADERS, &numObjects );
    if (numObjects>0)
      {
      vtkgl::LinkProgram(static_cast<GLuint>(this->Program));
      if (!this->IsLinked())
        {
        this->GetInfoLog();
        vtkErrorMacro(<< "Failed to link GLSL program:\n"
                      << this->Info);
        }
      }
    }

  if( this->IsLinked() )
    {
    // check to see if this is the active program
    vtkgl::UseProgram(static_cast<GLuint>(this->Program));
    }

  // handle attributes and uniform variables
  // uniform variables
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkGLSLShader* shader = vtkGLSLShader::SafeDownCast(
      iter->GetCurrentObject());
    if (!shader)
      {
      // no need to flag error...already marked.
      continue;
      }
    shader->SetProgram(this->Program);
    shader->PassShaderVariables(actor, renderer);
    }

  // establish any textures the shader might use
  vtkProperty *property = actor->GetProperty();
  vtkIdType numTextures = property->GetNumberOfTextures();

  GLint numSupportedTextures;
  glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS, &numSupportedTextures);
  if (numTextures >= numSupportedTextures)
    {
    vtkErrorMacro("Hardware does not support the number of textures defined.");
    }

  for (vtkIdType i = 0; i < numTextures; i++)
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0 + i);
    property->GetTexture(i)->Render(renderer);
    }
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
}
//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::PostRender(vtkActor* actor, vtkRenderer*)
{
  if (this->IsProgram())
    {
    // this unloads the shader program.
    vtkgl::UseProgram(0);
    }

  // Disable any textures that may have been enabled.
  vtkProperty *property = actor->GetProperty();
  vtkIdType numTextures = property->GetNumberOfTextures();
  for (vtkIdType i = 0; i < numTextures; i++)
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0 + i);
    // Disable any possible texture.  Wouldn't having a PostRender on
    // vtkTexture be better?
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(vtkgl::TEXTURE_3D);
    glDisable(vtkgl::TEXTURE_RECTANGLE_ARB);
    glDisable(vtkgl::TEXTURE_CUBE_MAP);
    }
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

