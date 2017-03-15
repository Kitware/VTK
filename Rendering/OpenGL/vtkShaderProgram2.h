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
/**
 * @class   vtkShaderProgram2
 * @brief   GLSL Program
 *
 * vtkShaderProgram2 represents an implementation of the programmable OpenGL
 * pipeline. It consists of a list of vtkShader2 object. Each vtkShader2 is a
 * piece of source code associated with one of the shader units (vertex,
 * fragment, geometry).
*/

#ifndef vtkShaderProgram2_h
#define vtkShaderProgram2_h

#include <cassert> // for templated functions
#include "vtkRenderingOpenGLModule.h" // for export macro
#include "vtkWeakPointer.h" // for weak ptr to rendering context
#include "vtkObject.h"

class vtkRenderWindow;
class vtkOpenGLExtensionManager;
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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns if the context supports the required extensions.
   * Extensions are loaded when the context is set.
   */
  static bool IsSupported(vtkRenderWindow *context);

  //@{
  /**
   * Tell if vtkErrorMacro should be called when there is a build error or not.
   * It is useful to switch it to false when building a shader is
   * a way to test if some OpenGL implementation support it or not.
   * Initial value is true.
   * Build errors are always reported in the status flags and log whatever is
   * the value of PrintErrors flag.
   */
  vtkGetMacro(PrintErrors,bool);
  vtkSetMacro(PrintErrors,bool);
  //@}

  //@{
  /**
   * Get/Set the context. This does not increase the reference count of the
   * context to avoid reference loops.
   * SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions.
   */
  void SetContext(vtkRenderWindow *context);
  vtkRenderWindow *GetContext();
  //@}

  //@{
  /**
   * The list of shaders. Initially, the list is empty.
   * \post result_exists: result!=0
   */
  vtkGetObjectMacro(Shaders,vtkShader2Collection);
  //@}

  /**
   * Tells if at least one of the shaders is a vertex shader.
   * If yes, it means the vertex processing of the fixed-pipeline is bypassed.
   * If no, it means the vertex processing of the fixed-pipeline is used.
   */
  bool HasVertexShaders();

  /**
   * Tells if at least one of the shaders is a tessellation control shader.
   */
  bool HasTessellationControlShaders();

  /**
   * Tells if at least one of the shaders is a tessellation evaluation shader.
   */
  bool HasTessellationEvaluationShaders();

  /**
   * Tells if at least one of the shaders is a geometry shader.
   */
  bool HasGeometryShaders();

  /**
   * Tells if at least one of the shaders is a fragment shader.
   * If yes, it means the fragment processing of the fixed-pipeline is
   * bypassed.
   * If no, it means the fragment processing of the fixed-pipeline is used.
   */
  bool HasFragmentShaders();

  /**
   * Tell if the shader program is valid with the current OpenGL state.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->Context()->IsCurrent()
   * \pre built this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
   */
  bool IsValid();

  /**
   * If not done yet, compile all the shaders and link the program.
   * The status of the build can then be query with GetLastBuildStatus()
   * and GetLastLinkLog().
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->GetContext()->IsCurrent()
   */
  void Build();

  /**
   * Send the uniform variables values to the program.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->GetContext()->IsCurrent()
   * \pre built this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
   */
  void SendUniforms();

  /**
   * Introspection. Return the list of active uniform variables of the program.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->Context()->IsCurrent()
   * \pre built this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
   */
  void PrintActiveUniformVariables(ostream &os,
                                   vtkIndent indent);

  /**
   * Call PrintActiveUniformVariables on cout. Useful for calling inside gdb.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->Context()->IsCurrent()
   * \pre built this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
   */
  void PrintActiveUniformVariablesOnCout();

  /**
   * Tell if the program is the one currently used by OpenGL.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->GetContext()->IsCurrent()
   */
  bool IsUsed();

  /**
   * Use the shader program.
   * It saves the current shader program or fixed-pipeline in use.
   * As a side affect it also set the uniform variables. If you don't
   * want that then see UseProgram.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->GetContext()->IsCurrent()
   */
  void Use();

  /**
   * Restore the previous shader program (or fixed-pipeline).
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->GetContext()->IsCurrent()
   */
  void Restore();

  /**
   * Force the current shader program to be the fixed-pipeline.
   * Warning: this call will be compiled if called inside a display list
   * creation.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->GetContext()->IsCurrent()
   */
  void RestoreFixedPipeline();

  //@{
  /**
   * Simple direct use of the program without side affects and with
   * error check. The Unuse version restores the default program.
   */
  void UseProgram();
  void UnuseProgram();
  //@}

  /**
   * Tells if the last build: failed during compilation of one of the
   * shader, fails during link of the program or succeeded to link the
   * program.
   * Initial value is VTK_SHADER_PROGRAM2_COMPILE_FAILED.
   * \post valid_value: result== VTK_SHADER_PROGRAM2_COMPILE_FAILED ||
   * result==VTK_SHADER_PROGRAM2_LINK_FAILED ||
   * result==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
   */
  int GetLastBuildStatus();

  /**
   * Return the log of the last link as a string.
   * Initial value is the empty string ""='\0'.
   * \post result_exists: result!=0
   */
  const char *GetLastLinkLog();

  /**
   * Return the log of the last call to IsValid as a string.
   * Initial value is the empty string ""='\0'.
   * \post result_exists: result!=0
   */
  const char *GetLastValidateLog();

  /**
   * Release OpenGL resource (program id and sub-resources).
   */
  virtual void ReleaseGraphicsResources();

  /**
   * Returns the generic attribute location.
   * The shader must be bound before calling this.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->GetContext()->IsCurrent()
   * \pre name_exists: name!=0
   * \pre built: this->GetLastBuildStatus()==VTK_SHADER_PROGRAM2_LINK_SUCCEEDED
   */
  int GetAttributeLocation(const char *name);

  //@{
  /**
   * Get/Set the list of uniform variables values.
   * Initial value is an empty list (not null pointer).
   */
  vtkGetObjectMacro(UniformVariables,vtkUniformVariables);
  virtual void SetUniformVariables(vtkUniformVariables *variables);
  //@}

  /**
   * Tells if a display list is under construction with GL_COMPILE mode.
   * Return false if there is no display list under construction of if the
   * mode is GL_COMPILE_AND_EXECUTE.
   * Used internally and provided as a public method for whoever find it
   * useful.
   * \pre context_is_set: this->GetContext()!=0
   * \pre current_context_matches: this->GetContext()->IsCurrent()
   */
  bool DisplayListUnderCreationInCompileMode();

  //@{
  /**
   * Specific to the geometry shader part of the program.
   * Relevant only when HasGeometryShaders() is true.
   * From OpenGL 3.2, it is replaced by an input layout qualifier in GLSL
   * 1.50.
   * The input primitive type on which the geometry shader operate.
   * It can be VTK_GEOMETRY_SHADER_IN_TYPE_POINTS,
   * VTK_GEOMETRY_SHADER_IN_TYPE_LINES,
   * VTK_GEOMETRY_SHADER_IN_TYPE_LINES_ADJACENCY,
   * VTK_GEOMETRY_SHADER_IN_TYPE_TRIANGLES or
   * VTK_GEOMETRY_SHADER_IN_TYPE_TRIANGLES_ADJACENCY
   * Initial value is VTK_GEOMETRY_SHADER_IN_TYPE_POINTS.
   */
  vtkSetMacro(GeometryTypeIn,int);
  vtkGetMacro(GeometryTypeIn,int);
  //@}

  //@{
  /**
   * Specific to the geometry shader part of the program.
   * Relevant only when HasGeometryShaders() is true.
   * This is a pre OpenGL 3.2 geometry shader setting.
   * From OpenGL 3.2, it is replaced by an output layout qualifier in GLSL
   * 1.50.
   * The maximum number of vertices the geometry shader will emit in one
   * invocation.
   * If a geometry shader, in one invocation, emits more vertices than this
   * value, these emits may have no effect.
   * Initial value is 1.
   */
  vtkSetMacro(GeometryVerticesOut,int);
  vtkGetMacro(GeometryVerticesOut,int);
  //@}

  //@{
  /**
   * Specific to the geometry shader part of the program.
   * Relevant only when HasGeometryShaders() is true.
   * From OpenGL 3.2, it is replaced by an output layout qualifier in GLSL
   * 1.50.
   * The output primitive type generated by the geometry shader.
   * It can be VTK_GEOMETRY_SHADER_OUT_TYPE_POINTS,
   * VTK_GEOMETRY_SHADER_OUT_TYPE_LINE_STRIP or
   * VTK_GEOMETRY_SHADER_OUT_TYPE_TRIANGLE_STRIP.
   * Initial value is VTK_GEOMETRY_SHADER_OUT_TYPE_POINTS.
   */
  vtkSetMacro(GeometryTypeOut,int);
  vtkGetMacro(GeometryTypeOut,int);
  //@}

  /*
  Low level api --
  this is provided as a way to avoid some of the overhead in this
  class's implementation of SendUniforms. One should use the
  following API if performance is a concern (eg. uniforms are
  set per primitive), or if the uniform management is not needed
  (eg. variables are already managed in other vtkObjects)
  */

  /**
   * Get a uniform's location.
   * Low level API
   */
  int GetUniformLocation(const char *name);

  /**
   * Set a uniform value directly. The driving use case for this api
   * is modifying a uniform per-primitive in a loop. In that case
   * we need the minimal implementtion passing the value directly to
   * the driver. It is an error to specify an invalid location.
   * Low level API
   */
  void SetUniformf(const char *name, float val)
    { this->SetUniform1f(name, &val); }
  void SetUniform1f(const char *name, float *val)
    { this->SetUniform1f(this->GetUniformLocation(name), val); }
  void SetUniform2f(const char *name, float *val)
    { this->SetUniform2f(this->GetUniformLocation(name), val); }
  void SetUniform3f(const char *name, float *val)
    { this->SetUniform3f(this->GetUniformLocation(name), val); }
  void SetUniform4f(const char *name, float *val)
    { this->SetUniform4f(this->GetUniformLocation(name), val); }

  void SetUniformi(const char *name, int val)
    { this->SetUniform1i(name, &val); }
  void SetUniform1i(const char *name, int *val)
    { this->SetUniform1i(this->GetUniformLocation(name), val); }
  void SetUniform2i(const char *name, int *val)
    { this->SetUniform2i(this->GetUniformLocation(name), val); }
  void SetUniform3i(const char *name, int *val)
    { this->SetUniform3i(this->GetUniformLocation(name), val); }
  void SetUniform4i(const char *name, int *val)
    { this->SetUniform4i(this->GetUniformLocation(name), val); }

  void SetUniformf(int loc, float val)
    { this->SetUniform1f(loc, &val); }
  void SetUniform1f(int loc, float *val);
  void SetUniform2f(int loc, float *val);
  void SetUniform3f(int loc, float *val);
  void SetUniform4f(int loc, float *val);

  void SetUniformi(int loc, int val)
    { this->SetUniform1i(loc, &val); }
  void SetUniform1i(int loc, int *val);
  void SetUniform2i(int loc, int *val);
  void SetUniform3i(int loc, int *val);
  void SetUniform4i(int loc, int *val);

  //@{
  /**
   * Convenience methods for copy/convert to supported type. Typically
   * this arises because VTK stores data in an internal format (eg double)
   * that's not supported.
   */
  template<typename T> void SetUniform1it(const char *name, T *value);
  template<typename T> void SetUniform2it(const char *name, T *value);
  template<typename T> void SetUniform3it(const char *name, T *value);
  template<typename T> void SetUniform4it(const char *name, T *value);
  //@}

  template<typename T> void SetUniform1ft(const char *name, T *value);
  template<typename T> void SetUniform2ft(const char *name, T *value);
  template<typename T> void SetUniform3ft(const char *name, T *value);
  template<typename T> void SetUniform4ft(const char *name, T *value);

  template<typename T> void SetUniform1it(int loc, T *value);
  template<typename T> void SetUniform2it(int loc, T *value);
  template<typename T> void SetUniform3it(int loc, T *value);
  template<typename T> void SetUniform4it(int loc, T *value);

  template<typename T> void SetUniform1ft(int loc, T *value);
  template<typename T> void SetUniform2ft(int loc, T *value);
  template<typename T> void SetUniform3ft(int loc, T *value);
  template<typename T> void SetUniform4ft(int loc, T *value);

