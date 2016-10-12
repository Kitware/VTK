/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShaderProgram.h"
#include "vtkObjectFactory.h"

#include "vtk_glew.h"
#include "vtkShader.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkTransformFeedback.h"
#include "vtkTypeTraits.h"

# include <sstream>

namespace {

inline GLenum convertTypeToGL(int type)
{
  switch (type)
  {
    case VTK_CHAR:
      return GL_BYTE;
    case VTK_UNSIGNED_CHAR:
      return GL_UNSIGNED_BYTE;
    case VTK_SHORT:
      return GL_SHORT;
    case VTK_UNSIGNED_SHORT:
      return GL_UNSIGNED_SHORT;
    case VTK_INT:
      return GL_INT;
    case VTK_UNSIGNED_INT:
      return GL_UNSIGNED_INT;
    case VTK_FLOAT:
      return GL_FLOAT;
    case VTK_DOUBLE:
#ifdef GL_DOUBLE
      return GL_DOUBLE;
#else
      vtkGenericWarningMacro(<< "Attempt to use GL_DOUBLE when not supported");
      return 0;
#endif
    default:
      return 0;
  }
}

} // end anon namespace

typedef std::map<const char *, int, vtkShaderProgram::cmp_str>::iterator IterT;

vtkStandardNewMacro(vtkShaderProgram)

vtkCxxSetObjectMacro(vtkShaderProgram,VertexShader,vtkShader)
vtkCxxSetObjectMacro(vtkShaderProgram,FragmentShader,vtkShader)
vtkCxxSetObjectMacro(vtkShaderProgram,GeometryShader,vtkShader)
vtkCxxSetObjectMacro(vtkShaderProgram, TransformFeedback, vtkTransformFeedback)

vtkShaderProgram::vtkShaderProgram()
{
  this->VertexShader = vtkShader::New();
  this->VertexShader->SetType(vtkShader::Vertex);
  this->FragmentShader = vtkShader::New();
  this->FragmentShader->SetType(vtkShader::Fragment);
  this->GeometryShader = vtkShader::New();
  this->GeometryShader->SetType(vtkShader::Geometry);

  this->TransformFeedback = NULL;

  this->Compiled = false;
  this->NumberOfOutputs = 0;
  this->Handle = 0;
  this->VertexShaderHandle = 0;
  this->FragmentShaderHandle = 0;
  this->GeometryShaderHandle = 0;
  this->Linked = false;
  this->Bound = false;
}

vtkShaderProgram::~vtkShaderProgram()
{
  this->ClearMaps();
  if (this->VertexShader)
  {
    this->VertexShader->Delete();
    this->VertexShader = NULL;
  }
  if (this->FragmentShader)
  {
    this->FragmentShader->Delete();
    this->FragmentShader = NULL;
  }
  if (this->GeometryShader)
  {
    this->GeometryShader->Delete();
    this->GeometryShader = NULL;
  }
  if (this->TransformFeedback)
  {
    this->TransformFeedback->Delete();
    this->TransformFeedback = NULL;
  }
}

// Process the string, and return a version with replacements.
bool vtkShaderProgram::Substitute(std::string &source, const std::string &search,
             const std::string &replace, bool all)
{
  std::string::size_type pos = 0;
  bool replaced = false;
  while ((pos = source.find(search, pos)) != std::string::npos)
  {
    source.replace(pos, search.length(), replace);
    if (!all)
    {
      return true;
    }
    pos += replace.length();
    replaced = true;
  }
  return replaced;
}



template <class T> bool vtkShaderProgram::SetAttributeArray(const char *name,
                                                const T &array, int tupleSize,
                                                NormalizeOption normalize)
{
  if (array.empty())
  {
    this->Error = "Refusing to upload empty array for attribute " + std::string(name) + ".";
    return false;
  }
  int type = vtkTypeTraits<typename T::value_type>::VTKTypeID();
  return this->SetAttributeArrayInternal(name, &array[0], type, tupleSize,
                                         normalize);
}

