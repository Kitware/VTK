/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrameBufferObject2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFrameBufferObject2 - Interface to OpenGL framebuffer object.
// .SECTION Description
// A light and efficient interface to an OpenGL Frame Buffer Object.
// Use is very simillalry to directly calling OpenGL, but as vtkObject
// it may safely stored, shared, or passed around. It supports FBO Blit
// and transfer to Pixel Buffer Object.
//
// Typical use case:
//\code{.cpp}
// vtkFrameBufferObject2 *fbo = this->Internals->FBO;
// fbo->SaveCurrentBindings();
// fbo->Bind(vtkgl::FRAMEBUFFER_EXT);
// fbo->AddDepthAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, depthBuffer);
// fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U, colorTex1);
// fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 1U, colorTex2);
// fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 2U, colorTex3);
// fbo->ActivateDrawBuffers(3);
// vtkCheckFrameBufferStatusMacro(vtkgl::FRAMEBUFFER_EXT);
//
// ...
//
// fbo->UnBind(vtkgl::FRAMEBUFFER_EXT);
//\endcode
//
// .SECTION See Also
// vtkRenderbuffer, vtkPixelBufferObject

#ifndef vtkFrameBufferObject2_h
#define vtkFrameBufferObject2_h

#include "vtkObject.h"
#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

