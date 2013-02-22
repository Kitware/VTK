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
#include "vtkShader2Collection.h"
#include "vtkShader2.h"
#include "vtkUniformVariables.h"

#include "vtkgl.h"

#include <vector>
#include <vtksys/ios/sstream>
#include <assert.h>
#include "vtkStdString.h"

GLenum vtkGeometryTypeInVTKToGL[5] = {
  GL_POINTS, // VTK_GEOMETRY_SHADER_IN_TYPE_POINTS=0
  GL_LINES, // VTK_GEOMETRY_SHADER_IN_TYPE_LINES=1,
  vtkgl::LINES_ADJACENCY_ARB, // VTK_GEOMETRY_SHADER_IN_TYPE_LINES_ADJACENCY=2
  GL_TRIANGLES, // VTK_GEOMETRY_SHADER_IN_TYPE_TRIANGLES=3
  vtkgl::TRIANGLES_ADJACENCY_ARB // VTK_GEOMETRY_SHADER_IN_TYPE_TRIANGLES_ADJACENCY=4
};

const char *vtkGeometryTypeInAsStringArray[5] = {
  "points",
  "lines",
  "lines with adjacency",
  "triangles",
  "triangles with adjacency"
};

GLenum vtkGeometryTypeOutVTKToGL[3] = {
  GL_POINTS, // VTK_GEOMETRY_SHADER_OUT_TYPE_POINTS=0
  GL_LINE_STRIP, // VTK_GEOMETRY_SHADER_OUT_TYPE_LINE_STRIP=1,
  GL_TRIANGLE_STRIP, // VTK_GEOMETRY_SHADER_OUT_TYPE_TRIANGLE_STRIP=2
};

const char *vtkGeometryTypeOutAsStringArray[3] = {
  "points",
  "line strip",
  "triangle strip"
};

vtkStandardNewMacro(vtkShaderProgram2);
vtkCxxSetObjectMacro(vtkShaderProgram2, UniformVariables, vtkUniformVariables);

//----------------------------------------------------------------------------
vtkShaderProgram2::vtkShaderProgram2()
{
  this->Context = 0;
  this->ExtensionsLoaded = false;

  this->Id = 0;
  this->SavedId = 0;
  this->Shaders = vtkShader2Collection::New(); // an empty list.

  this->LastBuildStatus = VTK_SHADER_PROGRAM2_COMPILE_FAILED;

  this->GeometryTypeIn = VTK_GEOMETRY_SHADER_IN_TYPE_POINTS;
  this->GeometryTypeOut = VTK_GEOMETRY_SHADER_OUT_TYPE_POINTS;
  this->GeometryVerticesOut = 1;

  // 8 as an initial capacity is nice because the allocation is aligned on
  // 32-bit or 64-bit architecture.

  this->LastLinkLogCapacity = 8;
  this->LastLinkLog = new char[this->LastLinkLogCapacity];
  this->LastLinkLog[0] = '\0'; // empty string

  this->LastValidateLogCapacity = 8;
  this->LastValidateLog = new char[this->LastValidateLogCapacity];
  this->LastValidateLog[0] = '\0'; // empty string

//  this->Context = 0;
//  this->GeometryShadersSupported = false;

  this->UniformVariables = vtkUniformVariables::New(); // empty list
  this->PrintErrors = true;
}

//-----------------------------------------------------------------------------
void vtkShaderProgram2::ReleaseGraphicsResources()
{
  if (this->Context)
    {
    if (this->Id != 0)
      {
      vtkgl::DeleteProgram(this->Id);
      this->Id = 0;
      }
    this->LastBuildStatus = VTK_SHADER_PROGRAM2_COMPILE_FAILED;
    this->Shaders->ReleaseGraphicsResources();
    }
  else
    {
    if (this->Id != 0)
      {
      vtkErrorMacro(<<" no context but some OpenGL resource has not been deleted.");
      }
    }
}

