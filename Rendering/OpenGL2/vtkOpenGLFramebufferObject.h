/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLFramebufferObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLFramebufferObject
 * @brief   Internal class which encapsulates OpenGL FramebufferObject
 *
 * Before delving into this class it is best to have some background
 * in some OpenGL terms. OpenGL has a notion of a currently
 * bound Framebuffers for drawing and reading. It can be the default
 * framebuffer such as created with a standard window/context or
 * it can be a user created Framebuffer objects. When draw and read
 * commands are invoked, they apply to the current draw and/or read
 * frambuffers.
 *
 * A framebuffer consists of color buffers and an optional depth buffer.
 * The FramebufferObject does not hold the memory for these buffers, it
 * just keeps track of what buffers are attached to it. The buffers themselves
 * hold the storage for the pixels/depths.
 *
 * In the context of this discussion a buffer can be either a
 * vtkTextureObject (both 2D or a slice of a 3D texture) or
 * a vtkRenderbuffer. In some cases a renderbuffer may be faster
 * or more lightweight but you cannot pass a renderbuffer into
 * a shader for sampling in a later pass like you can a texture.
 *
 * You attach these buffers to the Framebuffer using methods
 * such as AddColorAttachment or AddDepthAttachment
 * In normal usage a buffer is Attached to a FramebufferObject
 * and then some or all of the attached buffers are activated for drawing
 * or reading.
 *
 * When you have a framebuffer bound along with some buffers attached to it
 * you can then activate specific buffers for drawing or reading. So you
 * have draw and read framebuffer objects (bindings) and then for the currently
 * bound FramebufferObjects you have active draw and read buffers.
 *
 * A single FramebufferObject can be bound to both Draw and Read. Likewise
 * a single buffer can be activated for both draw and read operations. You
 * cannot assign and activate a TextureObject for drawing on the FO and
 * at the same time pass it in as a Texture to the shader program. That
 * type of operation is very common and must be done in two steps.
 *  - Render to the FO with the Texture attached as an active buffer
 *  - deactivate the texture obj and then render with the texture obj
 *    as a texture passed into the shader
 *
 * Typical use cases:
 * The simplest example
 *\code{.cpp}
 * fbo->SetContext(renWin);
 * fbo->SaveCurrentBindingsAndBuffers();
 * fbo->PopulateFramebuffer(width, height);
 * fbo->Start();
 *
 * ...
 *
 * fbo->RestorePreviousBindingsAndBuffers();
 *\endcode
 *
 * If you wish to use a texture you created
 *
 *\code{.cpp}
 * fbo->SetContext(renWin);
 * fbo->SaveCurrentBindingsAndBuffers();
 * fbo->Bind();
 * fbo->AddColorAttachment(0, vtkTextureObj);
 * fbo->AddDepthAttachment(); // auto create depth buffer
 * fbo->ActivateBuffer(0);
 * fbo->Start();
 *
 * ...
 *
 * fbo->RestorePreviousBindingsAndBuffers();
 *\endcode
 *
 * If you will be using a FO repeatedly then it is best to create it
 * attach the buffers and then use as needed for example
 *
 * Typical use case:
 *\code{.cpp}
 * // setup the FBO once
 * fbo->SetContext(renWin);
 * fbo->SaveCurrentBindingsAndBuffers();
 * fbo->Bind();
 * fbo->AddColorAttachment(0, vtkTextureObj);
 * fbo->AddDepthAttachment(); // auto create depth buffer
 * fbo->RestorePreviousBindingsAndBuffers();
 *
 * // use it many times
 * fbo->SaveCurrentBindingsAndBuffers();
 * fbo->Bind();
 * fbo->ActivateBuffer(0);
 * fbo->Start();
 * ... // render here etc
 * fbo->RestorePreviousBindingsAndBuffers();
 *\endcode
 *
 * If you with to only bind/attach the draw buffers or read buffers there
 * are mode specific versions of most methods that only apply to the
 * mode specified Draw/Read/Both. The mode argument uses OpenGL constants
 * so this class provides convenience methods to return them named
 * GetDrawMode() GetReadMode() and GetBothMode() so that your code
 * does not need to be polluted with OpenGL headers/constants.
 *
 * This class replaces both vtkFrameBufferObject and vtkFrameBufferObject2
 * and contins methods from both of them. Most methods from FO2 should
 * work with this class. Just rename FBO2 to FBO and make sure to Save and
 * Restore the bindings and buffers.
 * If you have been using the old FO class, which had comments
 * in the header saying not to use it. Then you are in for a bit more of
 * a conversion but generally it should still be easy. Use the code
 * samples above (or any of the classes in OpenGL2 that currently use FBOs)
 * to guide you. They have all been converted to this class. Where previously
 * a DepthBuffer was automatically created for you, you now need to do it
 * explicitly using AddDepthAttachment().
 *
 * This class should be named vtkOpenGLFramebufferObject (FO)
 * Note the capitalization of FramebufferObject
 *
 * @sa
 * vtkTextureObject, vtkRenderbufferObject
