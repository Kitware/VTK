// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLState
 * @brief   OpenGL state storage
 *
 * vtkOpenGLState is a class designed to keep track of the state of
 * an OpenGL context. Applications using VTK have so much control
 * over the rendering process that is can be difficult in VTK code
 * to know if the OpenGL state is correct for your code. The two
 * traditional solutions have been to set everything yourself
 * and to save and restore OpenGL state that you change. The former
 * makes your code work, the latter helps prevent your code from
 * breaking something else. The problem is that the former results
 * in tons of redundant OpenGL calls and the later is done by querying
 * the OpenGL state which can cause a pipeline sync/stall which is
 * very slow.
 *
 * To address these issues this class stores OpenGL state for commonly
 * used functions. Requests made to change state to the current
 * state become no-ops. Queries of state can be done by querying the
 * state stored in this class without impacting the OpenGL driver.
 *
 * This class is designed to hold all context related values and
 * could just as well be considered a representation of the OpenGL
 * context.
 *
 * To facilitate saving state and restoring it this class contains
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
 * You must use this class to make state changing OpenGL class otherwise the
 * results will be undefined.
 *
 * For convenience some OpenGL calls that do not impact state are also
 * provided.
 */

#ifndef vtkOpenGLState_h
#define vtkOpenGLState_h

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include <array>                       // for ivar
#include <list>                        // for ivar
#include <map>                         // for ivar
#include <stack>                       // for ivar
#include <string>                      // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLShaderCache;
class vtkOpenGLVertexBufferObjectCache;
class vtkTextureObject;
class vtkTextureUnitManager;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLState : public vtkObject
{
public:
  static vtkOpenGLState* New();
  vtkTypeMacro(vtkOpenGLState, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
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
  void vtkglBlendFunc(unsigned int sfactor, unsigned int dfactor)
  {
    this->vtkglBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
  }
  void vtkglBlendFuncSeparate(unsigned int sfactorRGB, unsigned int dfactorRGB,
    unsigned int sfactorAlpha, unsigned int dfactorAlpha);
  void vtkglBlendEquation(unsigned int val);
  void vtkglBlendEquationSeparate(unsigned int col, unsigned int alpha);
  void vtkglCullFace(unsigned int val);
  void vtkglActiveTexture(unsigned int);

  void vtkglBindFramebuffer(unsigned int target, unsigned int fb);
  void vtkglDrawBuffer(unsigned int);
  void vtkglDrawBuffers(unsigned int n, unsigned int*);
  void vtkglReadBuffer(unsigned int);

  void vtkglPointSize(float);
  void vtkglLineWidth(float);
  void vtkglStencilMaskSeparate(unsigned int face, unsigned int mask);
  void vtkglStencilMask(unsigned int mask);
  void vtkglStencilOpSeparate(
    unsigned int face, unsigned int sfail, unsigned int dpfail, unsigned int dppass);
  void vtkglStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass);
  void vtkglStencilFuncSeparate(unsigned int face, unsigned int func, int ref, unsigned int mask);
  void vtkglStencilFunc(unsigned int func, int ref, unsigned int mask);

  void vtkBindFramebuffer(unsigned int target, vtkOpenGLFramebufferObject* fo);
  void vtkDrawBuffers(unsigned int n, unsigned int*, vtkOpenGLFramebufferObject*);
  void vtkReadBuffer(unsigned int, vtkOpenGLFramebufferObject*);

  void vtkglPixelStorei(unsigned int, int);
  ///@}

  ///@{
  // Methods to reset the state to the current OpenGL context value.
  // These methods are useful when interfacing with third party code
  // that may have changed the opengl state.
  //
  void ResetGLClearColorState();
  void ResetGLClearDepthState();
  void ResetGLDepthFuncState();
  void ResetGLDepthMaskState();
  void ResetGLColorMaskState();
  void ResetGLViewportState();
  void ResetGLScissorState();
  void ResetGLBlendFuncState();
  void ResetGLBlendEquationState();
  void ResetGLCullFaceState();
  void ResetGLActiveTexture();
  ///@}

  ///@{
  // OpenGL functions that we provide an API for even though they may
  // not hold any state.
  void vtkglClear(unsigned int mask);
  ///@}

  ///@{
  // Get methods that can be used to query state if the state is not cached
  // they fall through and call the underlying opengl functions
  void vtkglGetBooleanv(unsigned int pname, unsigned char* params);
  void vtkglGetIntegerv(unsigned int pname, int* params);
  void vtkglGetDoublev(unsigned int pname, double* params);
  void vtkglGetFloatv(unsigned int pname, float* params);
  ///@}