//----------------------------------------------------------------------------
vtkShaderProgram2::~vtkShaderProgram2()
{
  delete[] this->LastLinkLog;
  delete[] this->LastValidateLog;

  if (this->UniformVariables)
    {
    this->UniformVariables->Delete();
    this->UniformVariables = 0;
    }
  if (this->Id != 0)
    {
    vtkErrorMacro(<<"a vtkShaderProgram2 object is being deleted before ReleaseGraphicsResources() has been called.");
    }
  if (this->Shaders)
    {
    this->Shaders->Delete();
    this->Shaders = 0;
    }
}

//----------------------------------------------------------------------------
bool vtkShaderProgram2::IsSupported(vtkOpenGLRenderWindow *context)
{
  assert("pre: context_exists" && context);

  vtkOpenGLExtensionManager *e = context->GetExtensionManager();

  bool multiTexture = e->ExtensionSupported("GL_VERSION_1_3") ||
    e->ExtensionSupported("GL_ARB_multitexture");

  bool glsl = e->ExtensionSupported("GL_VERSION_2_0") ||
    (e->ExtensionSupported("GL_ARB_shading_language_100") &&
     e->ExtensionSupported("GL_ARB_shader_objects") &&
     e->ExtensionSupported("GL_ARB_vertex_shader") &&
     e->ExtensionSupported("GL_ARB_fragment_shader"));

  return multiTexture && glsl;
}

//----------------------------------------------------------------------------
bool vtkShaderProgram2::LoadExtensions(vtkOpenGLRenderWindow *context)
{
  assert("pre: context_exists" && context);

  vtkOpenGLExtensionManager *e = context->GetExtensionManager();

  bool gl13 = e->ExtensionSupported("GL_VERSION_1_3") == 1;
  bool gl20 = e->ExtensionSupported("GL_VERSION_2_0") == 1;

  bool multiTexture = gl13 || e->ExtensionSupported("GL_ARB_multitexture");
  bool glsl = gl20 || (e->ExtensionSupported("GL_ARB_shading_language_100") &&
    e->ExtensionSupported("GL_ARB_shader_objects") &&
    e->ExtensionSupported("GL_ARB_vertex_shader") &&
    e->ExtensionSupported("GL_ARB_fragment_shader"));

  bool result = (multiTexture && glsl);

  if (result)
    {
    if (gl13)
      {
      e->LoadExtension("GL_VERSION_1_3");
      }
    else
      {
      e->LoadCorePromotedExtension("GL_ARB_multitexture");
      }
    if (gl20)
      {
      e->LoadExtension("GL_VERSION_2_0");
      }
    else
      {
      e->LoadCorePromotedExtension("GL_ARB_shading_language_100");
      e->LoadCorePromotedExtension("GL_ARB_shader_objects");
      e->LoadCorePromotedExtension("GL_ARB_vertex_shader");
      e->LoadCorePromotedExtension("GL_ARB_fragment_shader");
      }
    }

  return result;
}