*/

#ifndef vtkOpenGLFramebufferObject_h
#define vtkOpenGLFramebufferObject_h

/**
 * A variant of vtkErrorMacro that is used to verify framebuffer
 * object completeness. It's provided so that reporting may include
 * the file and line number of the offending code. In release mode
 * the macro does nothing.
 */
#ifdef NDEBUG
# define vtkCheckFrameBufferStatusMacro(mode)
# define vtkStaticCheckFrameBufferStatusMacro(mode)
#else
# define vtkCheckFrameBufferStatusMacroImpl(macro, mode)           \
{                                                                  \
const char *eStr;                                                  \
bool ok = vtkOpenGLFramebufferObject::GetFrameBufferStatus(mode, eStr); \
if (!ok)                                                           \
{                                                                \
  macro(                                                           \
    << "OpenGL ERROR. The FBO is incomplete : " << eStr);          \
}                                                                \
 }
# define vtkCheckFrameBufferStatusMacro(mode) \
    vtkCheckFrameBufferStatusMacroImpl(vtkErrorMacro, mode)
# define vtkStaticCheckFrameBufferStatusMacro(mode) \
    vtkCheckFrameBufferStatusMacroImpl(vtkGenericWarningMacro, mode)
#endif


#include "vtkFrameBufferObjectBase.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
#include <vector> // for the lists of logical buffers.
#include <map> // for the maps

