/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderProgram2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShaderProgram2.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkgl.h"

#include <vtkstd/vector>

static void printInfoLogs(GLuint obj, GLuint shader) 
{
  GLint infologLength = 0;
  GLsizei charsWritten  = 0;
  char *infoLog;
  vtkgl::GetProgramiv(obj, vtkgl::INFO_LOG_LENGTH, &infologLength);
  if (infologLength > 1) 
    {
    infoLog = static_cast<char *>(malloc(infologLength));
    vtkgl::GetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
    cout << infoLog << endl;
    free(infoLog);
    }
  if (shader)
    {
    vtkgl::GetShaderiv(shader, vtkgl::INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 1) 
      {
      infoLog = static_cast<char *>(malloc(infologLength));
      vtkgl::GetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
      cout << infoLog << endl;
      free(infoLog);
      }
    }
}

static bool checkErrors(const char *label) 
{
  GLenum errCode;
  //const GLubyte *errStr;
  if ((errCode = glGetError()) != GL_NO_ERROR) 
    {
    //errStr = gluErrorString(errCode);
    printf("%s: OpenGL ERROR ",label);
    //printf((char*)errStr);
    printf("\n");
    return false;
    }
  return true;
}
class vtkShaderProgram2::vtkInternal
{
public:
  GLuint GLSLProgram;
  typedef vtkstd::vector<GLuint> VectorOfGLuint;
  typedef vtkstd::vector<bool> VectorOfBool;

  VectorOfGLuint GLSLShaders;
  VectorOfBool GLSLShaderEnabled;
};

vtkStandardNewMacro(vtkShaderProgram2);
vtkCxxRevisionMacro(vtkShaderProgram2, "1.1");
//----------------------------------------------------------------------------
vtkShaderProgram2::vtkShaderProgram2()
{
  this->Internal = new vtkShaderProgram2::vtkInternal;
  this->Internal->GLSLProgram = 0;
  this->Context = 0;
  this->GeometryShadersSupported = false;
}