  // convenience to get all 4 values at once
  void GetBlendFuncState(int*);

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
    vtkOpenGLState* State;
    T Value;
    void (vtkOpenGLState::*Method)(T);
  };

  /**
   * Activate a texture unit for this texture
   */
  void ActivateTexture(vtkTextureObject*);

  /**
   * Deactivate a previously activated texture
   */
  void DeactivateTexture(vtkTextureObject*);

  /**
   * Get the texture unit for a given texture object
   */
  int GetTextureUnitForTexture(vtkTextureObject*);

  /**
   * Check to make sure no textures have been left active
   */
  void VerifyNoActiveTextures();

  ///@{
  /**
   * Store/Restore the current framebuffer bindings and buffers.
   */
  void PushFramebufferBindings()
  {
    this->PushDrawFramebufferBinding();
    this->PushReadFramebufferBinding();
  }
  void PushDrawFramebufferBinding();
  void PushReadFramebufferBinding();

  void PopFramebufferBindings()
  {
    this->PopReadFramebufferBinding();
    this->PopDrawFramebufferBinding();
  }
  void PopDrawFramebufferBinding();
  void PopReadFramebufferBinding();

  void ResetFramebufferBindings();
  ///@}

  // Scoped classes you can use to save state
  class VTKRENDERINGOPENGL2_EXPORT ScopedglDepthMask : public ScopedValue<unsigned char>
  {
  public:
    ScopedglDepthMask(vtkOpenGLState* state);
  };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglClearColor : public ScopedValue<std::array<float, 4>>
  {
  public:
    ScopedglClearColor(vtkOpenGLState* state);
  };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglColorMask
    : public ScopedValue<std::array<unsigned char, 4>>
  {
  public:
    ScopedglColorMask(vtkOpenGLState* state);
  };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglScissor : public ScopedValue<std::array<int, 4>>
  {
  public:
    ScopedglScissor(vtkOpenGLState* state);
  };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglViewport : public ScopedValue<std::array<int, 4>>
  {
  public:
    ScopedglViewport(vtkOpenGLState* state);
  };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglBlendFuncSeparate
    : public ScopedValue<std::array<unsigned int, 4>>
  {
  public:
    ScopedglBlendFuncSeparate(vtkOpenGLState* state);
  };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglDepthFunc : public ScopedValue<unsigned int>
  {
  public:
    ScopedglDepthFunc(vtkOpenGLState* state);
  };
  class VTKRENDERINGOPENGL2_EXPORT ScopedglActiveTexture : public ScopedValue<unsigned int>
  {
  public:
    ScopedglActiveTexture(vtkOpenGLState* state);
  };

  class ScopedglEnableDisable
  {
  public:
    ScopedglEnableDisable(vtkOpenGLState* state, unsigned int name)
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
    vtkOpenGLState* State;
    unsigned int Name;
    bool Value;
  };

  /**
   * Initialize OpenGL context using current state
   */
  void Initialize(vtkOpenGLRenderWindow*);

  /**
   * Set the texture unit manager.
   */
  void SetTextureUnitManager(vtkTextureUnitManager* textureUnitManager);

  /**
   * Returns its texture unit manager object. A new one will be created if one
   * hasn't already been set up.
   */
  vtkTextureUnitManager* GetTextureUnitManager();

  // get the shader program cache for this context
  vtkGetObjectMacro(ShaderCache, vtkOpenGLShaderCache);

  // get the vbo buffer cache for this context
  vtkGetObjectMacro(VBOCache, vtkOpenGLVertexBufferObjectCache);

  // set the VBO Cache to use for this state
  // this allows two contexts to share VBOs
  // basically this is OPenGL's support for shared
  // lists
  void SetVBOCache(vtkOpenGLVertexBufferObjectCache* val);

  /**
   * Get a mapping of vtk data types to native texture formats for this window
   * we put this on the RenderWindow so that every texture does not have to
   * build these structures themselves
   */
  int GetDefaultTextureInternalFormat(
    int vtktype, int numComponents, bool needInteger, bool needFloat, bool needSRGB);

  /**
   * Get the current stored state of the draw buffer and binding
   */
  void GetCurrentDrawFramebufferState(unsigned int& drawBinding, unsigned int& drawBuffer);

  /**
   * Perform a blit but handle some driver bugs safely. Use this instead of directly calling
   * glBlitFramebuffer.
   */
  void vtkglBlitFramebuffer(int, int, int, int, int, int, int, int, unsigned int, unsigned int);

  /**
   * Record the OpenGL state into this class. Lots of get calls so probably
   * a pipeline stall. This method is most useful when integrating VTK with
   * something else that touches OpenGL such as a GUI library or external
   * OpenGL code. As OpenGL has a lot of state it is easy for VTK and
   * external libraries to interfere with each other by changing that state.
   * When extrnal code is calling VTK you would typically call Reset()
   * Push() Pop() Reset will record the current state from OpenGL. Push
   * saves it on the stack. Pop pops it from the stack and reapplies it to
   * OpenGL so that the state is the same as when Pushed. Note that OpenGL
   * has an incredible amount of state. This class only handles the values
   * that VTK is known to touch. If you find other values that need saving
   * please feel free to report an issue or provide an MR.
   */
  void Reset();

  /**
   * Push all the recorded state onto the stack. Typically called after a
   * Reset. Not generally used internally in VTK as it is rarely required to
   * save more than a couple state settings within VTKs render process.
   */
  void Push();

  /**
   *  Pop the state stack to restore a previous state. At the end of this
   *  method OpenGL state will be set to the new popped state.
   */
  void Pop();

  /**
   * Return the opengl version for this context
   */
  std::string const& GetVersion() { return this->Version; }

  /**
   * Return the opengl vendor for this context
   */
  std::string const& GetVendor() { return this->Vendor; }

  /**
   * Return the opengl renderer for this context. Note this is
   * the renderer opengl property, not a vtk renderer.
   */
  std::string const& GetRenderer() { return this->Renderer; }