class vtkFOInfo;
class vtkGenericOpenGLResourceFreeCallback;
class vtkOpenGLRenderWindow;
class vtkOpenGLVertexArrayObject;
class vtkPixelBufferObject;
class vtkRenderWindow;
class vtkRenderbuffer;
class vtkShaderProgram;
class vtkTextureObject;
class vtkWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLFramebufferObject : public vtkFrameBufferObjectBase
{
public:
  static vtkOpenGLFramebufferObject* New();
  vtkTypeMacro(vtkOpenGLFramebufferObject, vtkFrameBufferObjectBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the context. Context must be a vtkOpenGLRenderWindow.
   * This does not increase the reference count of the
   * context to avoid reference loops.
   * SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions.
   */
  void SetContext(vtkRenderWindow *context);
  vtkOpenGLRenderWindow *GetContext();
  //@}

  /**
   * Make the draw frame buffer active.
   */
  void Bind();
  void Bind(unsigned int mode);

  /**
   * Unbind this buffer
   */
  void UnBind();
  void UnBind(unsigned int mode);

  //@{
  /**
   * Store/Restore the current framebuffer bindings and buffers.
   */
  void SaveCurrentBindings();
  void SaveCurrentBindings(unsigned int mode);
  void SaveCurrentBindingsAndBuffers() {
    this->SaveCurrentBuffers();
    this->SaveCurrentBindings();
  }
  void SaveCurrentBindingsAndBuffers(unsigned int mode) {
    this->SaveCurrentBuffers(mode);
    this->SaveCurrentBindings(mode);
  }
  void RestorePreviousBindings();
  void RestorePreviousBindings(unsigned int mode);
  void RestorePreviousBindingsAndBuffers() {
    this->RestorePreviousBindings();
    this->RestorePreviousBuffers();
  }
  void RestorePreviousBindingsAndBuffers(unsigned int mode) {
    this->RestorePreviousBindings(mode);
    this->RestorePreviousBuffers(mode);
  }
  //@}

  //@{
  /**
   * Store the current draw and read buffers. When restored
   * only the buffers matching mode are modified.
   * GetDrawMode() -> glDrawBuffer
   * GetReadMode() -> glReadBuffer
   * GetBothMode() -> both
   */
  void SaveCurrentBuffers();
  void SaveCurrentBuffers(unsigned int mode);
  void RestorePreviousBuffers();
  void RestorePreviousBuffers(unsigned int mode);
  //@}

  //@{
  /**
   * User must take care that width/height match the dimensions of
   * the user defined texture attachments.
   * This method makes the "active buffers" the buffers that will get drawn
   * into by subsequent drawing calls.
   * Note that this does not clear the render buffers i.e. no glClear() calls
   * are made by either of these methods. It's up to the caller to clear the
   * buffers if needed.
   */
  bool Start(int width, int height);
  bool StartNonOrtho(int width, int height);
  //@}

  /**
   * Set up ortho viewport with scissor, lighting, blend, and depth
   * disabled. The method affects the current bound FBO.
   */
  void InitializeViewport(int width, int height);

  //@{
  // activate deactivate draw/read buffers (color buffers)
  void ActivateDrawBuffers(unsigned int n);
  void ActivateDrawBuffers(unsigned int *ids, int n);
  void ActivateDrawBuffer(unsigned int id);
  void ActivateReadBuffer(unsigned int id);
  void ActivateBuffer(unsigned int id) {
    this->ActivateDrawBuffer(id);
    this->ActivateReadBuffer(id); }
  void DeactivateDrawBuffers();
  void DeactivateReadBuffer();
  //@}

  /**
   * Renders a quad at the given location with pixel coordinates. This method
   * is provided as a convenience, since we often render quads in a FBO.
   * \pre positive_minX: minX>=0
   * \pre increasing_x: minX<=maxX
   * \pre valid_maxX: maxX<LastSize[0]
   * \pre positive_minY: minY>=0
   * \pre increasing_y: minY<=maxY
   * \pre valid_maxY: maxY<LastSize[1]
   */
  void RenderQuad(int minX, int maxX, int minY, int maxY,
    vtkShaderProgram *program, vtkOpenGLVertexArrayObject *vao);

  //@{
  /**
   * Directly assign/remove a texture to color attachments.
   * Same as the Set methods but also does an attach call
   * so the FO has to be bound when called.
   */
  void AddColorAttachment(
        unsigned int mode,
        unsigned int attId,
        vtkTextureObject* tex,
        unsigned int zslice = 0,
        unsigned int format = 0,
        unsigned int mipmapLevel = 0);
  void AddColorAttachment(
        unsigned int mode,
        unsigned int attId,
        vtkRenderbuffer* tex);
  void RemoveColorAttachment(unsigned int mode, unsigned int index);
  void RemoveColorAttachments(unsigned int mode, unsigned int num);
  //@}

  /**
   * Return the number of color attachments for the given mode
   */
  int GetNumberOfColorAttachments(unsigned int mode);

  //@{
  /**
   * Directly assign/remove a texture/renderbuffer to depth attachments.
   */
  void AddDepthAttachment() {
    this->AddDepthAttachment(this->GetBothMode()); }
  void AddDepthAttachment(unsigned int mode);
  void AddDepthAttachment(unsigned int mode, vtkTextureObject* tex);
  void AddDepthAttachment(unsigned int mode, vtkRenderbuffer* tex);
  void RemoveDepthAttachment(unsigned int mode);
  //@}

  //@{
  /**
   * Convenience method to populate a framebuffer with
   * attachments created as well. Returns true if a
   * complete valid Framebuffer was created
   */
  bool PopulateFramebuffer(int width, int height);
  bool PopulateFramebuffer(
    int width,
    int height,
    bool useTextures,
    int numberOfColorAttachments,
    int colorDataType,
    bool wantDepthAttachment,
    int depthBitplanes,
    int multisamples);
  //@}

  /**
   * Returns the maximum number of targets that can be rendered to at one time.
   * This limits the active targets set by SetActiveTargets().
   * The return value is valid only if GetContext is non-null.
   */
  unsigned int GetMaximumNumberOfActiveTargets();

  /**
   * Returns the maximum number of render targets available. This limits the
   * available attachment points for SetColorAttachment().
   * The return value is valid only if GetContext is non-null.
   */
  unsigned int GetMaximumNumberOfRenderTargets();

  //@{
  /**
   * Dimensions in pixels of the framebuffer.
   */
  int *GetLastSize() override
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning LastSize pointer " << this->LastSize);
    return this->LastSize;
  }
  void GetLastSize(int &_arg1, int &_arg2) override
  {
      _arg1 = this->LastSize[0];
      _arg2 = this->LastSize[1];
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning LastSize (" << _arg1 << "," << _arg2 << ")");
  }
  void GetLastSize (int _arg[2]) override
  {
    this->GetLastSize (_arg[0], _arg[1]);
  }
  //@}

