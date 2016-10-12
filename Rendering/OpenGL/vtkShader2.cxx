/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShader2.h"
#include "vtkObjectFactory.h"
#include <cassert>
#include "vtkgl.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLError.h"

static GLenum vtkShaderTypeVTKToGL[5] = {
  vtkgl::VERTEX_SHADER, // VTK_SHADER_TYPE_VERTEX=0
  vtkgl::GEOMETRY_SHADER, // VTK_SHADER_TYPE_GEOMETRY=1,
  vtkgl::FRAGMENT_SHADER, // VTK_SHADER_TYPE_FRAGMENT=2,
  0, // VTK_SHADER_TYPE_TESSELLATION_CONTROL=3, not yet
  0// VTK_SHADER_TYPE_TESSELLATION_EVALUATION=4, not yet
};

static const char *TypeAsStringArray[5] = {
  "vertex shader",
  "geometry shader",
  "fragment shader",
  "tessellation control shader",
  "tessellation evaluation shader"
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkShader2);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkShader2,UniformVariables,vtkUniformVariables);

// ----------------------------------------------------------------------------
// Description:
// Default constructor. SourceCode is NULL. Type is vertex.
vtkShader2::vtkShader2()
{
  // user API
  this->SourceCode = 0;
  this->Type = VTK_SHADER_TYPE_VERTEX;

  // OpenGL part
  this->Context = NULL;
  this->Id = 0;
  this->ExtensionsLoaded = false;
  this->SupportGeometryShader = false;
  this->LastCompileStatus = false;

  // 8 as an initial capcity is nice because the allocation is aligned on
  // 32-bit or 64-bit architecture.
  this->LastCompileLogCapacity = 8;
  this->LastCompileLog = new char[this->LastCompileLogCapacity];
  this->LastCompileLog[0] = '\0'; // empty string

  this->UniformVariables = vtkUniformVariables::New();
}