// ----------------------------------------------------------------------------
void vtkShaderProgram2::SetContext(vtkOpenGLRenderWindow *context)
{
  if (this->Context != context)
    {
    this->ReleaseGraphicsResources();
    this->Context = context;
    if (this->Context)
      {
      this->ExtensionsLoaded = this->LoadExtensions(this->Context);
      }
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a vertex shader.
// If yes, it means the vertex processing of the fixed-pipeline is bypassed.
// If no, it means the vertex processing of the fixed-pipeline is used.
bool vtkShaderProgram2::HasVertexShaders()
{
  return (this->Shaders) ? this->Shaders->HasVertexShaders() : false;
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a tessellation control shader.
bool vtkShaderProgram2::HasTessellationControlShaders()
{
  return (this->Shaders) ? this->Shaders->HasTessellationControlShaders() : false;
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a tessellation evaluation shader.
bool vtkShaderProgram2::HasTessellationEvaluationShaders()
{
  return (this->Shaders) ? this->Shaders->HasTessellationEvaluationShaders() : false;
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a geometry shader.
bool vtkShaderProgram2::HasGeometryShaders()
{
  return (this->Shaders) ? this->Shaders->HasGeometryShaders() : false;
}

// ----------------------------------------------------------------------------
// Description:
// Tells if at least one of the shaders is a fragment shader.
// If yes, it means the fragment processing of the fixed-pipeline is
// bypassed.
// If no, it means the fragment processing of the fixed-pipeline is used.
bool vtkShaderProgram2::HasFragmentShaders()
{
  return (this->Shaders) ? this->Shaders->HasFragmentShaders() : false;
}

// ----------------------------------------------------------------------------
// Description:
// Tell if the program is the one currently used by OpenGL.
bool vtkShaderProgram2::IsUsed()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());

  GLint value;
  glGetIntegerv(vtkgl::CURRENT_PROGRAM,&value);
  return static_cast<GLuint>(value) == static_cast<GLuint>(this->Id);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if a display list is under construction with GL_COMPILE mode.
// Return false if there is no display list under construction of if the
// mode is GL_COMPILE_AND_EXECUTE.
// Used internally and provided as a public method for whoever find it
// useful.
bool vtkShaderProgram2::DisplayListUnderCreationInCompileMode()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());

  bool result = false;
  GLint value;
  glGetIntegerv(GL_LIST_INDEX, &value);
  if (value != 0) // some display list is under construction.
    {
    glGetIntegerv(GL_LIST_MODE, &value);
    if (value == GL_COMPILE)
      {
      result = true;
      }
    else
      {
      if (value != GL_COMPILE_AND_EXECUTE)
        {
        vtksys_ios::ostringstream ost;
        ost << "Unexpected display list creation mode:" << hex << value << dec;
        vtkErrorMacro(<< ost.str().c_str());
        }
      }
    }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Use the shader program.
// It saves the current shader program or fixed-pipeline in use.
// It also set the uniform variables.
void vtkShaderProgram2::Use()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());
  this->Build();

  // We need to know if this call happens in a display list or not because
  // glGetIntegerv(vtkgl::CURRENT_PROGRAM,&value) is executed immediately
  // while vtkgl::UseProgram(id) is just compiled and its execution is
  // postpone in GL_COMPILE mode.

  if (this->LastBuildStatus == VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    GLuint progId = static_cast<GLuint>(this->Id);
    if (this->DisplayListUnderCreationInCompileMode())
      {
      // don't look at current program, don't save it, don't restore it
      // later.
      vtkgl::UseProgram(progId);
      }
    else
      {
      GLint value;
      glGetIntegerv(vtkgl::CURRENT_PROGRAM, &value);
      if (static_cast<GLuint>(value) != progId)
        {
        this->SavedId = static_cast<unsigned int>(value);
        if (this->SavedId != 0)
          {
          vtkWarningMacro(<<"another program was used (id=" << this->SavedId
                          <<"), our id is" << progId << ".");
          }
        vtkgl::UseProgram(progId);
        }
      assert("check: in_use" && this->IsUsed());
      }
    this->SendUniforms();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Restore the previous shader program (or fixed-pipeline).
void vtkShaderProgram2::Restore()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());

  if (this->DisplayListUnderCreationInCompileMode())
    {
     vtkgl::UseProgram(0);
     this->SavedId = 0;
    }
  else
    {
    GLint value;
    glGetIntegerv(vtkgl::CURRENT_PROGRAM, &value);
    if (static_cast<GLuint>(value) == static_cast<GLuint>(this->Id))
      {
      vtkgl::UseProgram(static_cast<GLuint>(this->SavedId));
      this->SavedId = 0;
      }
    else
      {
      vtkWarningMacro(<<"cannot restore because the program in use (id="
                      << value <<
                      ") is not the id of the vtkShaderProgram2 object (id="
                      << this->Id << ").");
      }
    }
}

// ----------------------------------------------------------------------------
// Description:
// Force the current shader program to be the fixed-pipeline.
// Warning: this call will be compiled if called inside a display list
// creation.
void vtkShaderProgram2::RestoreFixedPipeline()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());

  vtkgl::UseProgram(0);
  this->SavedId = 0;
}