// Description:
// A variant of vtkErrorMacro that is used to verify framebuffer
// object completeness. It's provided so that reporting may include
// the file and line number of the offending code. In release mode
// the macro does nothing.
#ifdef NDEBUG
# define vtkCheckFrameBufferStatusMacro(mode)
# define vtkStaticCheckFrameBufferStatusMacro(mode)
#else
# define vtkCheckFrameBufferStatusMacroImpl(macro, mode)           \
{                                                                  \
const char *eStr;                                                  \
bool ok = vtkFrameBufferObject2::GetFrameBufferStatus(mode, eStr); \
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

class vtkRenderWindow;
class vtkTextureObject;
class vtkRenderbuffer;
class vtkPixelBufferObject;
class vtkOpenGLExtensionManager;
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL_EXPORT vtkFrameBufferObject2 : public vtkObject
{
public:
  static vtkFrameBufferObject2* New();
  vtkTypeMacro(vtkFrameBufferObject2, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the context. Context must be a vtkOpenGLRenderWindow.
  // This does not increase the reference count of the
  // context to avoid reference loops.
  // SetContext() may raise an error is the OpenGL context does not support the
  // required OpenGL extensions.
  void SetContext(vtkRenderWindow *context);
  vtkRenderWindow *GetContext();

  // Description:
  // Returns if the context supports the required extensions.
  // Extension will be loaded when the conetxt is set.
  static bool IsSupported(vtkRenderWindow *renWin);

  // Description:
  // Bind FBO to FRAMEBUFFER,  DRAW_FRAMEBUFFER or READ_FRAMEBUFFER
  // The current binding is not saved, nor restored. (see glBindFramebuffer)
  // This method can be used to prepare for FBO Blit or buffer ping-pong.
  // Low level api.
  void Bind(unsigned int mode);

  // Description:
  // Bind saved FBO (see SaveCurrentBindings) for DRAW or READ (see glBindFramebuffer)
  // If no bindings were saved bind to default FBO.
  // Low level api.
  void UnBind(unsigned int mode);

  // Description:
  // Store the current framebuffer bindings. If this method
  // is called then UnBind will restore the saved value accoring
  // to its mode (DRAW_FRAMEBUFFER,READ_FRAMEBUFFER,FRAMEBUFFER)
  // Restoration occurs in UnBind.
  // Low level api
  void SaveCurrentBindings();

  // Description:
  // Store the current draw and read buffers. When restored
  // only the buffers matching mode are modified.
  // DRAW_FRAMEBUFFER -> glDrawBuffer
  // READ_FRAMEBUFFER -> glReadBuffer
  // FRAMEBUFFER -> both
  void SaveCurrentBuffers();
  void RestorePreviousBuffers(unsigned int mode);

  // Description:
  // Directly assign/remove a texture to color attachments.
  void AddColorAttachment(
        unsigned int mode,
        unsigned int attId,
        vtkTextureObject* tex);

  void AddTexColorAttachment(
        unsigned int mode,
        unsigned int attId,
        unsigned int handle);

  void RemoveTexColorAttachments(unsigned int mode, unsigned int num);
  void RemoveTexColorAttachment(unsigned int mode, unsigned int attId)
    { this->AddTexColorAttachment(mode, attId, 0U); }

  // Description:
  // Directly assign/remove a renderbuffer to color attachments.
  void AddColorAttachment(
        unsigned int mode,
        unsigned int attId,
        vtkRenderbuffer* tex);

  void AddRenColorAttachment(
        unsigned int mode,
        unsigned int attId,
        unsigned int handle);

  void RemoveRenColorAttachments(unsigned int mode, unsigned int num);
  void RemoveRenColorAttachment(unsigned int mode, unsigned int attId)
    { this->AddRenColorAttachment(mode, attId, 0U); }

  // Description:
  // Directly assign/remove a texture/renderbuffer to depth attachments.
  void AddDepthAttachment(unsigned int mode, vtkTextureObject* tex);
  void AddTexDepthAttachment(unsigned int mode, unsigned int handle);
  void RemoveTexDepthAttachment(unsigned int mode)
    { this->AddTexDepthAttachment(mode, 0U); }

  // Description:
  // Directly assign/remove a renderbuffer to depth attachments.
  void AddDepthAttachment(unsigned int mode, vtkRenderbuffer* tex);
  void AddRenDepthAttachment(unsigned int mode, unsigned int handle);
  void RemoveRenDepthAttachment(unsigned int mode)
    { this->AddRenDepthAttachment(mode, 0U); }

  // Description:
  // Select a single specific draw or read buffer (zero based)
  void ActivateDrawBuffer(unsigned int id);
  void ActivateReadBuffer(unsigned int id);
  void DeactivateReadBuffer();

  // Description:
  // Select n consecutive write attachments.
  // Low level api.
  void ActivateDrawBuffers(unsigned int n);
  void ActivateDrawBuffers(unsigned int *ids, int n);
  void DeactivateDrawBuffers();

  // Description:
  // Set up ortho viewport with scissor, lighting, blend, and depth
  // disabled. The method affects the current bound FBO. The method is
  // static so that it may be used on the default FBO without an instance.
  // Low level api.
  static
  void InitializeViewport(int width, int height);

  // Description:
  // Validate the current FBO configuration (attachments, formats, etc)
  // prints detected errors to vtkErrorMacro.
  // Low level api.
  int CheckFrameBufferStatus(unsigned int mode);

  // Description:
  // Validate the current FBO configuration (attachments, formats, etc)
  // return false if the FBO is incomplete. Assigns description a literal
  // containing a description of the status.
  // Low level api.
  static
  bool GetFrameBufferStatus(
        unsigned int mode,
        const char *&desc);

  // Description:
  // Copy from the currently bound READ FBO to the currently
  // bound DRAW FBO. The method is static so that one doesn't
  // need to ccreate an instance when transfering between attachments
  // in the default FBO.
  static
  int Blit(
        int srcExt[4],
        int destExt[4],
        unsigned int bits,
        unsigned int mapping);

  // Description:
  // Download data from the read color attachment of the currently
  // bound FBO into the retruned PBO. The PBO must be free'd when
  // you are finished with it. The number of components in the
  // PBO is the same as in the name of the specific  download fucntion.
  // When downloading a single color channel, the channel must be
  // identified by index, 1->red, 2->green, 3-> blue.
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

  // Description:
  // Download data from the depth attachment of the currently
  // bound FBO. The returned PBO must be Delete'd by the caller.
  // The retruned PBO has one component.
  vtkPixelBufferObject *DownloadDepth(
        int extent[4],
        int vtkType);

  // Description:
  // Download data from the read buffer of the current FBO. These
  // are low level meothds. In the static variant a PBO must be
  // passed in since we don't have access to a context. The static
  // method is provided so that one may download from the default
  // FBO.
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

//BTX
protected:
  // Description:
  // Load all necessary extensions.
  static
  bool LoadRequiredExtensions(vtkRenderWindow *renWin);

  // gen buffer (occurs when context is set)
  void CreateFBO();

  // delete buffer (occurs during destruction or context swicth)
  void DestroyFBO();


  // Description:
  // Given a vtk type get a compatible open gl type.
  int GetOpenGLType(int vtkType);

  vtkFrameBufferObject2();
  ~vtkFrameBufferObject2();

  vtkWeakPointer<vtkRenderWindow> Context;

  unsigned int FBOIndex;
  unsigned int PreviousDrawFBO;
  unsigned int PreviousReadFBO;
  unsigned int DepthBuffer;
  unsigned int PreviousDrawBuffer;
  unsigned int PreviousReadBuffer;

private:
  vtkFrameBufferObject2(const vtkFrameBufferObject2&); // Not implemented.
  void operator=(const vtkFrameBufferObject2&); // Not implemented.

  friend class vtkRenderbuffer; // needs access to LoadRequiredExtentsions
//ETX
};

#endif
