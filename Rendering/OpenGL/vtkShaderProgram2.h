/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderProgram2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShaderProgram2 - GLSL Program
// .SECTION Description
// vtkShaderProgram2 represents an implementation of the programmable OpenGL
// pipeline. It consists of a list of vtkShader2 object. Each vtkShader2 is a
// piece of source code associated with one of the shader units (vertex,
// fragment, geometry).

#ifndef __vtkShaderProgram2_h
#define __vtkShaderProgram2_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkWindow;
class vtkOpenGLRenderWindow;
class vtkOpenGLExtensionManager;
class vtkShaderProgram2Uniforms; // internal
class vtkShader2Collection;
class vtkUniformVariables;

// Values for GetLastBuildStatus()
enum vtkShaderProgram2BuildStatus
{
  // one of the shaders failed to compile
  VTK_SHADER_PROGRAM2_COMPILE_FAILED=0,
  // all the shaders compiled successfully but the link failed
  VTK_SHADER_PROGRAM2_LINK_FAILED=1,
  // all the shaders compiled successfully and the link succeeded
  VTK_SHADER_PROGRAM2_LINK_SUCCEEDED=2
};

enum vtkShaderProgram2GeometryInType
{
  VTK_GEOMETRY_SHADER_IN_TYPE_POINTS,
  VTK_GEOMETRY_SHADER_IN_TYPE_LINES,
  VTK_GEOMETRY_SHADER_IN_TYPE_LINES_ADJACENCY,
  VTK_GEOMETRY_SHADER_IN_TYPE_TRIANGLES,
  VTK_GEOMETRY_SHADER_IN_TYPE_TRIANGLES_ADJACENCY
};

enum vtkShaderProgram2GeometryOutType
{
  VTK_GEOMETRY_SHADER_OUT_TYPE_POINTS,
  VTK_GEOMETRY_SHADER_OUT_TYPE_LINE_STRIP,
  VTK_GEOMETRY_SHADER_OUT_TYPE_TRIANGLE_STRIP
};