// ----------------------------------------------------------------------------
void vtkShaderProgram2::Build()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());

  if (this->Id == 0 || this->LastLinkTime<this->MTime ||
     (this->Shaders && this->LastLinkTime<this->Shaders->GetMTime()))
    {
    this->LastBuildStatus=VTK_SHADER_PROGRAM2_COMPILE_FAILED;
    GLuint progId = static_cast<GLuint>(this->Id);
    if (progId == 0)
      {
      progId = vtkgl::CreateProgram();
      if (progId == 0)
        {
        vtkErrorMacro(<<"fatal error (bad current OpenGL context?, extension not supported?).");
        return;
        }
      this->Id = static_cast<unsigned int>(progId);
      }
    // Detach all previous shaders (some may have disappeared
    // from this->Shaders)
    GLint numberOfAttachedShaders;
    vtkgl::GetProgramiv(progId, vtkgl::ATTACHED_SHADERS,
      &numberOfAttachedShaders);
    if (numberOfAttachedShaders > 0)
      {
      GLuint *attachedShaders = new GLuint[numberOfAttachedShaders];
      vtkgl::GetAttachedShaders(progId,numberOfAttachedShaders,0,
                                attachedShaders);
      int i = 0;
      while(i < numberOfAttachedShaders)
        {
        vtkgl::DetachShader(progId, attachedShaders[i]);
        ++i;
        }
      delete[] attachedShaders;
      }

    // We compile all the shaders, even if one fails so that
    // we can get info logs for all shaders.
    bool compileDone = true;
    this->Shaders->InitTraversal();
    vtkShader2 *s = this->Shaders->GetNextShader();
    while (s)
      {
      // Propagate the context to the shader.
      s->SetContext(this->Context);
      s->Compile();
      if (s->GetLastCompileStatus())
        {
        vtkgl::AttachShader(progId, static_cast<GLuint>(s->GetId()));
        }
      else
        {
        compileDone = false;
        if (this->PrintErrors)
          {
          vtkErrorMacro(<<" a shader failed to compile. Its log is:\n" << s->GetLastCompileLog() << "\n. Its source code is:\n" << s->GetSourceCode());
          }
        }
      s = this->Shaders->GetNextShader();
      }

    if (compileDone)
      {
      this->LastBuildStatus = VTK_SHADER_PROGRAM2_LINK_FAILED;

      // It is required to pass geometry shader parameters before linking.
      if (this->HasGeometryShaders())
        {
        vtkgl::ProgramParameteriARB(progId, vtkgl::GEOMETRY_INPUT_TYPE_ARB,
                                    static_cast<GLint>(vtkGeometryTypeInVTKToGL[this->GeometryTypeIn]));
        vtkgl::ProgramParameteriARB(progId, vtkgl::GEOMETRY_OUTPUT_TYPE_ARB,
                                    static_cast<GLint>(vtkGeometryTypeOutVTKToGL[this->GeometryTypeOut]));
        vtkgl::ProgramParameteriARB(progId, vtkgl::GEOMETRY_VERTICES_OUT_ARB,
                                    this->GeometryVerticesOut);
        }

      vtkgl::LinkProgram(progId);
      GLint value;
      vtkgl::GetProgramiv(progId, vtkgl::LINK_STATUS ,&value);
      if (value == GL_TRUE)
        {
        this->LastBuildStatus = VTK_SHADER_PROGRAM2_LINK_SUCCEEDED;
        }

      vtkgl::GetProgramiv(progId, vtkgl::INFO_LOG_LENGTH, &value);
      if (static_cast<size_t>(value)>this->LastLinkLogCapacity)
        {
        delete[] this->LastLinkLog;
        this->LastLinkLogCapacity = static_cast<size_t>(value);
        this->LastLinkLog = new char[this->LastLinkLogCapacity];
        }
      vtkgl::GetProgramInfoLog(progId, value, 0, this->LastLinkLog);

      if (this->LastBuildStatus == VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
        {
        this->LastLinkTime.Modified();
        }
      else
        {
         if (this->PrintErrors)
          {
          vtkErrorMacro(<<" the shader program failed to link. Its log is:\n"
                        << this->GetLastLinkLog() << "the shaders are: ");
          size_t i = 0;
          size_t c = static_cast<size_t>(this->Shaders->GetNumberOfItems());
          this->Shaders->InitTraversal();
          s = this->Shaders->GetNextShader();
          while (s)
            {
            vtkErrorMacro(<<"shader #" << i << "/" << c <<
              " (" <<s->GetTypeAsString()
              <<") log is:\n" << s->GetLastCompileLog()
              << "\n. Its source code is:\n" << s->GetSourceCode());
            s = this->Shaders->GetNextShader();
            ++i;
            }

          }
        }
      }
    }
}