bool vtkShaderProgram::AttachShader(const vtkShader *shader)
{
  if (shader->GetHandle() == 0)
  {
    this->Error = "Shader object was not initialized, cannot attach it.";
    return false;
  }
  if (shader->GetType() == vtkShader::Unknown)
  {
    this->Error = "Shader object is of type Unknown and cannot be used.";
    return false;
  }

  if (this->Handle == 0)
  {
    GLuint handle_ = glCreateProgram();
    if (handle_ == 0)
    {
      this->Error = "Could not create shader program.";
      return false;
    }
    this->Handle = static_cast<int>(handle_);
    this->Linked = false;
  }

  if (shader->GetType() == vtkShader::Vertex)
  {
    if (this->VertexShaderHandle != 0)
    {
      glDetachShader(static_cast<GLuint>(this->Handle),
                     static_cast<GLuint>(this->VertexShaderHandle));
    }
    this->VertexShaderHandle = shader->GetHandle();
  }
  else if (shader->GetType() == vtkShader::Fragment)
  {
    if (this->FragmentShaderHandle != 0)
    {
      glDetachShader(static_cast<GLuint>(this->Handle),
                     static_cast<GLuint>(this->FragmentShaderHandle));
    }
    this->FragmentShaderHandle = shader->GetHandle();
  }
  else if (shader->GetType() == vtkShader::Geometry)
  {
    if (this->GeometryShaderHandle != 0)
    {
      glDetachShader(static_cast<GLuint>(this->Handle),
                     static_cast<GLuint>(this->GeometryShaderHandle));
    }
// only use GS if supported
#ifdef GL_GEOMETRY_SHADER
    this->GeometryShaderHandle = shader->GetHandle();
#endif
  }
  else
  {
    this->Error = "Unknown shader type encountered - this should not happen.";
    return false;
  }

  glAttachShader(static_cast<GLuint>(this->Handle),
                 static_cast<GLuint>(shader->GetHandle()));
  this->Linked = false;
  return true;
}

bool vtkShaderProgram::DetachShader(const vtkShader *shader)
{
  if (shader->GetHandle() == 0)
  {
    this->Error = "Shader object was not initialized, cannot attach it.";
    return false;
  }
  if (shader->GetType() == vtkShader::Unknown)
  {
    this->Error = "Shader object is of type Unknown and cannot be used.";
    return false;
  }
  if (this->Handle == 0)
  {
    this->Error = "This shader prorgram has not been initialized yet.";
  }

  switch (shader->GetType())
  {
    case vtkShader::Vertex:
      if (this->VertexShaderHandle != shader->GetHandle())
      {
        Error = "The supplied shader was not attached to this program.";
        return false;
      }
      else
      {
        glDetachShader(static_cast<GLuint>(this->Handle),
                       static_cast<GLuint>(shader->GetHandle()));
        this->VertexShaderHandle = 0;
        this->Linked = false;
        return true;
      }
    case vtkShader::Fragment:
      if (this->FragmentShaderHandle != shader->GetHandle())
      {
        this->Error = "The supplied shader was not attached to this program.";
        return false;
      }
      else
      {
        glDetachShader(static_cast<GLuint>(this->Handle),
                       static_cast<GLuint>(shader->GetHandle()));
        this->FragmentShaderHandle = 0;
        this->Linked = false;
        return true;
      }
#ifdef GL_GEOMETRY_SHADER
    case vtkShader::Geometry:
      if (this->GeometryShaderHandle != shader->GetHandle())
      {
        this->Error = "The supplied shader was not attached to this program.";
        return false;
      }
      else
      {
        glDetachShader(static_cast<GLuint>(this->Handle),
                       static_cast<GLuint>(shader->GetHandle()));
        this->GeometryShaderHandle = 0;
        this->Linked = false;
        return true;
      }
#endif
    default:
      return false;
  }
}

void vtkShaderProgram::ClearMaps()
{
  for (IterT i = this->UniformLocs.begin(); i != this->UniformLocs.end(); i++)
  {
    free(const_cast<char *>(i->first));
  }
  this->UniformLocs.clear();
  for (IterT i = this->AttributeLocs.begin(); i != this->AttributeLocs.end(); i++)
  {
    free(const_cast<char *>(i->first));
  }
  this->AttributeLocs.clear();
}

