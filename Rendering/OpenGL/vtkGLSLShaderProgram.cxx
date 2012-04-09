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
#include "vtkGLSLShaderDeviceAdapter.h"
#include "vtkGLSLShader.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkXMLDataElement.h"
#include "vtkWindow.h"

#include <vector>
#include <string>
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
vtkStandardNewMacro(vtkGLSLShaderProgram);

//-----------------------------------------------------------------------------
vtkGLSLShaderProgram::vtkGLSLShaderProgram()
  : Program(0),
    Info(NULL)
{
  vtkGLSLShaderDeviceAdapter* adapter = vtkGLSLShaderDeviceAdapter::New();
  this->SetShaderDeviceAdapter(adapter);
  adapter->Delete();
}

//-----------------------------------------------------------------------------
vtkGLSLShaderProgram::~vtkGLSLShaderProgram()
{
  this->SetShaderDeviceAdapter(0);
}


//-----------------------------------------------------------------------------
vtkShader* vtkGLSLShaderProgram::NewShader()
{
  return vtkGLSLShader::New();
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::ReleaseGraphicsResources(vtkWindow* w)
{
  if (w && w->GetMapped() && this->IsProgram())
    {
    vtkgl::DeleteProgram(this->Program);
    }
  this->Program = 0;
  this->Superclass::ReleaseGraphicsResources(w);
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::Link()
{
}

//-----------------------------------------------------------------------------
int vtkGLSLShaderProgram::IsProgram()
{
  return this->Program &&
    vtkgl::IsProgram( static_cast<GLuint>(this->Program) ) == GL_TRUE;
}

//-----------------------------------------------------------------------------
int vtkGLSLShaderProgram::IsLinked()
{
  if(!this->IsProgram())
    {
    return false;
    }
  GLint value = 0;
  vtkgl::GetProgramiv(static_cast<GLuint>(this->Program),
                      vtkgl::LINK_STATUS, &value);
  return value==1;
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::GetProgramInfo()
{
  if (!this->Program)
    {
    return;
    }

  std::string infoString;
  if(this->IsProgram())
    {
    infoString += "GLSL Program. \n";
    }
  else
    {
    this->SetInfo("Not a GLSL Program. \n");
    return;
    }

  // is this Program linked?
  infoString += "Linked Status: ";
  char linkedStr[256];
  sprintf( linkedStr, "%d", this->IsLinked() );
  infoString += linkedStr;
  infoString += "\n";

  // how many objects are attached?
  GLint numObjects = 0;
  vtkgl::GetProgramiv(static_cast<GLuint>(this->Program),
                      vtkgl::ATTACHED_SHADERS, &numObjects);

  char numStr[256];
  sprintf( numStr, "%d", static_cast<int>(numObjects) );
  infoString += "Number of attached objects: ";
  infoString += numStr;
  infoString += "\n";


  // Anything in the info log?
  GLint maxLength = 0;
  vtkgl::GetProgramiv(static_cast<GLuint>(this->Program),
                      vtkgl::INFO_LOG_LENGTH, &maxLength);

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

  vtkgl::GetProgramiv(static_cast<GLuint>(this->Program),
                      vtkgl::INFO_LOG_LENGTH,
                      reinterpret_cast<GLint*>(&infologLength));

  if(infologLength > 0)
    {
    infoLog = new vtkgl::GLchar[infologLength];
    if(infoLog == NULL)
      {
      printf("ERROR: Could not allocate InfoLog buffer\n");
      return;
      }
    vtkgl::GetProgramInfoLog(static_cast<GLuint>(this->Program),
                             infologLength,
                             reinterpret_cast<GLsizei*>(&charsWritten),
                             infoLog);
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

  std::vector<GLuint> attachedObjects(numObjects);
  if( numObjects > 0 )
    {
    vtkgl::GetAttachedShaders(static_cast<GLuint>(this->Program), numObjects,
                              &writtenObjects, &attachedObjects[0]);
    }

  std::vector<GLuint>::iterator it = attachedObjects.begin();
  std::vector<GLuint>::iterator itEnd = attachedObjects.end();
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
void vtkGLSLShaderProgram::LoadExtensions( vtkRenderWindow* renWin )
{
  if(this->GetGLExtensionsLoaded())
    {
    return;
    }

  // Load extensions using vtkOpenGLExtensionManager
  vtkOpenGLExtensionManager *extensions = vtkOpenGLExtensionManager::New();
  // How can I get access to the vtkRenderWindow from here?
  extensions->SetRenderWindow( renWin );
  if(extensions->ExtensionSupported("GL_VERSION_2_0")
     && extensions->ExtensionSupported("GL_VERSION_1_3") )
    {
    extensions->LoadExtension("GL_VERSION_2_0");
    extensions->LoadExtension("GL_VERSION_1_3");
    this->SetGLExtensionsLoaded(1);
    }
  else if (extensions->ExtensionSupported("GL_VERSION_1_3")
           && extensions->ExtensionSupported("GL_ARB_shading_language_100")
           && extensions->ExtensionSupported("GL_ARB_shader_objects")
           && extensions->ExtensionSupported("GL_ARB_vertex_shader")
           && extensions->ExtensionSupported("GL_ARB_fragment_shader") )
    {
    // Support older drivers that implement GLSL but not all of OpenGL 2.0.
    extensions->LoadExtension("GL_VERSION_1_3");
    extensions->LoadCorePromotedExtension("GL_ARB_shading_language_100");
    extensions->LoadCorePromotedExtension("GL_ARB_shader_objects");
    extensions->LoadCorePromotedExtension("GL_ARB_vertex_shader");
    extensions->LoadCorePromotedExtension("GL_ARB_fragment_shader");
    this->SetGLExtensionsLoaded(1);
    }
  else
    {
    vtkErrorMacro( "Required extension (GL_VERSION_2_0) is not supported." )
    this->SetGLExtensionsLoaded(0);
    }
  extensions->Delete();
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::Render(vtkActor *actor, vtkRenderer *renderer)
{
  this->LoadExtensions( renderer->GetRenderWindow() );
  if (!this->GetGLExtensionsLoaded())
    {
    return;
    }

  // Get a gl identifier for the shader program if we don't already have one.
  if(!this->IsProgram())
    {
    this->Program = static_cast<unsigned int>(vtkgl::CreateProgram());
    }

  if(!this->IsProgram())
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
    vtkgl::GetProgramiv(static_cast<GLuint>(this->Program),
                        vtkgl::ATTACHED_SHADERS, &numObjects);
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
}
//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::PostRender(vtkActor*, vtkRenderer*)
{
  if (!this->GetGLExtensionsLoaded())
    {
    return;
    }

  if (this->IsProgram())
    {
    // this unloads the shader program.
    vtkgl::UseProgram(0);
    }
}

//-----------------------------------------------------------------------------
void vtkGLSLShaderProgram::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