// ----------------------------------------------------------------------------
void vtkShaderProgram2::SendUniforms()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());
  assert("pre: built" &&
         this->GetLastBuildStatus() == VTK_SHADER_PROGRAM2_LINK_SUCCEEDED);

  bool needUpdate = this->LastSendUniformsTime<this->LastLinkTime;
  if (!needUpdate)
    {
    needUpdate=this->UniformVariables!=0 &&
      this->LastSendUniformsTime<this->UniformVariables->GetMTime();
    }

  this->Shaders->InitTraversal();
  vtkShader2 *s = this->Shaders->GetNextShader();
  vtkUniformVariables *list;
  while (!needUpdate && s)
    {
    list = s->GetUniformVariables();
    needUpdate = (list!=0 && this->LastSendUniformsTime<list->GetMTime());
    s = this->Shaders->GetNextShader();
    }

  if (needUpdate)
    {
    bool inListCreation = this->DisplayListUnderCreationInCompileMode();
    bool isUsed = false;
    if (!inListCreation)
      {
      isUsed = this->IsUsed();
      if (!isUsed)
        {
        this->Use();
        }
      }

    GLuint progId = static_cast<GLuint>(this->Id);

    this->Shaders->InitTraversal();
    s = this->Shaders->GetNextShader();
    const char *name;
    GLint uniformId;
    while (s)
      {
      list = s->GetUniformVariables();
      list->Start();
      while(!list->IsAtEnd())
        {
        name = list->GetCurrentName();
        uniformId = vtkgl::GetUniformLocation(progId,name);
        if (uniformId != -1)
          {
          // -1 means is not an active uniform
          // it does not mean it is an error.
          list->SendCurrentUniform(uniformId);
          }
        list->Next();
        }
      s = this->Shaders->GetNextShader();
      }

    // override some of the values of the uniform variables set at the shader
    // level with the uniform values set at the program level.
    list = this->GetUniformVariables();
    list->Start();
    while (!list->IsAtEnd())
      {
      name = list->GetCurrentName();
      uniformId = vtkgl::GetUniformLocation(progId,name);
      if (uniformId != -1)
        {
        // -1 means is not an active uniform
        // it does not mean it is an error.
        list->SendCurrentUniform(uniformId);
        }
      list->Next();
      }

    if (!inListCreation && !isUsed)
      {
      this->Restore();
      }
    }
}