protected:
  vtkOpenGLState(); // set initial values
  ~vtkOpenGLState() override;

  void BlendFuncSeparate(std::array<unsigned int, 4> val);
  void ClearColor(std::array<float, 4> val);
  void ColorMask(std::array<unsigned char, 4> val);
  void Scissor(std::array<int, 4> val);
  void Viewport(std::array<int, 4> val);

  int TextureInternalFormats[VTK_OBJECT + 1][3][5];
  void InitializeTextureInternalFormats();

  vtkTextureUnitManager* TextureUnitManager;
  std::map<const vtkTextureObject*, int> TextureResourceIds;

  /**
   * Check that this OpenGL state has consistent values
   * with the current OpenGL context
   */
  void CheckState();

  // framebuffers hold state themselves
  // specifically they hold their draw and read buffers
  // and when bound they reinstate those buffers
  class VTKRENDERINGOPENGL2_EXPORT BufferBindingState
  {
  public:
    BufferBindingState();
    unsigned int Binding;
    unsigned int ReadBuffer;
    unsigned int DrawBuffers[10];
    unsigned int GetBinding();
    unsigned int GetDrawBuffer(unsigned int);
    unsigned int GetReadBuffer();
  };
  std::list<BufferBindingState> DrawBindings;
  std::list<BufferBindingState> ReadBindings;

  // static opengl properties
  int MajorVersion;
  int MinorVersion;
  int MaxTessellationLevel;
  int MaxTextureSize;
  std::string Vendor;
  std::string Renderer;
  std::string Version;

  class VTKRENDERINGOPENGL2_EXPORT GLState
  {
  public:
    double ClearDepth;
    unsigned char DepthMask;
    unsigned int DepthFunc;
    unsigned int BlendEquationValue1;
    unsigned int BlendEquationValue2;
    unsigned int CullFaceMode;
    unsigned int ActiveTexture;

    float PointSize;
    float LineWidth;
    unsigned int StencilMaskFront;
    unsigned int StencilMaskBack;
    std::array<unsigned int, 3> StencilFuncFront;
    std::array<unsigned int, 3> StencilFuncBack;
    std::array<unsigned int, 3> StencilOpFront;
    std::array<unsigned int, 3> StencilOpBack;

    int PackAlignment;
    int UnpackAlignment;
    int UnpackRowLength;
    int UnpackImageHeight;

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
    bool CubeMapSeamless;
    bool LineSmooth;
    int BoundVAO;
    int BoundArrayBuffer;
    int BoundElementArrayBuffer;
    int BoundProgram;
    BufferBindingState DrawBinding;
    BufferBindingState ReadBinding;
    GLState() = default;
  };

  std::stack<GLState> Stack;

  vtkOpenGLVertexBufferObjectCache* VBOCache;
  vtkOpenGLShaderCache* ShaderCache;

private:
  vtkOpenGLState(const vtkOpenGLState&) = delete;
  void operator=(const vtkOpenGLState&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
