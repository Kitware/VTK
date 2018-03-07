/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLState.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLState
 * @brief   OpenGL state storage
 *
 * vtkOpenGLState is a class designed to keep track of the state of
 * an OpenGL context. Applications using VTK have so much ontrol
 * over the rendering process that is can be difficult in VTK code
 * to know if the OpenGL state is correct for your code. The two
 * traditional solutions have been to set everything yourself
 * and to save and restore OpenGL state that you change. The former
 * makes your code work, the latter hels prevent your code from
 * breaking something else. The problem is that the former results
 * in tons of redundent OpenGL calls and the later is done by querying
 * the OpenGL state which can cause a pipeline sync/stall which is
 * very slow.
 *
 * To address these issues this class stores OpenGL state for commonly
 * used functions. Requests made to change state to the current
 * state become no-ops. Queries of state can be done by querying the
 * state stored in this class without impacting the OpenGL driver.
 *
 * To fascilitate saving state and restoring it this class contains
 * a number of nested classes named Scoped<glFunction> that store
 * the state of that glFunction and when they go out of scope they restore
 * it. This is useful when you want to change the OpenGL state and then
 * automatically restore it when done. They can be used as follows
 *
 * {
 *   vtkOpenGLState *ostate = renWin->GetState();
 *   vtkOpenGLState::ScopedglDepthMask dmsaved(ostate);
 *   // the prior state is now saved
 *   ...
 *   ostate->glDepthMask(GL_TRUE);  // maybe change the state
 *   ... etc
 * } // prior state will be restored here as it goes out of scope
 *
 *
 * You must use this class to make stat changing OpenGL class otherwise the
 * results will be undefined.
 *
 * For convenience some OpenGL calls that do not impact state are also
 * provided.
 */

#ifndef vtkOpenGLState_h
#define vtkOpenGLState_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include <array>      // for ivar
#include "vtk_glew.h" // for gl types

class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLState
{
public:

  //@{
  // cached OpenGL methods. By calling these the context will check
  // the current value prior to making the OpenGL call. This can reduce
  // the burden on the driver.
  //
  void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
  void glClearDepth(double depth);
  void glDepthFunc(GLenum val);
  void glDepthMask(GLboolean flag);
  void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
  void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
  void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
  void glEnable(GLenum cap);
  void glDisable(GLenum cap);
  void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    this->glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
  }
  void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB,
    GLenum sfactorAlpha, GLenum dfactorAlpha);
  void glBlendEquation(GLenum val);
  void glCullFace(GLenum val);
  //@}

  //@{
  // OpenGL functions that we provide an API for even though they may
  // not hold any state.
  void glClear(GLbitfield mask);
  //@}


  //@{
  // Get methods that can be used to query state
  GLenum GetCullFace() { return this->CurrentState.CullFaceMode; }
  bool GetEnumState(GLenum name);
  void GetBlendFuncState(int *);
  void GetClearColor(GLclampf *);
  bool GetDepthMask();
  //@}

  // convenience method to set a enum (glEnable/glDisable)
  void SetEnumState(GLenum name, bool value);

  // superclass for Scoped subclasses
  template <typename T>
  class VTKRENDERINGOPENGL2_EXPORT ScopedValue
  {
    public:
      ~ScopedValue() // restore value
      {
        ((*this->State).*(this->Method))(this->Value);
      }
    protected:
      vtkOpenGLState *State;
      T Value;
      void (vtkOpenGLState::*Method)(T);
  };

  // Scoped classes you can use to save state
  class VTKRENDERINGOPENGL2_EXPORT ScopedglDepthMask
    : public ScopedValue<GLboolean> {
    public: ScopedglDepthMask(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglClearColor
    : public ScopedValue<std::array<GLclampf, 4> > {
    public: ScopedglClearColor(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglColorMask
    : public ScopedValue<std::array<GLboolean, 4> > {
    public: ScopedglColorMask(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglScissor
    : public ScopedValue<std::array<GLint, 4> > {
    public: ScopedglScissor(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglViewport
    : public ScopedValue<std::array<GLint, 4> > {
    public: ScopedglViewport(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglBlendFuncSeparate
    : public ScopedValue<std::array<GLenum, 4> > {
    public: ScopedglBlendFuncSeparate(vtkOpenGLState *state); };

  class ScopedglEnableDisable
  {
    public:
      ScopedglEnableDisable(vtkOpenGLState *state, GLenum name)
      {
        this->State = state;
        this->Name = name;
        this->Value = this->State->GetEnumState(name);
      }
      ~ScopedglEnableDisable() // restore value
      {
        this->State->SetEnumState(this->Name, this->Value);
      }
    protected:
      vtkOpenGLState *State;
      GLenum Name;
      bool Value;
  };

  // initialize both OpenGL and these state ivars to known
  // and consistent values
  void Initialize(vtkOpenGLRenderWindow *);

protected:
  void BlendFuncSeparate(std::array<GLenum, 4> val);
  void ClearColor(std::array<GLclampf, 4> val);
  void ColorMask(std::array<GLboolean, 4> val);
  void Scissor(std::array<GLint, 4> val);
  void Viewport(std::array<GLint, 4> val);

  class VTKRENDERINGOPENGL2_EXPORT GLState
  {
    public:
      double ClearDepth;
      GLboolean DepthMask;
      GLenum DepthFunc;
      GLenum BlendEquation;
      GLenum CullFaceMode;
      std::array<GLclampf, 4> ClearColor;
      std::array<GLboolean, 4> ColorMask;
      std::array<GLint, 4> Viewport;
      std::array<GLint, 4> Scissor;
      std::array<GLenum, 4> BlendFunc;
      bool DepthTest;
      bool CullFace;
      bool ScissorTest;
      bool StencilTest;
      bool Blend;
      bool MultiSample;
      GLState() {
      }
  };

  GLState CurrentState;
};

#endif

// VTK-HeaderTest-Exclude: vtkOpenGLState.h