//----------------------------------------------------------------------------
vtkShaderProgram2::~vtkShaderProgram2()
{
  // this destroys the shaders if any.
  this->SetContext(0);
  delete this->Internal;
  this->Internal = 0;
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::SetContext(vtkRenderWindow* renWin)
{
  if (this->Context == renWin)
    {
    return;
    }

  this->DestroyShader();
  vtkOpenGLRenderWindow* openGLRenWin = 
    vtkOpenGLRenderWindow::SafeDownCast(renWin);
  this->Context = openGLRenWin;
  if (openGLRenWin)
    {
    if (!this->LoadRequiredExtensions(openGLRenWin->GetExtensionManager()))
      {
      this->Context = 0;
      vtkErrorMacro("Required OpenGL extensions not supported by the context.");
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkShaderProgram2::IsSupported(vtkRenderWindow* win)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  if (renWin)
    {
    vtkOpenGLExtensionManager* mgr = renWin->GetExtensionManager();
    return mgr->ExtensionSupported("GL_VERSION_2_0") != 0;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkShaderProgram2::LoadRequiredExtensions(vtkOpenGLExtensionManager* mgr)
{
  this->GeometryShadersSupported = 
    mgr->LoadSupportedExtension("GL_EXT_geometry_shader4")!=0;
  return mgr->LoadSupportedExtension("GL_VERSION_2_0") != 0;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkShaderProgram2::GetContext()
{
  return this->Context.GetPointer();
}

//----------------------------------------------------------------------------
int vtkShaderProgram2::AddKernel(KernelType type, const char* source)
{
  if (type == GEOMETRY && !this->GeometryShadersSupported)
    {
    vtkErrorMacro("Geometry shaders are not supported "
      "(Missing extensions GL_EXT_geometry_shader4.)");
    return -1;
    }

  if (!this->CreateShaderProgram())
    {
    vtkErrorMacro("Could not create shader program.");
    return -1;
    }
 
  GLenum shaderType;
  switch (type)
    {
  case VERTEX:
    shaderType = vtkgl::VERTEX_SHADER;
    break;

  case GEOMETRY:
    shaderType = vtkgl::GEOMETRY_SHADER_EXT;
    break;

  case FRAGMENT:
    shaderType = vtkgl::FRAGMENT_SHADER;
    break;

  default:
    vtkErrorMacro("Unsupported shader type: " << type);
    return -1;
    }

  GLuint shader = vtkgl::CreateShader(shaderType);

  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");

  vtkgl::ShaderSource(shader, 1, &source, 0);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  vtkgl::CompileShader(shader);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  GLint value = 0;
  vtkgl::GetShaderiv(shader, vtkgl::COMPILE_STATUS, &value);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  if (value != 1)
    {
    vtkErrorMacro("Compilation failed.");
    printInfoLogs(this->Internal->GLSLProgram, shader);
    vtkgl::DeleteShader(shader);
    return -1;
    }
  if (!checkErrors("COMPILE"))
    {
    return -1;
    }
  this->Internal->GLSLShaders.push_back(shader);
  int index = static_cast<int>(this->Internal->GLSLShaders.size()-1);
  this->EnableKernel(index);
  this->Modified();
  return index;
}

//----------------------------------------------------------------------------
unsigned int vtkShaderProgram2::GetOpenGLProgramId()
{
  return static_cast<unsigned int>(this->Internal->GLSLProgram);
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::EnableKernel(int index)
{
  if (index < 0 || index >= static_cast<int>(this->Internal->GLSLShaders.size()))
    {
    vtkErrorMacro("Invalid index: " << index );
    return;
    }

  if (static_cast<int>(this->Internal->GLSLShaderEnabled.size()) <= index)
    {
    this->Internal->GLSLShaderEnabled.resize(index+1, false);
    }

  if (!this->Internal->GLSLShaderEnabled[index])
    {
    GLint shader = this->Internal->GLSLShaders[index];
    vtkgl::AttachShader(this->Internal->GLSLProgram, shader);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
    this->Internal->GLSLShaderEnabled[index] = true;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::DisableKernel(int index)
{
  if (index < 0 || index >= static_cast<int>(this->Internal->GLSLShaders.size()))
    {
    vtkErrorMacro("Invalid index: " << index );
    return;
    }

  if (static_cast<int>(this->Internal->GLSLShaderEnabled.size()) <= index)
    {
    return;
    }

  if (this->Internal->GLSLShaderEnabled[index])
    {
    GLint shader = this->Internal->GLSLShaders[index];
    vtkgl::DetachShader(this->Internal->GLSLProgram, shader);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
    this->Internal->GLSLShaderEnabled[index] = false;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::RemoveAllKernels()
{
  if (!this->Context)
    {
    vtkErrorMacro("Context not specified");
    return;
    }

  this->Context->MakeCurrent();
  if (vtkgl::IsProgram(this->Internal->GLSLProgram) == GL_TRUE)
    {
    this->UnBind();
    this->DeleteShaders();
    }
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkShaderProgram2::Bind()
{
  if (!this->Internal->GLSLProgram)
    {
    return false;
    }

  bool must_link = (this->GetMTime() > this->LinkTime);
  if (!must_link)
    {
    GLint value=0;
    vtkgl::GetProgramiv(this->Internal->GLSLProgram, vtkgl::LINK_STATUS, &value);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
    must_link = (value != 1);
    }
  if (must_link)
    {
    vtkDebugMacro("Linking");
    vtkgl::LinkProgram(this->Internal->GLSLProgram);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");

    GLint status=0;
    vtkgl::GetProgramiv(this->Internal->GLSLProgram, vtkgl::LINK_STATUS, &status);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
    if (status != 1)
      {
      vtkErrorMacro("Link failed");
      printInfoLogs(this->Internal->GLSLProgram, 0);
      return false;
      }
    this->LinkTime.Modified();
    }

  vtkgl::UseProgram(this->Internal->GLSLProgram);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  if (!checkErrors("COMPILE"))
    {
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::UnBind()
{
  if (!this->Internal->GLSLProgram)
    {
    return;
    }
  vtkgl::UseProgram(0);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
}

//----------------------------------------------------------------------------
int vtkShaderProgram2::GetUniformLocation(const char* name)
{
  return vtkgl::GetUniformLocation(this->Internal->GLSLProgram, name);
}

//----------------------------------------------------------------------------
int vtkShaderProgram2::GetAttributeLocation(const char* name)
{
  return vtkgl::GetAttribLocation(this->Internal->GLSLProgram, name);
}

//----------------------------------------------------------------------------
bool vtkShaderProgram2::CreateShaderProgram()
{
  if (this->Internal->GLSLProgram)
    {
    // Already created.
    return true;
    }

  if (!this->Context)
    {
    vtkErrorMacro("Context not specified");
    return false;
    }

  this->Internal->GLSLProgram = vtkgl::CreateProgram();
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  return (this->Internal->GLSLProgram != 0);
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::DestroyShader()
{
  if (this->Context)
    {
    this->Context->MakeCurrent();
    if (this->Internal->GLSLProgram)
      {
      this->UnBind();

      this->DeleteShaders();
      vtkgl::DeleteProgram(this->Internal->GLSLProgram);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
      }
    }
  this->Internal->GLSLProgram = 0;
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::DeleteShaders()
{
  vtkInternal::VectorOfGLuint::iterator iter =
    this->Internal->GLSLShaders.begin();
  int index=0;
  for (; iter != this->Internal->GLSLShaders.end(); ++iter, ++index)
    {
    this->DisableKernel(index);
    vtkgl::DeleteShader(*iter);
    }
  this->Internal->GLSLShaders.clear();
  this->Internal->GLSLShaderEnabled.clear();
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