// ----------------------------------------------------------------------------
// Description:
// Introspection. Return the list of active uniform variables of the program.
void vtkShaderProgram2::PrintActiveUniformVariables(
  ostream &os,
  vtkIndent indent)
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());
  assert("pre: built" &&
         this->GetLastBuildStatus() == VTK_SHADER_PROGRAM2_LINK_SUCCEEDED);

  GLint params;
  GLuint progId = static_cast<GLuint>(this->Id);

  // info about the list of active uniform variables
  vtkgl::GetProgramiv(progId, vtkgl::ACTIVE_UNIFORMS, &params);
  os<< indent << "There are " << params << " active uniform variables."<<endl;
  GLuint i = 0;
  GLuint c = static_cast<GLuint>(params);
  vtkgl::GetProgramiv(progId, vtkgl::ACTIVE_UNIFORM_MAX_LENGTH, &params);

  GLint buffSize = params;
  char *name = new char[buffSize + 1];
  GLint size;
  GLenum type;
  bool isInt;
  int elementSize;
  bool isSampler;
  GLenum textureBinding;
  while (i < c)
    {
    vtkgl::GetActiveUniform(progId, i, buffSize, 0, &size, &type, name);
    os << indent << i <<" ";
    os << indent;
    isInt = true;
    isSampler = false;
    textureBinding = GL_TEXTURE_BINDING_1D;
    elementSize = 1;
    switch(type)
      {
      case GL_FLOAT:
        os << "float";
        isInt = false;
        break;
      case vtkgl::FLOAT_VEC2:
        os << "vec2";
        isInt = false;
        elementSize = 2;
        break;
      case vtkgl::FLOAT_VEC3:
        os << "vec3";
        isInt = false;
        elementSize = 3;
        break;
      case vtkgl::FLOAT_VEC4:
        os << "vec4";
        isInt = false;
        elementSize = 4;
        break;
      case GL_INT:
        os << "int";
        break;
      case vtkgl::INT_VEC2:
        os << "ivec2";
        elementSize = 2;
        break;
      case vtkgl::INT_VEC3:
        os << "ivec3";
        elementSize = 3;
        break;
      case vtkgl::INT_VEC4:
        os << "ivec4";
        elementSize = 4;
        break;
      case vtkgl::BOOL:
        os << "bool";
        break;
      case vtkgl::BOOL_VEC2:
        os << "bvec2";
        elementSize = 2;
        break;
      case vtkgl::BOOL_VEC3:
        os << "bvec3";
        elementSize = 3;
        break;
      case vtkgl::BOOL_VEC4:
        os << "bvec4";
        elementSize = 4;
        break;
      case vtkgl::FLOAT_MAT2:
        os << "mat2";
        isInt = false;
        elementSize = 4;
        break;
      case vtkgl::FLOAT_MAT3:
        os << "mat3";
        isInt = false;
        elementSize = 9;
        break;
      case vtkgl::FLOAT_MAT4:
        os << "mat4";
        isInt = false;
        elementSize = 16;
        break;
      case vtkgl::FLOAT_MAT2x3:
        os << "mat2x3";
        isInt = false;
        elementSize = 6;
        break;
      case vtkgl::FLOAT_MAT2x4:
        os << "mat2x4";
        isInt = false;
        elementSize = 8;
        break;
      case vtkgl::FLOAT_MAT3x2:
        os << "mat3x2";
        isInt = false;
        elementSize = 6;
        break;
      case vtkgl::FLOAT_MAT3x4:
        os << "mat3x4";
        isInt = false;
        elementSize = 12;
        break;
      case vtkgl::FLOAT_MAT4x2:
        os << "mat4x2";
        isInt = false;
        elementSize = 8;
        break;
      case vtkgl::FLOAT_MAT4x3:
        os << "mat4x3";
        isInt = false;
        elementSize = 12;
        break;
      case vtkgl::SAMPLER_1D:
        os << "sampler1D";
        isSampler = true;
        textureBinding = GL_TEXTURE_BINDING_1D;
        break;
      case vtkgl::SAMPLER_2D:
        os << "sampler2D";
        isSampler = true;
        textureBinding = GL_TEXTURE_BINDING_2D;
        break;
      case vtkgl::SAMPLER_3D:
        os << "sampler3D";
        isSampler = true;
        textureBinding = vtkgl::TEXTURE_BINDING_3D;
        break;
      case vtkgl::SAMPLER_CUBE:
        os<<"samplerCube";
        isSampler = true;
        textureBinding = vtkgl::TEXTURE_BINDING_CUBE_MAP;
        break;
      case vtkgl::SAMPLER_1D_SHADOW:
        os << "sampler1DShadow";
        isSampler = true;
        textureBinding = GL_TEXTURE_BINDING_1D;
        break;
      case vtkgl::SAMPLER_2D_SHADOW:
        os << "sampler2DShadow";
        isSampler = true;
        textureBinding = GL_TEXTURE_BINDING_2D;
        break;
      case vtkgl::SAMPLER_2D_RECT_ARB:
        os << "sampler2DRect";
        isSampler = true;
        textureBinding = vtkgl::TEXTURE_BINDING_RECTANGLE_ARB;
        break;
      case vtkgl::SAMPLER_2D_RECT_SHADOW_ARB:
        os << "sampler2DRectShadow";
        isSampler = true;
        textureBinding = vtkgl::TEXTURE_BINDING_RECTANGLE_ARB;
        break;
      }
    os << " " << name;
    if (size>1)
      {
      os << "[" << size << "]";
      }
    os << "=";
    if (size > 1)
      {
      os << "{";
      }
    GLint *ivalues = 0;
    GLfloat *fvalues = 0;

    if (isInt)
      {
      ivalues = new GLint[elementSize];
      }
    else
      {
      fvalues = new GLfloat[elementSize];
      }

    int element = 0;
    while (element<size)
      {
      vtksys_ios::ostringstream ost2;
      ost2 << name;
      if (size > 1)
        {
        ost2<<"["<<element<<"]";
        }
      vtkStdString arrayElementName;
      arrayElementName = ost2.str();
      GLint loc = vtkgl::GetUniformLocation(progId,arrayElementName);
      if (elementSize > 1)
        {
        os << "{";
        }
      if (loc == -1)
        {
        // this a built-in variable like:
        // gl_DepthRange.near, gl_DepthRange.far, gl_DepthRange.diff,
        // gl_ProjectionMatrixInverse, ...
        // we cannot get the value through vtkgl::GetUniform*()
        os << "<built-in value>";
        }
      else
        {
        if (isInt)
          {
          vtkgl::GetUniformiv(progId, loc, ivalues);
          int j = 0;
          while (j < elementSize)
            {
            os << ivalues[j];
            if (j < (elementSize - 1))
              {
              os << " ";
              }
            ++j;
            }
          if (isSampler)
            {
            os << " (Texture Unit)->";
            GLint savedActiveTextureUnit;
            glGetIntegerv(vtkgl::ACTIVE_TEXTURE,&savedActiveTextureUnit);
            savedActiveTextureUnit = static_cast<GLint>(
              static_cast<GLenum>(savedActiveTextureUnit) - vtkgl::TEXTURE0);
            if (savedActiveTextureUnit != ivalues[0])
              {
              vtkgl::ActiveTexture(vtkgl::TEXTURE0 +
                                   static_cast<GLenum>(ivalues[0]));
              }
            GLint textureObject;
            glGetIntegerv(textureBinding, &textureObject);
            if (savedActiveTextureUnit != ivalues[0])
              {
              vtkgl::ActiveTexture(
                vtkgl::TEXTURE0 + static_cast<GLenum>(savedActiveTextureUnit));
              }
            os << textureObject << " (Texture Object)";
            }
          }
        else
          {
          vtkgl::GetUniformfv(progId, loc, fvalues);
          int j = 0;
          while(j < elementSize)
            {
            os << fvalues[j];
            if (j < (elementSize - 1))
              {
              os << " ";
              }
            ++j;
            }
          }
        }
      if (elementSize > 1)
        {
        os << "}";
        }
      if (element < (size - 1))
        {
        os << " ";
        }
      ++element;
      }
    delete[] ivalues;
    delete[] fvalues;

    if (size > 1)
      {
      os << "}";
      }
    os << endl;
    ++i;
    }
  delete[] name;
}