protected:
  vtkShaderProgram2();
  ~vtkShaderProgram2() VTK_OVERRIDE;

  /**
   * Load the required OpenGL extensions.
   */
  bool LoadRequiredExtensions(vtkRenderWindow *context);

  /**
   * Get the location of a uniform, without
   * caring if it really exists. This is used
   * because this class will attempt to set *all*
   * uniforms knows about via the associated
   * vtkUniformVariables on *all* shaders it manages
   * regardless of if a given uniform actually
   * belongs to a given shader.
   */
  int GetUniformLocationInternal(const char *name);

  unsigned int Id; // actually GLuint. Initial value is 0.
  unsigned int SavedId;

  vtkTimeStamp LastLinkTime;
  vtkTimeStamp LastSendUniformsTime;

  vtkShader2Collection *Shaders;
  vtkUniformVariables *UniformVariables;

  int LastBuildStatus; // Initial value is VTK_SHADER_PROGRAM2_COMPILE_FAILED

  char *LastLinkLog; // Initial value is the empty string ""='\0'
  size_t LastLinkLogCapacity; // Initial value is 8.

  char *LastValidateLog; // Initial value is the empty string ""='\0'
  size_t LastValidateLogCapacity; // Initial value is 8.


  bool PrintErrors;

  vtkWeakPointer<vtkRenderWindow> Context;
  bool ExtensionsLoaded;

  int GeometryTypeIn;
  int GeometryTypeOut;
  int GeometryVerticesOut;