bool vtkShaderProgram::Link()
{
  if (this->Linked)
  {
    return true;
  }

  if (this->Handle == 0)
  {
    this->Error = "Program has not been initialized, and/or does not have shaders.";
    return false;
  }

  // clear out the list of uniforms used
  this->ClearMaps();

#if GL_ES_VERSION_2_0 != 1
  // bind the outputs if specified
  if (this->NumberOfOutputs)
  {
    for (unsigned int i = 0; i < this->NumberOfOutputs; i++)
    {
      // this naming has to match the bindings
      // in vtkOpenGLShaderCache.cxx
      std::ostringstream dst;
      dst << "fragOutput" << i;
      glBindFragDataLocation(static_cast<GLuint>(this->Handle), i,
        dst.str().c_str());
    }
  }
#endif

  GLint isCompiled;
  glLinkProgram(static_cast<GLuint>(this->Handle));
  glGetProgramiv(static_cast<GLuint>(this->Handle), GL_LINK_STATUS, &isCompiled);
  if (isCompiled == 0)
  {
    GLint length(0);
    glGetProgramiv(static_cast<GLuint>(this->Handle), GL_INFO_LOG_LENGTH, &length);
    if (length > 1)
    {
      char *logMessage = new char[length];
      glGetProgramInfoLog(static_cast<GLuint>(this->Handle), length, NULL, logMessage);
      this->Error = logMessage;
      delete[] logMessage;
    }
    return false;
  }
  this->Linked = true;
  return true;
}

bool vtkShaderProgram::Bind()
{
  if (!this->Linked && !this->Link())
  {
    return false;
  }

  glUseProgram(static_cast<GLuint>(this->Handle));
  this->Bound = true;
  return true;
}

// return 0 if there is an issue
int vtkShaderProgram::CompileShader()
{
  if (!this->GetVertexShader()->Compile())
  {
    int lineNum = 1;
    std::istringstream stream(this->GetVertexShader()->GetSource());
    std::stringstream sstm;
    std::string aline;
    while (std::getline(stream, aline))
    {
      sstm << lineNum << ": " << aline << "\n";
      lineNum++;
    }
    vtkErrorMacro(<< sstm.str());
    vtkErrorMacro(<< this->GetVertexShader()->GetError());
    return 0;
  }
  if (!this->GetFragmentShader()->Compile())
  {
    int lineNum = 1;
    std::istringstream stream(this->GetFragmentShader()->GetSource());
    std::stringstream sstm;
    std::string aline;
    while (std::getline(stream, aline))
    {
      sstm << lineNum << ": " << aline << "\n";
      lineNum++;
    }
    vtkErrorMacro(<< sstm.str());
    vtkErrorMacro(<< this->GetFragmentShader()->GetError());
    return 0;
  }
#ifdef GL_GEOMETRY_SHADER
  if (this->GetGeometryShader()->GetSource().size() > 0 &&
      !this->GetGeometryShader()->Compile())
  {
    int lineNum = 1;
    std::istringstream stream(this->GetGeometryShader()->GetSource());
    std::stringstream sstm;
    std::string aline;
    while (std::getline(stream, aline))
    {
      sstm << lineNum << ": " << aline << "\n";
      lineNum++;
    }
    vtkErrorMacro(<< sstm.str());
    vtkErrorMacro(<< this->GetGeometryShader()->GetError());
    return 0;
  }
  if (this->GetGeometryShader()->GetSource().size() > 0 &&
      !this->AttachShader(this->GetGeometryShader()))
  {
    vtkErrorMacro(<< this->GetError());
    return 0;
  }
#endif
  if (!this->AttachShader(this->GetVertexShader()))
  {
    vtkErrorMacro(<< this->GetError());
    return 0;
  }
  if (!this->AttachShader(this->GetFragmentShader()))
  {
    vtkErrorMacro(<< this->GetError());
    return 0;
  }

  // Setup transform feedback:
  if (this->TransformFeedback)
  {
    this->TransformFeedback->BindVaryings(this);
  }

  if (!this->Link())
  {
    vtkErrorMacro(<< "Links failed: " << this->GetError());
    return 0;
  }

  this->Compiled = true;
  return 1;
}

void vtkShaderProgram::Release()
{
  glUseProgram(0);
  this->Bound = false;
}