//----------------------------------------------------------------------------
// Description:
// Call PrintActiveUniformVariables on cout. Useful for calling inside gdb.
void vtkShaderProgram2::PrintActiveUniformVariablesOnCout()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());
  assert("pre: built" &&
         this->GetLastBuildStatus() == VTK_SHADER_PROGRAM2_LINK_SUCCEEDED);
  vtkIndent i;
  this->PrintActiveUniformVariables(cout, i);
}

//----------------------------------------------------------------------------
// Description:
// Tell if the shader program is valid with the current OpenGL state.
bool vtkShaderProgram2::IsValid()
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());
  assert("pre: built" &&
         this->GetLastBuildStatus() == VTK_SHADER_PROGRAM2_LINK_SUCCEEDED);

  // this line change the program log.
  GLuint progId = static_cast<GLuint>(this->Id);
  vtkgl::ValidateProgram(progId);

  GLint value;
  vtkgl::GetProgramiv(progId, vtkgl::VALIDATE_STATUS, &value);

  bool result = (value == GL_TRUE);

  vtkgl::GetProgramiv(progId, vtkgl::INFO_LOG_LENGTH, &value);
  if (static_cast<size_t>(value)>this->LastValidateLogCapacity)
    {
    delete[] this->LastValidateLog;
    this->LastValidateLogCapacity = static_cast<size_t>(value);
    this->LastValidateLog = new char[this->LastValidateLogCapacity];
    }
  vtkgl::GetProgramInfoLog(progId, value, 0, this->LastValidateLog);

  return result;
}

