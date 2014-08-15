/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGLSLShader.h"

#include <iostream>
#include <fstream>

///---------------------------------------------------------------------------
vtkGLSLShader::vtkGLSLShader(void)
{
  this->Program = 0;
  this->TotalShaders = 0;
  this->Shaders[VERTEX_SHADER] = 0;
  this->Shaders[FRAGMENT_SHADER] = 0;
  this->Shaders[GEOMETRY_SHADER] = 0;
  this->AttributeList.clear();
  this->UniformLocationList.clear();
}

///---------------------------------------------------------------------------
///
vtkGLSLShader::~vtkGLSLShader(void)
{
    this->AttributeList.clear();
    this->UniformLocationList.clear();
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::DeleteShaderProgram
///
void vtkGLSLShader::DeleteShaderProgram()
{
    if (this->Program)
      {
      glDeleteProgram(this->Program);
      }
    this->AttributeList.clear();
    this->UniformLocationList.clear();
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::LoadFromString
/// \param type
/// \param source
///
void vtkGLSLShader::LoadFromString(GLenum type, const string& source)
{
  GLuint shader = glCreateShader(type);

  const char * ptmp = source.c_str();
  glShaderSource (shader, 1, &ptmp, NULL);

  //check whether the shader loads fine
  GLint status;
  glCompileShader (shader);
  glGetShaderiv (shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE)
    {
    GLint infoLogLength;
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    GLchar *infoLog= new GLchar[infoLogLength];
    glGetShaderInfoLog (shader, infoLogLength, NULL, infoLog);
    cerr<<"Compile log: "<<infoLog<<endl;
    delete [] infoLog;
    }
  this->Shaders[this->TotalShaders++]=shader;
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::CreateAndLinkProgram
///
void vtkGLSLShader::CreateAndLinkProgram()
{
  this->Program = glCreateProgram ();
  if (this->Shaders[VERTEX_SHADER] != 0)
    {
    glAttachShader (this->Program, this->Shaders[VERTEX_SHADER]);
    }
  if (this->Shaders[FRAGMENT_SHADER] != 0)
    {
    glAttachShader (this->Program, this->Shaders[FRAGMENT_SHADER]);
    }
  if (this->Shaders[GEOMETRY_SHADER] != 0)
    {
    glAttachShader (this->Program, this->Shaders[GEOMETRY_SHADER]);
    }

  //link and check whether the program links fine
  GLint status;
  glLinkProgram (this->Program);
  glGetProgramiv (this->Program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
    {
    GLint infoLogLength;

    glGetProgramiv (this->Program, GL_INFO_LOG_LENGTH, &infoLogLength);
    GLchar *infoLog= new GLchar[infoLogLength];
    glGetProgramInfoLog (this->Program, infoLogLength, NULL, infoLog);
    cerr<<"Link log: "<<infoLog<<endl;
    delete [] infoLog;
    }

  glDeleteShader(this->Shaders[VERTEX_SHADER]);
  glDeleteShader(this->Shaders[FRAGMENT_SHADER]);
  glDeleteShader(this->Shaders[GEOMETRY_SHADER]);
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::Use
///
void vtkGLSLShader::Use()
{
    glUseProgram(this->Program);
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::UnUse
///
void vtkGLSLShader::UnUse()
{
    glUseProgram(0);
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::AddAttribute
/// \param attribute
///
void vtkGLSLShader::AddAttribute(const string& attribute)
{
  this->AttributeList[attribute] = glGetAttribLocation(this->Program, attribute.c_str());
  std::cerr << "this->AttributeList[attribute] " << attribute << std::endl;
  std::cerr << "this->AttributeList[attribute] value " <<  this->AttributeList[attribute] << std::endl;
}

///---------------------------------------------------------------------------
/// An indexer that returns the location of the attribute
////
/// \brief vtkGLSLShader::operator []
/// \param attribute
/// \return
///
GLuint vtkGLSLShader::operator [](const string& attribute)
{
  return this->AttributeList[attribute];
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::AddUniform
/// \param uniform
///
void vtkGLSLShader::AddUniform(const string& uniform)
{
  this->UniformLocationList[uniform] =
    glGetUniformLocation(this->Program, uniform.c_str());
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::operator ()
/// \param uniform
/// \return
///
GLuint vtkGLSLShader::operator()(const string& uniform)
{
  return this->UniformLocationList[uniform];
}

///---------------------------------------------------------------------------
///
/// \brief vtkGLSLShader::LoadFromFile
/// \param whichShader
/// \param filename
///
void vtkGLSLShader::LoadFromFile(GLenum whichShader, const string& filename)
{
  ifstream fp;
  fp.open(filename.c_str(), ios_base::in);
  if(fp)
    {
    string line, buffer;
    while(getline(fp, line))
      {
      buffer.append(line);
      buffer.append("\r\n");
      }
    /// copy to source
    LoadFromString(whichShader, buffer);
  }
  else
    {
    cerr<<"Error loading shader: "<<filename<<endl;
    }
}