// ----------------------------------------------------------------------------
void vtkShader2::ReleaseGraphicsResources()
{
  // because we don't hold a reference to the render
  // context we don't have any control on when it is
  // destroyed. In fact it may be destroyed before
  // we are(eg smart pointers), in which case we should
  // do nothing.
  if (this->Context && (this->Id != 0))
  {
    vtkgl::DeleteShader(this->Id);
    vtkOpenGLCheckErrorMacro("failed at glDeleteShader");
    this->Id = 0;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Destructor. Delete SourceCode if any.
vtkShader2::~vtkShader2()
{
  // esplicitly release resources
  this->ReleaseGraphicsResources();

  delete[] this->SourceCode;
  delete[] this->LastCompileLog;

  if (this->UniformVariables)
  {
    this->UniformVariables->Delete();
  }
}

//----------------------------------------------------------------------------
bool vtkShader2::IsSupported(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *context
    = dynamic_cast<vtkOpenGLRenderWindow*>(renWin);

  if (!context) return false;

  vtkOpenGLExtensionManager *e = context->GetExtensionManager();
  return e->ExtensionSupported("GL_VERSION_2_0") ||
    (e->ExtensionSupported("GL_ARB_shading_language_100") &&
     e->ExtensionSupported("GL_ARB_shader_objects") &&
     e->ExtensionSupported("GL_ARB_vertex_shader") &&
     e->ExtensionSupported("GL_ARB_fragment_shader"));
}

//----------------------------------------------------------------------------
bool vtkShader2::LoadRequiredExtensions(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *context
    = dynamic_cast<vtkOpenGLRenderWindow*>(renWin);

  this->ExtensionsLoaded = false;
  this->SupportGeometryShader = false;

  if (!context) return false;

  vtkOpenGLExtensionManager *e = context->GetExtensionManager();


  if (e->ExtensionSupported("GL_VERSION_2_0"))
  {
    e->LoadExtension("GL_VERSION_2_0");
    this->ExtensionsLoaded = true;
  }
  else
  {
    if (e->ExtensionSupported("GL_ARB_shading_language_100") &&
      e->ExtensionSupported("GL_ARB_shader_objects") &&
      e->ExtensionSupported("GL_ARB_vertex_shader") &&
      e->ExtensionSupported("GL_ARB_fragment_shader"))
    {
      e->LoadCorePromotedExtension("GL_ARB_shading_language_100");
      e->LoadCorePromotedExtension("GL_ARB_shader_objects");
      e->LoadCorePromotedExtension("GL_ARB_vertex_shader");
      e->LoadCorePromotedExtension("GL_ARB_fragment_shader");
      this->ExtensionsLoaded = true;
    }
  }

  if (this->ExtensionsLoaded)
  {
    bool supportGeometryShaderARB
      = e->ExtensionSupported("GL_ARB_geometry_shader4") == 1;

    this->SupportGeometryShader
      = supportGeometryShaderARB
      || e->ExtensionSupported("GL_EXT_geometry_shader4") == 1;

    if (this->SupportGeometryShader)
    {
      if (supportGeometryShaderARB)
      {
        e->LoadExtension("GL_ARB_geometry_shader4");
      }
      else
      {
        e->LoadAsARBExtension("GL_EXT_geometry_shader4");
      }
    }
  }

  return this->ExtensionsLoaded;
}

// ----------------------------------------------------------------------------
vtkRenderWindow *vtkShader2::GetContext()
{
  return this->Context;
}

// ----------------------------------------------------------------------------
void vtkShader2::SetContext(vtkRenderWindow *renWin)
{
  // avoid pointless reassignment
  if (this->Context == renWin)
  {
    return;
  }
  // free resources
  this->ReleaseGraphicsResources();
  this->Context = NULL;
  this->Modified();
  // all done if assigned null
  if (!renWin)
  {
    return;
  }
  // check for support
  vtkOpenGLRenderWindow *context
    = dynamic_cast<vtkOpenGLRenderWindow*>(renWin);

  if ( !context
    || !this->LoadRequiredExtensions(renWin) )
  {
    vtkErrorMacro("The context does not support the required extensions");
    return;
  }
  // initialize
  this->Context = renWin;
  this->Context->MakeCurrent();
}

//-----------------------------------------------------------------------------
// Description:
// Compile the shader code.
// The result of compilation can be query with GetLastCompileStatus()
// The log of compilation can be query with GetLastCompileLog()
// \pre SourceCode_exists: SourceCode!=0
void vtkShader2::Compile()
{
  assert("pre: SourceCode_exists" && this->SourceCode != 0);
  vtkOpenGLClearErrorMacro();

  if(this->Id == 0 || this->LastCompileTime < this->MTime)
  {
    if (this->Type == VTK_SHADER_TYPE_TESSELLATION_CONTROL)
    {
      vtkErrorMacro(<<"tessellation control shader is not supported.");
      this->LastCompileStatus = false;
      this->LastCompileLog = 0;
      return;
    }
     if (this->Type == VTK_SHADER_TYPE_TESSELLATION_EVALUATION)
     {
      vtkErrorMacro(<<"tessellation evaluation shader is not supported.");
      this->LastCompileStatus = false;
      this->LastCompileLog = 0;
      return;
     }
    if (this->Type == VTK_SHADER_TYPE_GEOMETRY && !this->SupportGeometryShader)
    {
      vtkErrorMacro(<<"geometry shader is not supported.");
      this->LastCompileStatus = false;
      this->LastCompileLog = 0;
      return;
    }
    GLuint shaderId = static_cast<GLuint>(this->Id);
    if (shaderId == 0)
    {
      shaderId = vtkgl::CreateShader(vtkShaderTypeVTKToGL[this->Type]);
      if (shaderId == 0)
      {
        vtkErrorMacro(<<"fatal error (bad current OpenGL context?, extension not supported?).");
        this->LastCompileStatus = false;
        this->LastCompileLog = 0;
        return;
      }
      this->Id = static_cast<unsigned int>(shaderId);
    }

    vtkgl::ShaderSource(shaderId,1,const_cast<const vtkgl::GLchar**>(&this->SourceCode), 0);
    vtkgl::CompileShader(shaderId);
    GLint value;
    vtkgl::GetShaderiv(shaderId, vtkgl::COMPILE_STATUS, &value);
    this->LastCompileStatus = (value== GL_TRUE);
    vtkgl::GetShaderiv(shaderId, vtkgl::INFO_LOG_LENGTH, &value);
    if (static_cast<size_t>(value) > this->LastCompileLogCapacity)
    {
      delete[] this->LastCompileLog;
      this->LastCompileLogCapacity = static_cast<size_t>(value);
      this->LastCompileLog = new char[this->LastCompileLogCapacity];
    }
    vtkgl::GetShaderInfoLog(shaderId,value, 0, this->LastCompileLog);
    this->LastCompileTime.Modified();
  }

  vtkOpenGLCheckErrorMacro("failed after Compile");
}

//-----------------------------------------------------------------------------
// Description:
// Return the shader type as a string.
const char *vtkShader2::GetTypeAsString()
{
  return TypeAsStringArray[this->Type];
}

//-----------------------------------------------------------------------------
// Description:
// Tells if the last call to compile succeeded (true) or not (false).
bool vtkShader2::GetLastCompileStatus()
{
  return this->LastCompileStatus;
}

//-----------------------------------------------------------------------------
// Description:
// Return the log of the last call to compile as a string.
const char *vtkShader2::GetLastCompileLog()
{
  assert("post: result_exists" && this->LastCompileLog != 0);
  return this->LastCompileLog;
}

//---------------------------------------------------------------------------
void vtkShader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Type: ";
  switch (this->Type)
  {
    case VTK_SHADER_TYPE_VERTEX:
      os << "vertex" << endl;
      break;
    case VTK_SHADER_TYPE_TESSELLATION_CONTROL:
      os << "tessellation control" << endl;
      break;
    case VTK_SHADER_TYPE_TESSELLATION_EVALUATION:
      os << "tessellation evaluation" << endl;
      break;
    case VTK_SHADER_TYPE_GEOMETRY:
      os << "geometry" << endl;
      break;
    case VTK_SHADER_TYPE_FRAGMENT:
      os << "fragment" << endl;
      break;
    default:
      assert("check: impossible_case" && 0); // impossible case
  }

  os << indent << "OpenGL Id: " << this->Id << endl;
  os << indent << "Last Compile Status: ";
  os << ((this->LastCompileStatus) ? "true" : "false") << endl;

  os << indent << "Last Compile Log Capacity: " <<
    this->LastCompileLogCapacity<< endl;

  os << indent << "Last Compile Log: ";
  if (this->LastCompileLog == 0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->LastCompileLog << endl;
  }

  os << indent << "Context: ";
  if (this->Context)
  {
    os << static_cast<void *>(this->Context) <<endl;
  }
  else
  {
    os << "none" << endl;
  }

  os << indent << "UniformVariables: ";
  if (this->UniformVariables)
  {
    this->UniformVariables->PrintSelf(os, indent);
  }
  else
  {
    os << "none" << endl;
  }

  os << indent << "SourceCode: ";
  if (this->SourceCode == 0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << endl << this->SourceCode << endl;
  }
}