//----------------------------------------------------------------------------
// Description:
// Tells if the last build: failed during compilation of one of the
// shader, fails during link of the program or succeeded to link the
// program.
// Initial value is VTK_SHADER_PROGRAM2_COMPILE_FAILED.
// \post valid_value: result== VTK_SHADER_PROGRAM2_COMPILE_FAILED ||
// result==VTK_SHADER_PROGRAM2_LINK_FAILED ||
// result==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
int vtkShaderProgram2::GetLastBuildStatus()
{
  return this->LastBuildStatus;
}

//----------------------------------------------------------------------------
// Description:
// Return the log of the last link as a string.
// Initial value is the empty string ""='\0'.
// \post result_exists: result!=0
const char *vtkShaderProgram2::GetLastLinkLog()
{
  assert("post: result_exists" && this->LastLinkLog);
  return this->LastLinkLog;
}

//----------------------------------------------------------------------------
// Description:
// Return the log of the last call to IsValid as a string.
// Initial value is the empty string ""='\0'.
// \post result_exists: result!=0
const char *vtkShaderProgram2::GetLastValidateLog()
{
  assert("post: result_exists" && this->LastValidateLog);
  return this->LastValidateLog;
}

//----------------------------------------------------------------------------
int vtkShaderProgram2::GetAttributeLocation(const char *name)
{
  assert("pre: context_is_set" && this->Context);
  assert("pre: current_context_matches" && this->Context->IsCurrent());
  assert("pre: name_exists" && name);
  assert("pre: built" &&
         this->LastBuildStatus == VTK_SHADER_PROGRAM2_LINK_SUCCEEDED);
  return vtkgl::GetAttribLocation(this->Id, name);
}

//----------------------------------------------------------------------------
void vtkShaderProgram2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Context: ";
  if (this->Context)
    {
    os << static_cast<void *>(this->Context) <<endl;
    }
  else
    {
    os << "none" << endl;
    }

  os << indent << "PrintErrors: ";
  if (this->PrintErrors)
    {
    os << "true" << endl;
    }
  else
    {
    os << "false" << endl;
    }

  os << indent << "LastBuildStatus: ";
  switch(this->LastBuildStatus)
    {
    case VTK_SHADER_PROGRAM2_COMPILE_FAILED:
      os << "Compile failed";
      break;
    case VTK_SHADER_PROGRAM2_LINK_FAILED:
      os << "Link failed";
      break;
    case VTK_SHADER_PROGRAM2_LINK_SUCCEEDED:
      os << "Link succeeded";
      break;
    default:
      os << "ERROR unknown value!";
      break;
    }
  os << endl;

  os << indent << "OpenGL Id: " << this->Id << endl;

  os << indent << "GeometryTypeIn: " <<
    vtkGeometryTypeInAsStringArray[this->GeometryTypeIn] << endl;

  os << indent << "GeometryTypeOut: " <<
    vtkGeometryTypeInAsStringArray[this->GeometryTypeOut] << endl;

  os << indent << "GeometryVerticesOut: " << this->GeometryVerticesOut << endl;

  os << indent << "UniformVariables: ";
  if (this->UniformVariables)
    {
    this->UniformVariables->PrintSelf(os, indent);
    }
  else
    {
    os << "none" << endl;
    }

  os << indent << "Shaders: ";
  if (this->Shaders)
    {
    this->Shaders->PrintSelf(os, indent);
    }
  else
    {
    os << "none" <<  endl;
    }
  }