void vtkShaderProgram::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Release();

  if (this->Compiled)
  {
    this->DetachShader(this->VertexShader);
    this->DetachShader(this->FragmentShader);
    this->DetachShader(this->GeometryShader);
    this->VertexShader->Cleanup();
    this->FragmentShader->Cleanup();
    this->GeometryShader->Cleanup();
    this->Compiled = false;
  }

  vtkOpenGLRenderWindow *renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  if (renWin && renWin->GetShaderCache()->GetLastShaderBound() == this)
  {
    renWin->GetShaderCache()->ClearLastShaderBound();
  }

  if (this->Handle != 0)
  {
    glDeleteProgram(this->Handle);
    this->Handle = 0;
    this->Linked = false;
  }

  if (this->TransformFeedback)
  {
    this->TransformFeedback->ReleaseGraphicsResources();
  }
}

bool vtkShaderProgram::EnableAttributeArray(const char *name)
{
  GLint location = static_cast<GLint>(this->FindAttributeArray(name));
  if (location == -1)
  {
    this->Error = "Could not enable attribute " + std::string(name) + ". No such attribute.";
    return false;
  }
  glEnableVertexAttribArray(location);
  return true;
}

bool vtkShaderProgram::DisableAttributeArray(const char *name)
{
  GLint location = static_cast<GLint>(this->FindAttributeArray(name));
  if (location == -1)
  {
    this->Error = "Could not disable attribute " + std::string(name) + ". No such attribute.";
    return false;
  }
  glDisableVertexAttribArray(location);
  return true;
}

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

bool vtkShaderProgram::UseAttributeArray(const char *name, int offset,
                                      size_t stride, int elementType,
                                      int elementTupleSize,
                                      NormalizeOption normalize)
{
  GLint location = static_cast<GLint>(this->FindAttributeArray(name));
  if (location == -1)
  {
    this->Error = "Could not use attribute (does not exist) ";
    this->Error += name;
    return false;
  }
  glVertexAttribPointer(location, elementTupleSize, convertTypeToGL(elementType),
                        normalize == Normalize ? GL_TRUE : GL_FALSE,
                        static_cast<GLsizei>(stride), BUFFER_OFFSET(offset));
  return true;
}

bool vtkShaderProgram::SetUniformi(const char *name, int i)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform1i(location, static_cast<GLint>(i));
  return true;
}

bool vtkShaderProgram::SetUniformf(const char *name, float f)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform1f(location, static_cast<GLfloat>(f));
  return true;
}

bool vtkShaderProgram::SetUniformMatrix(const char *name,
                                    vtkMatrix4x4 *matrix)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  float data[16];
  for (int i = 0; i < 16; ++i)
  {
    data[i] = matrix->Element[i / 4][i % 4];
  }
  glUniformMatrix4fv(location, 1, GL_FALSE, data);
  return true;
}

bool vtkShaderProgram::SetUniformMatrix3x3(const char *name,
                                           float *matrix)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniformMatrix3fv(location, 1, GL_FALSE, matrix);
  return true;
}

bool vtkShaderProgram::SetUniformMatrix4x4(const char *name,
                                           float *matrix)
{
  return this->SetUniformMatrix4x4v(name,1,matrix);
}

bool vtkShaderProgram::SetUniformMatrix4x4v(
  const char *name,
  const int count,
  float *matrix)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniformMatrix4fv(location, count, GL_FALSE, matrix);
  return true;
}

bool vtkShaderProgram::SetUniformMatrix(const char *name,
                                    vtkMatrix3x3 *matrix)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  float data[9];
  for (int i = 0; i < 9; ++i)
  {
    data[i] = matrix->GetElement(i / 3, i % 3);
  }
  glUniformMatrix3fv(location, 1, GL_FALSE, data);
  return true;
}

bool vtkShaderProgram::SetUniform1fv(const char *name, const int count,
                                    const float *v)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform1fv(location, count, static_cast<const GLfloat *>(v));
  return true;
}

bool vtkShaderProgram::SetUniform1iv(const char *name, const int count,
                                    const int *v)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform1iv(location, count, static_cast<const GLint *>(v));
  return true;
}

bool vtkShaderProgram::SetUniform3fv(const char *name, const int count,
                                    const float (*v)[3])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform3fv(location, count, (const GLfloat *)v);
  return true;
}