  /**
   * Returns if the context supports the required extensions.
   * Extension will be loaded when the context is set.
   */
  static bool IsSupported(vtkOpenGLRenderWindow *) {
      return true; }

  /**
   * Validate the current FBO configuration (attachments, formats, etc)
   * prints detected errors to vtkErrorMacro.
   */
  int CheckFrameBufferStatus(unsigned int mode);

  /**
   * Deactivate and UnBind
   */
  virtual void ReleaseGraphicsResources(vtkWindow *win);

  /**
   * Validate the current FBO configuration (attachments, formats, etc)
   * return false if the FBO is incomplete. Assigns description a literal
   * containing a description of the status.
   * Low level api.
   */
  static
  bool GetFrameBufferStatus(
        unsigned int mode,
        const char *&desc);

    /**
   * Copy from the currently bound READ FBO to the currently
   * bound DRAW FBO. The method is static so that one doesn't
   * need to ccreate an instance when transferring between attachments
   * in the default FBO.
   */
  static int Blit(
    const int srcExt[4], const int destExt[4], unsigned int bits, unsigned int mapping);

  /**
   * Download data from the read color attachment of the currently
   * bound FBO into the returned PBO. The PBO must be free'd when
   * you are finished with it. The number of components in the
   * PBO is the same as in the name of the specific download function.
   * When downloading a single color channel, the channel must be
   * identified by index, 1->red, 2->green, 3-> blue.
   */
  vtkPixelBufferObject *DownloadColor1(
        int extent[4],
        int vtkType,
        int channel);

  vtkPixelBufferObject *DownloadColor3(
        int extent[4],
        int vtkType);

  vtkPixelBufferObject *DownloadColor4(
        int extent[4],
        int vtkType);

  /**
   * Download data from the depth attachment of the currently
   * bound FBO. The returned PBO must be Delete'd by the caller.
   * The returned PBO has one component.
   */
  vtkPixelBufferObject *DownloadDepth(
        int extent[4],
        int vtkType);

  /**
   * Download data from the read buffer of the current FBO. These
   * are low level meothds. In the static variant a PBO must be
   * passed in since we don't have access to a context. The static
   * method is provided so that one may download from the default
   * FBO.
   */
  vtkPixelBufferObject *Download(
        int extent[4],
        int vtkType,
        int nComps,
        int oglType,
        int oglFormat);

  static
  void Download(
        int extent[4],
        int vtkType,
        int nComps,
        int oglType,
        int oglFormat,
        vtkPixelBufferObject *pbo);

