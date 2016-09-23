/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrameBufferObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFrameBufferObject
 * @brief   internal class which encapsulates OpenGL
 * frame buffer object. Not to be used directly.
 *
 * Encapsulates an OpenGL Frame Buffer Object.
 * For use by vtkOpenGLFBORenderWindow, not to be used directly.
 * Use vtkFrameBufferObject2 instead.
 * @warning
 * DON'T PLAY WITH IT YET.
 * @sa
 * vtkFrameBufferObject2, vtkRenderbufferObject
*/

#ifndef vtkFrameBufferObject_h
#define vtkFrameBufferObject_h

#include "vtkFrameBufferObjectBase.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
#include <vector> // for the lists of logical buffers.


class vtkRenderWindow;
class vtkTextureObject;
class vtkPixelBufferObject;
class vtkOpenGLRenderWindow;
class vtkShaderProgram;
class vtkOpenGLVertexArrayObject;
class vtkGenericOpenGLResourceFreeCallback;
class vtkWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkFrameBufferObject : public vtkFrameBufferObjectBase
{
public:
  static vtkFrameBufferObject* New();
  vtkTypeMacro(vtkFrameBufferObject, vtkFrameBufferObjectBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get/Set the context. Context must be a vtkOpenGLRenderWindow.
   * This does not increase the reference count of the
   * context to avoid reference loops.
   * SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions.
   */
  void SetContext(vtkOpenGLRenderWindow *context);
  vtkOpenGLRenderWindow *GetContext();
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
  bool Start(int width, int height, bool shaderSupportsTextureInt);
  bool StartNonOrtho(int width, int height, bool shaderSupportsTextureInt);
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

  /**
   * Make the draw frame buffer active (uses FRAMEBUFFER).
   */
  void Bind();

  /**
   * Restore the previous draw framebuffer if saved, else
   * bind the default buffer.
   */
  void UnBind();

  /**
   * Choose the buffers to render into.
   */
  void SetActiveBuffer(unsigned int index)
  {
    this->SetActiveBuffers(1, &index);
  }

  /**
   * User provided color buffers are attached by index
   * to color buffers. This command lets you select which
   * attachments are written to. See set color buffer.
   * This call overwrites what the previous list of active
   * buffers.
   */
  void SetActiveBuffers(int numbuffers, unsigned int indices[]);

  /**
   * Insert a color buffer into the list of available color buffers.
   * 0 to NumberOfRenderTargets of these are attached to color attachments
   * by index. See SetActiveBuffers to select them for writing.
   * All user specified texture objects must match the FBO dimensions
   * and must have been created by the time Start() gets called.
   * If texture is a 3D texture, zslice identifies the zslice that will be
   * attached to the color buffer.
   * .SECTION Caveat
   * Currently, 1D textures are not supported.
   */
  void SetColorBuffer(
        unsigned int index,
        vtkTextureObject *texture,
        unsigned int zslice=0);

  vtkTextureObject *GetColorBuffer(unsigned int index);
  void RemoveColorBuffer(unsigned int index);
  void RemoveAllColorBuffers();

  //@{
  /**
   * Set the texture to use as depth buffer.
   */
  void SetDepthBuffer(vtkTextureObject *depthTexture);
  void RemoveDepthBuffer();
  //@}

  //@{
  /**
   * If true, the frame buffer object will be initialized with a depth buffer.
   * Initial value is true.
   */
  vtkSetMacro(DepthBufferNeeded,bool);
  vtkGetMacro(DepthBufferNeeded,bool);
  //@}

  //@{
  /**
   * Set/Get the number of render targets to render into at once.
   * Textures (user supplied or generated internally) are attached
   * to color attachment 0 to NumberOfRenderTargets. You can use
   * SetActiveBuffer to specify which of these are actually written to.
   * If zero then all of the user provided color buffers are used.
   */
  void SetNumberOfRenderTargets(unsigned int);
  vtkGetMacro(NumberOfRenderTargets,unsigned int);
  //@}

  /**
   * Returns the maximum number of targets that can be rendered to at one time.
   * This limits the active targets set by SetActiveTargets().
   * The return value is valid only if GetContext is non-null.
   */
  unsigned int GetMaximumNumberOfActiveTargets();

  /**
   * Returns the maximum number of render targets available. This limits the
   * available attachement points for SetColorAttachment().
   * The return value is valid only if GetContext is non-null.
   */
  unsigned int GetMaximumNumberOfRenderTargets();

  //@{
  /**
   * Dimensions in pixels of the framebuffer.
   */
  vtkGetVector2Macro(LastSize,int);
  //@}

  /**
   * Returns if the context supports the required extensions.
   * Extension will be loaded when the conetxt is set.
   */
  static bool IsSupported(vtkOpenGLRenderWindow *renWin);

  /**
   * Validate the current FBO configuration (attachments, formats, etc)
   * prints detected errors to vtkErrorMacro.
   */
  int CheckFrameBufferStatus(unsigned int mode);

  /**
   * Deactivate and UnBind
   */
  virtual void ReleaseGraphicsResources(vtkWindow *win);

protected:
  /**
   * Load all necessary extensions.
   */
  static
  bool LoadRequiredExtensions(vtkOpenGLRenderWindow *renWin);

  vtkGenericOpenGLResourceFreeCallback *ResourceCallback;

  // gen buffer (occurs when context is set)
  void CreateFBO();

  // delete buffer (occurs during destruction or context swicth)
  void DestroyFBO();

  // create texture or renderbuffer and attach
  // if user provided a texture just use that
  // mode specifies DRAW or READ
  void CreateDepthBuffer(int width, int height, unsigned int mode);

  // create textures for each target and attach
  // if user provided textures use those, if the user
  // provides any then they need to provide all
  // mode specifies DRAW or READ
  void CreateColorBuffers(
        int width,
        int height,
        unsigned int mode,
        bool shaderSupportsTextureInt);

  // detach and delete our reference(s)
  void DestroyDepthBuffer();
  void DestroyColorBuffers();

  // glDrawBuffers
  void ActivateBuffers();

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

  vtkFrameBufferObject();
  ~vtkFrameBufferObject();

  vtkWeakPointer<vtkOpenGLRenderWindow> Context;

  bool DepthBufferNeeded;
  bool ColorBuffersDirty;
  unsigned int FBOIndex;
  int PreviousFBOIndex;
  unsigned int DepthBuffer;
  unsigned int NumberOfRenderTargets;
  int LastSize[2];
  std::vector<unsigned int> UserZSlices;
  std::vector<vtkSmartPointer<vtkTextureObject> > UserColorBuffers;
  std::vector<vtkSmartPointer<vtkTextureObject> > ColorBuffers;
  std::vector<unsigned int> ActiveBuffers;
  vtkSmartPointer<vtkTextureObject> UserDepthBuffer;
  bool DepthBufferDirty;

private:
  vtkFrameBufferObject(const vtkFrameBufferObject&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFrameBufferObject&) VTK_DELETE_FUNCTION;
};

#endif