bool vtkShaderProgram::SetUniform4fv(const char *name, const int count,
                                    const float (*v)[4])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform4fv(location, count, (const GLfloat *)v);
  return true;
}

bool vtkShaderProgram::SetUniform2f(const char *name, const float v[2])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform2fv(location, 1, v);
  return true;
}

bool vtkShaderProgram::SetUniform2fv(const char *name, const int count,
                                    const float (*f)[2])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform2fv(location, count, (const GLfloat *)f);
  return true;
}

bool vtkShaderProgram::SetUniform3f(const char *name, const float v[3])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform3fv(location, 1, v);
  return true;
}

bool vtkShaderProgram::SetUniform4f(const char *name, const float v[4])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform4fv(location, 1, v);
  return true;
}

bool vtkShaderProgram::SetUniform2i(const char *name, const int v[2])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  glUniform2iv(location, 1, v);
  return true;
}

bool vtkShaderProgram::SetUniform3uc(const char *name,
                                    const unsigned char v[3])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  float colorf[3] = {v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f};
  glUniform3fv(location, 1, colorf);
  return true;
}

bool vtkShaderProgram::SetUniform4uc(const char *name,
                                    const unsigned char v[4])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
  {
    this->Error = "Could not set uniform (does not exist) ";
    this->Error += name;
    return false;
  }
  float colorf[4] = {v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f, v[3] / 255.0f};
  glUniform4fv(location, 1, colorf);
  return true;
}

bool vtkShaderProgram::SetAttributeArrayInternal(
    const char *name, void *buffer, int type, int tupleSize,
    vtkShaderProgram::NormalizeOption normalize)
{
  if (type == -1)
  {
    this->Error = "Unrecognized data type for attribute ";
    this->Error += name;
    return false;
  }
  GLint location = static_cast<GLint>(this->FindAttributeArray(name));
  if (location == -1)
  {
    this->Error = "Could not set attribute (does not exist) ";
    this->Error += name;
    return false;
  }
  const GLvoid *data = static_cast<const GLvoid *>(buffer);
  glVertexAttribPointer(location, tupleSize, convertTypeToGL(type),
                        normalize == Normalize ? GL_TRUE : GL_FALSE, 0, data);
  return true;
}

inline int vtkShaderProgram::FindAttributeArray(const char *cname)
{
  if (cname == NULL || !this->Linked)
  {
    return -1;
  }

  GLint loc = -1;

  IterT iter = this->AttributeLocs.find(cname);
  if (iter == this->AttributeLocs.end())
  {
    loc = glGetAttribLocation(static_cast<GLuint>(Handle),
                              static_cast<const GLchar *>(cname));
    const char *allocStr = strdup(cname);
    this->AttributeLocs.insert(std::make_pair(allocStr, static_cast<int>(loc)));
  }
  else
  {
    loc = iter->second;
  }
  return loc;
}

inline int vtkShaderProgram::FindUniform(const char *cname)
{
  if (cname == NULL || !this->Linked)
  {
    return -1;
  }

  GLint loc = -1;

  IterT iter = this->UniformLocs.find(cname);
  if (iter == this->UniformLocs.end())
  {
    loc = static_cast<int>(glGetUniformLocation(static_cast<GLuint>(Handle),
                                                (const GLchar *)cname));
    const char *allocStr = strdup(cname);
    this->UniformLocs.insert(std::make_pair(allocStr, static_cast<int>(loc)));
  }
  else
  {
    loc = iter->second;
  }
  return loc;
}

bool vtkShaderProgram::IsUniformUsed(const char *cname)
{
  int result = this->FindUniform(cname);

  if (result == -1 && !this->Linked)
  {
    vtkErrorMacro("attempt to find uniform when the shader program is not linked");
  }
  return (result != -1);
}

// ----------------------------------------------------------------------------
bool vtkShaderProgram::IsAttributeUsed(const char *cname)
{
  int result = this->FindAttributeArray(cname);

  if (result == -1 && !this->Linked)
  {
    vtkErrorMacro("attempt to find attribute when the shader program is not linked");
  }
  return (result != -1);
}

// ----------------------------------------------------------------------------
void vtkShaderProgram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
