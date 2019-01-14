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

class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLState
{
public:
  vtkOpenGLState(); // set initial values

  //@{
  // cached OpenGL methods. By calling these the context will check
  // the current value prior to making the OpenGL call. This can reduce
  // the burden on the driver.
  //
  void vtkglClearColor(float red, float green, float blue, float alpha);
  void vtkglClearDepth(double depth);
  void vtkglDepthFunc(unsigned int val);
  void vtkglDepthMask(unsigned char flag);
  void vtkglColorMask(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void vtkglViewport(int x, int y, int width, int height);
  void vtkglScissor(int x, int y, int width, int height);
  void vtkglEnable(unsigned int cap);
  void vtkglDisable(unsigned int cap);
  void vtkglBlendFunc(unsigned int sfactor, unsigned int dfactor) {
    this->vtkglBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
  }
  void vtkglBlendFuncSeparate(unsigned int sfactorRGB, unsigned int dfactorRGB,
    unsigned int sfactorAlpha, unsigned int dfactorAlpha);
  void vtkglBlendEquation(unsigned int val);
  void vtkglBlendEquationSeparate(unsigned int col, unsigned int alpha);
  void vtkglCullFace(unsigned int val);
  //@}

  //@{
  // Methods to reset the state to the current OpenGL context value.
  //
  void ResetGlClearColorState();
  void ResetGlClearDepthState();
  void ResetGlDepthFuncState();
  void ResetGlDepthMaskState();
  void ResetGlColorMaskState();
  void ResetGlViewportState();
  void ResetGlScissorState();
  void ResetGlBlendFuncState();
  void ResetGlBlendEquationState();
  void ResetGlCullFaceState();
  //@}

  //@{
  // OpenGL functions that we provide an API for even though they may
  // not hold any state.
  void vtkglClear(unsigned int mask);
  //@}


  //@{
  // Get methods that can be used to query state
  // if the state is not cached they fall through
  // and call the underlying opengl functions
  void vtkglGetBooleanv(unsigned int pname, unsigned char *params);
  void vtkglGetIntegerv(unsigned int pname, int *params);
  void vtkglGetDoublev(unsigned int pname, double *params);
  void vtkglGetFloatv(unsigned int pname, float *params);
  //@}

  // convenience to get all 4 values at once
  void GetBlendFuncState(int *);

  // convenience to return a bool
  // as opposed to a unsigned char
  bool GetEnumState(unsigned int name);

  // convenience method to set a enum (glEnable/glDisable)
  void SetEnumState(unsigned int name, bool value);

  /**
   * convenience method to reset an enum state from current openGL context
   */
  void ResetEnumState(unsigned int name);

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
    : public ScopedValue<unsigned char> {
    public: ScopedglDepthMask(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglClearColor
    : public ScopedValue<std::array<float, 4> > {
    public: ScopedglClearColor(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglColorMask
    : public ScopedValue<std::array<unsigned char, 4> > {
    public: ScopedglColorMask(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglScissor
    : public ScopedValue<std::array<int, 4> > {
    public: ScopedglScissor(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglViewport
    : public ScopedValue<std::array<int, 4> > {
    public: ScopedglViewport(vtkOpenGLState *state); };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglBlendFuncSeparate
    : public ScopedValue<std::array<unsigned int, 4> > {
    public: ScopedglBlendFuncSeparate(vtkOpenGLState *state); };

  class ScopedglEnableDisable
  {
    public:
      ScopedglEnableDisable(vtkOpenGLState *state, unsigned int name)
      {
        this->State = state;
        this->Name = name;
        unsigned char val;
        this->State->vtkglGetBooleanv(name, &val);
        this->Value = val == 1;
      }
      ~ScopedglEnableDisable() // restore value
      {
        this->State->SetEnumState(this->Name, this->Value);
      }
    protected:
      vtkOpenGLState *State;
      unsigned int Name;
      bool Value;
  };

  /**
   * Initialize OpenGL context using current state
   */
  void Initialize(vtkOpenGLRenderWindow*);

protected:
  void BlendFuncSeparate(std::array<unsigned int, 4> val);
  void ClearColor(std::array<float, 4> val);
  void ColorMask(std::array<unsigned char, 4> val);
  void Scissor(std::array<int, 4> val);
  void Viewport(std::array<int, 4> val);

  /**
   * Check that this OpenGL state has consistent values
   * with the current OpenGL context
   */
  void CheckState();

  class VTKRENDERINGOPENGL2_EXPORT GLState
  {
    public:
      double ClearDepth;
      unsigned char DepthMask;
      unsigned int DepthFunc;
      unsigned int BlendEquationValue1;
      unsigned int BlendEquationValue2;
      unsigned int CullFaceMode;
      std::array<float, 4> ClearColor;
      std::array<unsigned char, 4> ColorMask;
      std::array<int, 4> Viewport;
      std::array<int, 4> Scissor;
      std::array<unsigned int, 4> BlendFunc;
      bool DepthTest;
      bool CullFace;
      bool ScissorTest;
      bool StencilTest;
      bool Blend;
      bool MultiSample;
      int MaxTextureSize;
      int MajorVersion;
      int MinorVersion;
      GLState() {
      }
  };

  GLState CurrentState;
};

#endif

// VTK-HeaderTest-Exclude: vtkOpenGLState.h