class VTKRENDERINGOPENGL_EXPORT vtkShaderProgram2 : public vtkObject
{
public:
  static vtkShaderProgram2* New();
  vtkTypeMacro(vtkShaderProgram2, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns if the context supports the required extensions.
  static bool IsSupported(vtkOpenGLRenderWindow *context);
  static bool LoadExtensions(vtkOpenGLRenderWindow *context);

  // Description:
  // Tell if vtkErrorMacro should be called when there is a build error or not.
  // It is useful to switch it to false when building a shader is
  // a way to test if some OpenGL implementation support it or not.
  // Initial value is true.
  // Build errors are always reported in the status flags and log whatever is
  // the value of PrintErrors flag.
  vtkGetMacro(PrintErrors,bool);
  vtkSetMacro(PrintErrors,bool);

  // Description:
  // Get/Set the context. This does not increase the reference count of the
  // context to avoid reference loops.
  // SetContext() may raise an error is the OpenGL context does not support the
  // required OpenGL extensions.
  void SetContext(vtkOpenGLRenderWindow *context);
  vtkGetObjectMacro(Context,vtkOpenGLRenderWindow);

  // Description:
  // The list of shaders. Initially, the list is empty.
  // \post result_exists: result!=0
  vtkGetObjectMacro(Shaders,vtkShader2Collection);

  // Description:
  // Tells if at least one of the shaders is a vertex shader.
  // If yes, it means the vertex processing of the fixed-pipeline is bypassed.
  // If no, it means the vertex processing of the fixed-pipeline is used.
  bool HasVertexShaders();

  // Description:
  // Tells if at least one of the shaders is a tessellation control shader.
  bool HasTessellationControlShaders();

  // Description:
  // Tells if at least one of the shaders is a tessellation evaluation shader.
  bool HasTessellationEvaluationShaders();

  // Description:
  // Tells if at least one of the shaders is a geometry shader.
  bool HasGeometryShaders();

  // Description:
  // Tells if at least one of the shaders is a fragment shader.
  // If yes, it means the fragment processing of the fixed-pipeline is
  // bypassed.
  // If no, it means the fragment processing of the fixed-pipeline is used.
  bool HasFragmentShaders();

  // Description:
  // Tell if the shader program is valid with the current OpenGL state.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->Context()->IsCurrent()
  // \pre built this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
  bool IsValid();

  // Description:
  // If not done yet, compile all the shaders and link the program.
  // The status of the build can then be query with GetLastBuildStatus()
  // and GetLastLinkLog().
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->GetContext()->IsCurrent()
  void Build();

  // Description:
  // Send the uniform variables values to the program.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->GetContext()->IsCurrent()
  // \pre built this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
  void SendUniforms();

  // Description:
  // Introspection. Return the list of active uniform variables of the program.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->Context()->IsCurrent()
  // \pre built this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
  void PrintActiveUniformVariables(ostream &os,
                                   vtkIndent indent);

  // Description:
  // Call PrintActiveUniformVariables on cout. Useful for calling inside gdb.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->Context()->IsCurrent()
  // \pre built this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
  void PrintActiveUniformVariablesOnCout();

  // Description:
  // Tell if the program is the one currently used by OpenGL.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->GetContext()->IsCurrent()
  bool IsUsed();

  // Description:
  // Use the shader program.
  // It saves the current shader program or fixed-pipeline in use.
  // It also set the uniform variables.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->GetContext()->IsCurrent()
  void Use();

  // Description:
  // Restore the previous shader program (or fixed-pipeline).
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->GetContext()->IsCurrent()
  void Restore();

  // Description:
  // Force the current shader program to be the fixed-pipeline.
  // Warning: this call will be compiled if called inside a display list
  // creation.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->GetContext()->IsCurrent()
  void RestoreFixedPipeline();

  // Description:
  // Tells if the last build: failed during compilation of one of the
  // shader, fails during link of the program or succeeded to link the
  // program.
  // Initial value is VTK_SHADER_PROGRAM2_COMPILE_FAILED.
  // \post valid_value: result== VTK_SHADER_PROGRAM2_COMPILE_FAILED ||
  // result==VTK_SHADER_PROGRAM2_LINK_FAILED ||
  // result==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
  int GetLastBuildStatus();

  // Description:
  // Return the log of the last link as a string.
  // Initial value is the empty string ""='\0'.
  // \post result_exists: result!=0
  const char *GetLastLinkLog();

  // Description:
  // Return the log of the last call to IsValid as a string.
  // Initial value is the empty string ""='\0'.
  // \post result_exists: result!=0
  const char *GetLastValidateLog();

  // Description:
  // Release OpenGL resource (program id and sub-resources).
  virtual void ReleaseGraphicsResources();

  // Description:
  // Returns the generic attribute location.
  // The shader must be bound before calling this.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->GetContext()->IsCurrent()
  // \pre name_exists: name!=0
  // \pre built: this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
  int GetAttributeLocation(const char *name);

  // Description:
  // Get/Set the list of uniform variables values.
  // Initial value is an empty list (not null pointer).
  vtkGetObjectMacro(UniformVariables,vtkUniformVariables);
  virtual void SetUniformVariables(vtkUniformVariables *variables);

  // Description:
  // Tells if a display list is under construction with GL_COMPILE mode.
  // Return false if there is no display list under construction of if the
  // mode is GL_COMPILE_AND_EXECUTE.
  // Used internally and provided as a public method for whoever find it
  // useful.
  // \pre context_is_set: this->GetContext()!=0
  // \pre current_context_matches: this->GetContext()->IsCurrent()
  bool DisplayListUnderCreationInCompileMode();

  // Description:
  // Specific to the geometry shader part of the program.
  // Relevant only when HasGeometryShaders() is true.
  // From OpenGL 3.2, it is replaced by an input layout qualifier in GLSL
  // 1.50.
  // The input primitive type on which the geometry shader operate.
  // It can be VTK_GEOMETRY_SHADER_IN_TYPE_POINTS,
  // VTK_GEOMETRY_SHADER_IN_TYPE_LINES,
  // VTK_GEOMETRY_SHADER_IN_TYPE_LINES_ADJACENCY,
  // VTK_GEOMETRY_SHADER_IN_TYPE_TRIANGLES or
  // VTK_GEOMETRY_SHADER_IN_TYPE_TRIANGLES_ADJACENCY
  // Initial value is VTK_GEOMETRY_SHADER_IN_TYPE_POINTS.
  vtkSetMacro(GeometryTypeIn,int);
  vtkGetMacro(GeometryTypeIn,int);

  // Description:
  // Specific to the geometry shader part of the program.
  // Relevant only when HasGeometryShaders() is true.
  // This is a pre OpenGL 3.2 geometry shader setting.
  // From OpenGL 3.2, it is replaced by an output layout qualifier in GLSL
  // 1.50.
  // The maximum number of vertices the geometry shader will emit in one
  // invocation.
  // If a geometry shader, in one invocation, emits more vertices than this
  // value, these emits may have no effect.
  // Initial value is 1.
  vtkSetMacro(GeometryVerticesOut,int);
  vtkGetMacro(GeometryVerticesOut,int);

  // Description:
  // Specific to the geometry shader part of the program.
  // Relevant only when HasGeometryShaders() is true.
  // From OpenGL 3.2, it is replaced by an output layout qualifier in GLSL
  // 1.50.
  // The output primitive type generated by the geometry shader.
  // It can be VTK_GEOMETRY_SHADER_OUT_TYPE_POINTS,
  // VTK_GEOMETRY_SHADER_OUT_TYPE_LINE_STRIP or
  // VTK_GEOMETRY_SHADER_OUT_TYPE_TRIANGLE_STRIP.
  // Initial value is VTK_GEOMETRY_SHADER_OUT_TYPE_POINTS.
  vtkSetMacro(GeometryTypeOut,int);
  vtkGetMacro(GeometryTypeOut,int);

protected:
  vtkShaderProgram2();
  virtual ~vtkShaderProgram2();

  unsigned int Id; // actually GLuint. Initial value is 0.
  unsigned int SavedId;

  vtkTimeStamp LastLinkTime;
  vtkTimeStamp LastSendUniformsTime;

  vtkShaderProgram2Uniforms *Uniforms;
  vtkShader2Collection *Shaders;

  int LastBuildStatus; // Initial value is VTK_SHADER_PROGRAM2_COMPILE_FAILED

  char *LastLinkLog; // Initial value is the empty string ""='\0'
  size_t LastLinkLogCapacity; // Initial value is 8.

  char *LastValidateLog; // Initial value is the empty string ""='\0'
  size_t LastValidateLogCapacity; // Initial value is 8.

  vtkUniformVariables *UniformVariables; // Initial values is an empty list

  bool PrintErrors; // use vtkErrorMacro ?

  vtkOpenGLRenderWindow *Context;
  bool ExtensionsLoaded;

  int GeometryTypeIn;
  int GeometryTypeOut;
  int GeometryVerticesOut;

private:
  vtkShaderProgram2(const vtkShaderProgram2&); // Not implemented.
  void operator=(const vtkShaderProgram2&); // Not implemented.
};

#endif