  // returns the mode values for draw/read/both
  // Can be used in cases where you do not
  // want to have OpenGL code mixed in.
  static unsigned int GetDrawMode();
  static unsigned int GetReadMode();
  static unsigned int GetBothMode();

  /**
   * Resize all FO attachments
   */
  void Resize(int width, int height);

  // Deprecated
  void RemoveTexColorAttachments(unsigned int mode, unsigned int num)
    { this->RemoveColorAttachments(mode, num); }
  void RemoveTexColorAttachment(unsigned int mode, unsigned int attId)
    { this->RemoveColorAttachment(mode, attId); }
  void RemoveRenDepthAttachment(unsigned int mode)
    { this->RemoveDepthAttachment(mode); }
  void RemoveTexDepthAttachment(unsigned int mode)
    { this->RemoveDepthAttachment(mode); }

protected:
  void SetColorBuffer(unsigned int mode,
    unsigned int index, vtkTextureObject *texture,
    unsigned int zslice=0, unsigned int format=0, unsigned int mipmapLevel=0);
  void SetColorBuffer(unsigned int mode,
    unsigned int index, vtkRenderbuffer *rb);
  void SetDepthBuffer(unsigned int mode, vtkTextureObject *depthTexture);
  void SetDepthBuffer(unsigned int mode, vtkRenderbuffer *depthBuffer);

  /**
   * Attach all buffers to the FO if not already done so
   */
  void Attach();

  /**
   * Attach a specific buffer
   */
  void AttachColorBuffer(unsigned int mode, unsigned int index);
  void AttachDepthBuffer(unsigned int mode);

  /**
   * Load all necessary extensions.
   */
  static
  bool LoadRequiredExtensions(vtkOpenGLRenderWindow *) {
    return true; };

  vtkGenericOpenGLResourceFreeCallback *ResourceCallback;

  // gen buffer (occurs when context is set)
  void CreateFBO();

  // delete buffer (occurs during destruction or context swicth)
  void DestroyFBO();

  // detach and delete our reference(s)
  void DestroyDepthBuffer(vtkWindow *win);
  void DestroyColorBuffers(vtkWindow *win);

  // glDrawBuffers
  void ActivateBuffers();

  // examine attachments to see if they have the same size
  void UpdateSize();

  /**
   * Display all the attachments of the current framebuffer object.
   */
  void DisplayFrameBufferAttachments();

  /**
   * Display a given attachment for the current framebuffer object.
   */
  void DisplayFrameBufferAttachment(unsigned int uattachment);

  /**
   * Display the draw buffers.
   */
  void DisplayDrawBuffers();

  /**
   * Display the read buffer.
   */
  void DisplayReadBuffer();

  /**
   * Display any buffer (convert value into string).
   */
  void DisplayBuffer(int value);

    /**
   * Given a vtk type get a compatible open gl type.
   */
  int GetOpenGLType(int vtkType);

  vtkOpenGLFramebufferObject();
  ~vtkOpenGLFramebufferObject() override;

  vtkWeakPointer<vtkOpenGLRenderWindow> Context;

  unsigned int FBOIndex;

  unsigned int PreviousDrawFBO;
  unsigned int PreviousReadFBO;
  bool DrawBindingSaved;
  bool ReadBindingSaved;
  unsigned int PreviousDrawBuffer;
  unsigned int PreviousReadBuffer;
  bool DrawBufferSaved;
  bool ReadBufferSaved;

  int LastSize[2];
  std::vector<unsigned int> ActiveBuffers;

  vtkFOInfo *DrawDepthBuffer;
  vtkFOInfo *ReadDepthBuffer;
  std::map<unsigned int, vtkFOInfo *> DrawColorBuffers;
  std::map<unsigned int, vtkFOInfo *> ReadColorBuffers;

private:
  vtkOpenGLFramebufferObject(const vtkOpenGLFramebufferObject&) = delete;
  void operator=(const vtkOpenGLFramebufferObject&) = delete;
};

#endif