private:
  vtkShaderProgram2(const vtkShaderProgram2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkShaderProgram2&) VTK_DELETE_FUNCTION;
};

// ----------------------------------------------------------------------------

#define vtkShaderProgram2SetUniformCopyCastMacro(toLetter, toType, num) \
template<typename fromType> \
void vtkShaderProgram2::SetUniform##num##toLetter##t(const char *name, fromType *fvalues) \
{ \
  toType tvalues[num]; \
  for (int i=0; i<num; ++i) \
  { \
    tvalues[i] = static_cast<toType>(fvalues[i]); \
  } \
  this->SetUniform##num##toLetter(name, tvalues); \
} \
template<typename fromType> \
void vtkShaderProgram2::SetUniform##num##toLetter##t(int location, fromType *fvalues) \
{ \
  assert(location!=-1); \
  toType tvalues[num]; \
  for (int i=0; i<num; ++i) \
  { \
    tvalues[i] = static_cast<toType>(fvalues[i]); \
  } \
  this->SetUniform##num##toLetter(location, tvalues); \
}
vtkShaderProgram2SetUniformCopyCastMacro(f, float, 1)
vtkShaderProgram2SetUniformCopyCastMacro(f, float, 2)
vtkShaderProgram2SetUniformCopyCastMacro(f, float, 3)
vtkShaderProgram2SetUniformCopyCastMacro(f, float, 4)
vtkShaderProgram2SetUniformCopyCastMacro(i, int, 1)
vtkShaderProgram2SetUniformCopyCastMacro(i, int, 2)
vtkShaderProgram2SetUniformCopyCastMacro(i, int, 3)
vtkShaderProgram2SetUniformCopyCastMacro(i, int, 4)

#endif
