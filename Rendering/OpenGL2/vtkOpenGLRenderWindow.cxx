/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderWindow.h"
#include "vtk_glew.h"

#include "vtkOpenGLHelper.h"

#include <cassert>

#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkOutputWindow.h"
#include "vtkPerlinNoise.h"
#include "vtkRenderTimerLog.h"
#include "vtkRendererCollection.h"
#include "vtkShaderProgram.h"
#include "vtkStdString.h"
#include "vtkStringOutputWindow.h"
#include "vtkTextureObject.h"
#include "vtkTextureUnitManager.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include "vtkTextureObjectVS.h" // a pass through shader

#include <sstream>
using std::ostringstream;

#include <cassert>

// Initialize static member that controls global maximum number of multisamples
// (off by default on Apple because it causes problems on some Mac models).
#if defined(__APPLE__)
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 0;
#else
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 8;
#endif

const char* defaultWindowName = "Visualization Toolkit - OpenGL";

namespace
{
// helper class to save/restore the framebuffer and draw/read buffer state.
// just create it on the stack with appropriate constructor arguments and it
// will restore the framebuffer/active buffers state in the destructor.
class FrameBufferHelper
{
public:
  enum EType
  {
    READ = 1,
    DRAW = 2
  };

  FrameBufferHelper(EType type, vtkOpenGLRenderWindow* rw, int, int)
    : Type(type)
  {
    this->State = rw->GetState();
    switch (type)
    {
      case READ:
      {
        this->State->PushReadFramebufferBinding();
        if (!rw->GetOffScreenFramebuffer()->GetFBOIndex())
        {
          vtkGenericWarningMacro("Error invoking helper with no framebuffer");
          return;
        }
        this->State->vtkBindFramebuffer(GL_READ_FRAMEBUFFER, rw->GetOffScreenFramebuffer());
        rw->GetOffScreenFramebuffer()->ActivateReadBuffer(0);
      }
      break;

      case DRAW:
      {
        this->State->PushDrawFramebufferBinding();
        if (!rw->GetOffScreenFramebuffer()->GetFBOIndex())
        {
          vtkGenericWarningMacro("Error invoking helper with no framebuffer");
          return;
        }
        this->State->vtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, rw->GetOffScreenFramebuffer());
        rw->GetOffScreenFramebuffer()->ActivateDrawBuffer(0);
      }
      break;

      default:
        assert(false);
    }
  }

  ~FrameBufferHelper()
  {
    switch (this->Type)
    {
      case READ:
      {
        this->State->PopReadFramebufferBinding();
      }
      break;

      case DRAW:
      {
        this->State->PopDrawFramebufferBinding();
      }
    }
  }

private:
  FrameBufferHelper(const FrameBufferHelper&) = delete;
  void operator=(const FrameBufferHelper&) = delete;

  EType Type;
  vtkOpenGLState* State;
};
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(int val)
{
  if (val == vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples)
  {
    return;
  }
  vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = val;
}

// ----------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples()
{
  return vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
}

//----------------------------------------------------------------------------
const char* vtkOpenGLRenderWindow::GetRenderingBackend()
{
  return "OpenGL2";
}

// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow::vtkOpenGLRenderWindow()
{
  this->State = vtkOpenGLState::New();

  this->Initialized = false;
  this->GlewInitValid = false;

  this->MultiSamples = vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
  delete[] this->WindowName;
  this->WindowName = new char[strlen(defaultWindowName) + 1];
  strcpy(this->WindowName, defaultWindowName);

  this->OffScreenFramebuffer = vtkOpenGLFramebufferObject::New();
  this->OffScreenFramebuffer->SetContext(this);

  this->BackLeftBuffer = static_cast<unsigned int>(GL_BACK_LEFT);
  this->BackRightBuffer = static_cast<unsigned int>(GL_BACK_RIGHT);
  this->FrontLeftBuffer = static_cast<unsigned int>(GL_FRONT_LEFT);
  this->FrontRightBuffer = static_cast<unsigned int>(GL_FRONT_RIGHT);
  this->DefaultFrameBufferId = 0;

  this->DrawPixelsTextureObject = nullptr;

  this->OwnContext = 1;
  this->MaximumHardwareLineWidth = 1.0;

  this->OpenGLSupportTested = false;
  this->OpenGLSupportResult = 0;
  this->OpenGLSupportMessage = "Not tested yet";

  // this->NumberOfFrameBuffers = 0;
  // this->DepthRenderBufferObject = 0;
  this->AlphaBitPlanes = 8;
  this->Capabilities = nullptr;

  this->TQuad2DVBO = nullptr;
  this->NoiseTextureObject = nullptr;
  this->FirstRenderTime = -1;
  this->LastMultiSamples = -1;

  this->ScreenSize[0] = 0;
  this->ScreenSize[1] = 0;
}

// free up memory & close the window
// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow::~vtkOpenGLRenderWindow()
{
  if (this->OffScreenFramebuffer)
  {
    this->OffScreenFramebuffer->Delete();
    this->OffScreenFramebuffer = nullptr;
  }

  if (this->DrawPixelsTextureObject != nullptr)
  {
    this->DrawPixelsTextureObject->UnRegister(this);
    this->DrawPixelsTextureObject = nullptr;
  }
  this->GLStateIntegers.clear();

  if (this->TQuad2DVBO)
  {
    this->TQuad2DVBO->Delete();
    this->TQuad2DVBO = nullptr;
  }

  if (this->NoiseTextureObject)
  {
    this->NoiseTextureObject->Delete();
  }

  delete[] this->Capabilities;
  this->Capabilities = nullptr;

  this->State->Delete();
}

//------------------------------------------------------------------------------
const char* vtkOpenGLRenderWindow::ReportCapabilities()
{
  this->MakeCurrent();

  const char* glVendor = (const char*)glGetString(GL_VENDOR);
  const char* glRenderer = (const char*)glGetString(GL_RENDERER);
  const char* glVersion = (const char*)glGetString(GL_VERSION);

  std::ostringstream strm;
  if (glVendor)
  {
    strm << "OpenGL vendor string:  " << glVendor << endl;
  }
  if (glRenderer)
  {
    strm << "OpenGL renderer string:  " << glRenderer << endl;
  }
  if (glVersion)
  {
    strm << "OpenGL version string:  " << glVersion << endl;
  }

  strm << "OpenGL extensions:  " << endl;
  GLint n, i;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n);
  for (i = 0; i < n; i++)
  {
    const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
    strm << "  " << ext << endl;
  }

  delete[] this->Capabilities;

  size_t len = strm.str().length() + 1;
  this->Capabilities = new char[len];
  strncpy(this->Capabilities, strm.str().c_str(), len);

  return this->Capabilities;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::ReleaseGraphicsResources(vtkWindow* renWin)
{
  this->PushContext();

  this->OffScreenFramebuffer->ReleaseGraphicsResources(renWin);

  // release the registered resources
  if (this->NoiseTextureObject)
  {
    this->NoiseTextureObject->ReleaseGraphicsResources(this);
  }

  std::set<vtkGenericOpenGLResourceFreeCallback*>::iterator it = this->Resources.begin();
  while (it != this->Resources.end())
  {
    (*it)->Release();
    it = this->Resources.begin();
  }

  vtkCollectionSimpleIterator rsit;
  this->Renderers->InitTraversal(rsit);
  vtkRenderer* aren;
  while ((aren = this->Renderers->GetNextRenderer(rsit)))
  {
    if (aren->GetRenderWindow() == this)
    {
      aren->ReleaseGraphicsResources(renWin);
    }
  }

  if (this->DrawPixelsTextureObject != nullptr)
  {
    this->DrawPixelsTextureObject->ReleaseGraphicsResources(renWin);
  }

  this->GetShaderCache()->ReleaseGraphicsResources(renWin);
  // this->VBOCache->ReleaseGraphicsResources(renWin);

  this->GetState()->VerifyNoActiveTextures();

  this->RenderTimer->ReleaseGraphicsResources();

  if (this->TQuad2DVBO)
  {
    this->TQuad2DVBO->ReleaseGraphicsResources();
  }

  this->PopContext();

  this->State->Delete();
  this->State = vtkOpenGLState::New();

  this->Initialized = false;
}

// ----------------------------------------------------------------------------
vtkMTimeType vtkOpenGLRenderWindow::GetContextCreationTime()
{
  return this->ContextCreationTime.GetMTime();
}

// ----------------------------------------------------------------------------
vtkOpenGLShaderCache* vtkOpenGLRenderWindow::GetShaderCache()
{
  return this->GetState()->GetShaderCache();
}

// ----------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectCache* vtkOpenGLRenderWindow::GetVBOCache()
{
  return this->GetState()->GetVBOCache();
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the back left buffer.
// It is GL_BACK_LEFT if GL is bound to the window-system-provided
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetBackLeftBuffer()
{
  return this->BackLeftBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the back right buffer.
// It is GL_BACK_RIGHT if GL is bound to the window-system-provided
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetBackRightBuffer()
{
  return this->BackRightBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the front left buffer.
// It is GL_FRONT_LEFT if GL is bound to the window-system-provided
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetFrontLeftBuffer()
{
  return this->FrontLeftBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the front right buffer.
// It is GL_FRONT_RIGHT if GL is bound to the window-system-provided
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetFrontRightBuffer()
{
  return this->FrontRightBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the back left buffer.
// It is GL_BACK if GL is bound to the window-system-provided
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetBackBuffer()
{
  return this->BackLeftBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the front left buffer.
// It is GL_FRONT if GL is bound to the window-system-provided
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetFrontBuffer()
{
  return this->FrontLeftBuffer;
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SetSize(int width, int height)
{
  if (this->Size[0] == width && this->Size[1] == height)
  {
    // Nothing should've happened in the superclass but one never knows...
    this->Superclass::SetSize(width, height);
    return;
  }

  this->Superclass::SetSize(width, height);
  if (this->UseOffScreenBuffers && this->OffScreenFramebuffer)
  {
    // resize the framebuffer
    this->OffScreenFramebuffer->Resize(width, height);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::OpenGLInit()
{
  this->OpenGLInitContext();
  if (this->Initialized)
  {
    this->OpenGLInitState();

    // This is required for some reason when using vtkSynchronizedRenderers.
    // Without it, the initial render of an offscreen context will always be
    // empty:
    glFlush();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::OpenGLInitState()
{
  this->GetState()->Initialize(this);

#ifdef GL_FRAMEBUFFER_SRGB
  if (this->UseSRGBColorSpace && this->GetUsingSRGBColorSpace())
  {
    glEnable(GL_FRAMEBUFFER_SRGB);
  }
#endif

  // Default OpenGL is 4 bytes but it is only safe with RGBA format.
  // If format is RGB, row alignment is 4 bytes only if the width is divisible
  // by 4. Let's do it the safe way: 1-byte alignment.
  // If an algorithm really need 4 bytes alignment, it should set it itself,
  // this is the recommended way in "Avoiding 16 Common OpenGL Pitfalls",
  // section 7:
  // http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
#ifdef GL_UNPACK_ALIGNMENT
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#endif
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  // Set the number of alpha bit planes used by the window
  int rgba[4];
  this->GetColorBufferSizes(rgba);
  this->SetAlphaBitPlanes(rgba[3]);
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetDefaultTextureInternalFormat(
  int vtktype, int numComponents, bool needInt, bool needFloat, bool needSRGB)
{
  return this->GetState()->GetDefaultTextureInternalFormat(
    vtktype, numComponents, needInt, needFloat, needSRGB);
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::GetOpenGLVersion(int& major, int& minor)
{
  int glMajorVersion = 2;
  int glMinorVersion = 0;

  if (this->Initialized)
  {
    this->GetState()->vtkglGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    this->GetState()->vtkglGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
  }

  major = glMajorVersion;
  minor = glMinorVersion;
}

// ----------------------------------------------------------------------------
bool vtkOpenGLRenderWindow::InitializeFromCurrentContext()
{
  int frameBufferBinding = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &frameBufferBinding);
  if (frameBufferBinding == 0)
  {
    this->DefaultFrameBufferId = 0;
    this->BackLeftBuffer = static_cast<unsigned int>(GL_BACK_LEFT);
    this->BackRightBuffer = static_cast<unsigned int>(GL_BACK_RIGHT);
    this->FrontLeftBuffer = static_cast<unsigned int>(GL_FRONT_LEFT);
    this->FrontRightBuffer = static_cast<unsigned int>(GL_FRONT_RIGHT);
  }
  else
  {
    this->DefaultFrameBufferId = frameBufferBinding;
    GLint attachment = GL_COLOR_ATTACHMENT0;
#ifdef GL_DRAW_BUFFER
    glGetIntegerv(GL_DRAW_BUFFER, &attachment);
#endif
    this->BackLeftBuffer = static_cast<unsigned int>(attachment);
    this->FrontLeftBuffer = static_cast<unsigned int>(attachment);
    // How to setup BackRightBuffer/FrontRightBuffer correctly? Should we assume
    // GL_COLOR_ATTACHMENT0+1? For now leaving them unchanged.
    //{
    //  buffer = static_cast<unsigned int>(GL_COLOR_ATTACHMENT0+1);
    //  this->BackRightBuffer = buffer;
    //  this->FrontRightBuffer = buffer;
    //}
  }

  this->OpenGLInit();
  this->OwnContext = 0;
  return true;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::OpenGLInitContext()
{
  this->ContextCreationTime.Modified();

  // When a new OpenGL context is created, force an update
  if (!this->Initialized)
  {
#ifdef GLEW_OK
    GLenum result = glewInit();
    this->GlewInitValid = (result == GLEW_OK);
    if (!this->GlewInitValid)
    {
      const char* errorMsg = reinterpret_cast<const char*>(glewGetErrorString(result));
      vtkErrorMacro("GLEW could not be initialized: " << errorMsg);
      return;
    }

    if (!GLEW_VERSION_3_2 && !GLEW_VERSION_3_1)
    {
      vtkErrorMacro("Unable to find a valid OpenGL 3.2 or later implementation. "
                    "Please update your video card driver to the latest version. "
                    "If you are using Mesa please make sure you have version 11.2 or "
                    "later and make sure your driver in Mesa supports OpenGL 3.2 such "
                    "as llvmpipe or openswr. If you are on windows and using Microsoft "
                    "remote desktop note that it only supports OpenGL 3.2 with nvidia "
                    "quadro cards. You can use other remoting software such as nomachine "
                    "to avoid this issue.");
      return;
    }
#else
    // GLEW is not being used, so avoid false failure on GL checks later.
    this->GlewInitValid = true;
#endif
    this->Initialized = true;

    // get this system's supported maximum line width
    // we do it here and store it to avoid repeated glGet
    // calls when the result should not change
    GLfloat lineWidthRange[2];
    this->MaximumHardwareLineWidth = 1.0;
#if defined(GL_SMOOTH_LINE_WIDTH_RANGE) && defined(GL_ALIASED_LINE_WIDTH_RANGE)
    if (this->LineSmoothing)
    {
      glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, lineWidthRange);
      if (glGetError() == GL_NO_ERROR)
      {
        this->MaximumHardwareLineWidth = lineWidthRange[1];
      }
    }
    else
    {
      glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
      if (glGetError() == GL_NO_ERROR)
      {
        this->MaximumHardwareLineWidth = lineWidthRange[1];
      }
    }
#endif
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultFrameBufferId: " << this->DefaultFrameBufferId << endl;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetDepthBufferSize()
{
  GLint size;

  if (this->Initialized)
  {
    this->MakeCurrent();
    size = 0;
    GLint fboBind = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fboBind);

    if (fboBind == 0)
    {
      glGetFramebufferAttachmentParameteriv(
        GL_DRAW_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &size);
    }
    else
    {
      glGetFramebufferAttachmentParameteriv(
        GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &size);
    }
    return static_cast<int>(size);
  }
  else
  {
    vtkDebugMacro(<< "OpenGL is not initialized yet!");
    return 24;
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderWindow::GetUsingSRGBColorSpace()
{
  if (this->Initialized)
  {
    this->MakeCurrent();

    GLint attachment = GL_BACK_LEFT;
#ifdef GL_DRAW_BUFFER
    glGetIntegerv(GL_DRAW_BUFFER, &attachment);
#endif
    // GL seems odd with its handling of left/right.
    // if it says we are using GL_FRONT or GL_BACK
    // then convert those to GL_FRONT_LEFT and
    // GL_BACK_LEFT.
    if (attachment == GL_FRONT)
    {
      attachment = GL_FRONT_LEFT;
      // for hardware windows this query seems to not work
      // and they seem to almost always honor SRGB values so return
      // the setting the user requested
      return this->UseSRGBColorSpace;
    }
    if (attachment == GL_BACK)
    {
      attachment = GL_BACK_LEFT;
      // for hardware windows this query seems to not work
      // and they seem to almost always honor SRGB values so return
      // the setting the user requested
      return this->UseSRGBColorSpace;
    }
    GLint enc = GL_LINEAR;
    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &enc);
    if (glGetError() == GL_NO_ERROR)
    {
      return (enc == GL_SRGB);
    }
    vtkDebugMacro(<< "Error getting color encoding!");
    return false;
  }

  vtkDebugMacro(<< "OpenGL is not initialized yet!");
  return false;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetColorBufferSizes(int* rgba)
{
  GLint size;

  if (rgba == nullptr)
  {
    return 0;
  }
  rgba[0] = 0;
  rgba[1] = 0;
  rgba[2] = 0;
  rgba[3] = 0;

  if (this->Initialized)
  {
    this->MakeCurrent();
    GLint attachment = GL_BACK_LEFT;
#ifdef GL_DRAW_BUFFER
    glGetIntegerv(GL_DRAW_BUFFER, &attachment);
#endif
    // GL seems odd with its handling of left/right.
    // if it says we are using GL_FRONT or GL_BACK
    // then convert those to GL_FRONT_LEFT and
    // GL_BACK_LEFT.
    if (attachment == GL_FRONT)
    {
      attachment = GL_FRONT_LEFT;
    }
    if (attachment == GL_BACK)
    {
      attachment = GL_BACK_LEFT;
    }

    // make sure we clear any errors before we start
    // otherwise we may get incorrect results
    while (glGetError() != GL_NO_ERROR)
    {
    }

    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &size);
    if (glGetError() == GL_NO_ERROR)
    {
      rgba[0] = static_cast<int>(size);
    }
    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &size);
    if (glGetError() == GL_NO_ERROR)
    {
      rgba[1] = static_cast<int>(size);
    }
    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &size);
    if (glGetError() == GL_NO_ERROR)
    {
      rgba[2] = static_cast<int>(size);
    }
    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &size);
    if (glGetError() == GL_NO_ERROR)
    {
      rgba[3] = static_cast<int>(size);
    }
    return rgba[0] + rgba[1] + rgba[2] + rgba[3];
  }
  else
  {
    vtkDebugMacro(<< "Window is not mapped yet!");
    rgba[0] = 8;
    rgba[1] = 8;
    rgba[2] = 8;
    rgba[3] = 8;
    return 32;
  }
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetColorBufferInternalFormat(int attachmentPoint)
{
  int format = 0;

#ifndef GL_ES_VERSION_3_0
  if (GLEW_ARB_direct_state_access)
  {
    int type;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentPoint,
      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    if (type == GL_TEXTURE)
    {
      int texName;
      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentPoint,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &texName);

      glGetTextureLevelParameteriv(texName, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
    }
    else if (type == GL_RENDERBUFFER)
    {
      int rbName;
      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentPoint,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &rbName);

      glGetNamedRenderbufferParameteriv(rbName, GL_RENDERBUFFER_INTERNAL_FORMAT, &format);
    }
    vtkOpenGLClearErrorMacro();
  }
#endif

  return format;
}

//------------------------------------------------------------------------------
unsigned char* vtkOpenGLRenderWindow::GetPixelData(
  int x1, int y1, int x2, int y2, int front, int right)
{
  int y_low, y_hi;
  int x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  int width = (x_hi - x_low) + 1;
  int height = (y_hi - y_low) + 1;

  unsigned char* ucdata = new unsigned char[width * height * 3];
  vtkRecti rect(x_low, y_low, width, height);
  this->ReadPixels(rect, front, GL_RGB, GL_UNSIGNED_BYTE, ucdata, right);
  return ucdata;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetPixelData(
  int x1, int y1, int x2, int y2, int front, vtkUnsignedCharArray* data, int right)
{
  int y_low, y_hi;
  int x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  int width = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 3 * width * height;

  if (data->GetMaxId() + 1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(3);
    data->SetNumberOfValues(size);
  }

  vtkRecti rect(x_low, y_low, width, height);
  return this->ReadPixels(rect, front, GL_RGB, GL_UNSIGNED_BYTE, data->GetPointer(0), right);
}

//------------------------------------------------------------------------------
// does the current read buffer require resolving for reading pixels
bool vtkOpenGLRenderWindow::GetBufferNeedsResolving()
{
  if (this->OffScreenFramebuffer->GetMultiSamples())
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::ReadPixels(
  const vtkRecti& rect, int front, int glformat, int gltype, void* data, int right)
{
  // set the current window
  this->MakeCurrent();

  if (rect.GetWidth() < 0 || rect.GetHeight() < 0)
  {
    // invalid box
    return VTK_ERROR;
  }

  // Must clear previous errors first.
  while (glGetError() != GL_NO_ERROR)
  {
    ;
  }

  FrameBufferHelper helper(FrameBufferHelper::READ, this, front, right);

  // Let's determine if we're reading from an FBO.
  bool resolveMSAA = this->GetBufferNeedsResolving();

  this->GetState()->vtkglDisable(GL_SCISSOR_TEST);

  // Calling pack alignment ensures that we can grab the any size window
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  if (resolveMSAA)
  {
    vtkNew<vtkOpenGLFramebufferObject> resolvedFBO;
    resolvedFBO->SetContext(this);
    this->GetState()->PushFramebufferBindings();
    resolvedFBO->PopulateFramebuffer(rect.GetWidth(), rect.GetHeight(),
      /* useTextures = */ true,
      /* numberOfColorAttachments = */ 1,
      /* colorDataType = */ VTK_UNSIGNED_CHAR,
      /* wantDepthAttachment = */ false,
      /* depthBitplanes = */ 0,
      /* multisamples = */ 0);

    // PopulateFramebuffer changes active read/write buffer bindings,
    // hence we restore the read buffer bindings to read from the original
    // frame buffer.
    this->GetState()->PopReadFramebufferBinding();

    // Now blit to resolve the MSAA and get an anti-aliased rendering in
    // resolvedFBO.
    // Note: extents are (x-min, x-max, y-min, y-max).
    const int srcExtents[4] = { rect.GetLeft(), rect.GetRight(), rect.GetBottom(), rect.GetTop() };
    const int destExtents[4] = { 0, rect.GetWidth(), 0, rect.GetHeight() };
    vtkOpenGLFramebufferObject::Blit(srcExtents, destExtents, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Now make the resolvedFBO the read buffer and read from it.
    this->GetState()->PushReadFramebufferBinding();
    resolvedFBO->Bind(GL_READ_FRAMEBUFFER);
    resolvedFBO->ActivateReadBuffer(0);

    // read pixels from the resolvedFBO. Note, the resolvedFBO has different
    // dimensions than the render window, hence different read extents.
    glReadPixels(0, 0, rect.GetWidth(), rect.GetHeight(), glformat, gltype, data);

    // restore bindings and release the resolvedFBO.
    this->GetState()->PopFramebufferBindings();
  }
  else
  {
    glReadPixels(
      rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight(), glformat, gltype, data);
  }

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::End()
{
  this->GetState()->PopFramebufferBindings();
}

//------------------------------------------------------------------------------
// for crystal eyes in stereo we have to blit here as well
void vtkOpenGLRenderWindow::StereoMidpoint()
{
  this->Superclass::StereoMidpoint();
  if (this->StereoType == VTK_STEREO_CRYSTAL_EYES && !this->UseOffScreenBuffers)
  {
    this->GetState()->PushFramebufferBindings();
    this->OffScreenFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->GetState()->vtkglBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->DefaultFrameBufferId);
    this->GetState()->vtkglDrawBuffer(this->GetBackLeftBuffer());

    int* fbsize = this->OffScreenFramebuffer->GetLastSize();
    // recall Blit upper right corner is exclusive of the range
    const int srcExtents[4] = { 0, fbsize[0], 0, fbsize[1] };
    const int destExtents[4] = { 0, this->Size[0], 0, this->Size[1] };
    this->GetState()->vtkglViewport(0, 0, this->Size[0], this->Size[1]);
    this->GetState()->vtkglScissor(0, 0, this->Size[0], this->Size[1]);
    vtkOpenGLFramebufferObject::Blit(srcExtents, destExtents, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    this->GetState()->PopFramebufferBindings();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::Frame()
{
  if (!this->UseOffScreenBuffers)
  {
    this->GetState()->PushFramebufferBindings();
    this->OffScreenFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->GetState()->vtkglBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->DefaultFrameBufferId);
    if (this->StereoRender && this->StereoType == VTK_STEREO_CRYSTAL_EYES)
    {
      this->GetState()->vtkglDrawBuffer(this->GetBackRightBuffer());
    }
    else
    {
      this->GetState()->vtkglDrawBuffer(this->GetBackLeftBuffer());
    }

    int* fbsize = this->OffScreenFramebuffer->GetLastSize();
    // recall Blit upper right corner is exclusive of the range
    const int srcExtents[4] = { 0, fbsize[0], 0, fbsize[1] };
    const int destExtents[4] = { 0, this->Size[0], 0, this->Size[1] };
    this->GetState()->vtkglViewport(0, 0, this->Size[0], this->Size[1]);
    this->GetState()->vtkglScissor(0, 0, this->Size[0], this->Size[1]);
    vtkOpenGLFramebufferObject::Blit(srcExtents, destExtents, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    this->GetState()->PopFramebufferBindings();
  }
}

//------------------------------------------------------------------------------
// Begin the rendering process.
void vtkOpenGLRenderWindow::Start()
{
  if (!this->Initialized)
  {
    this->Initialize();
  }

  // set the current window
  this->MakeCurrent();

  if (!this->OwnContext)
  {
    // if the context doesn't belong to us, it's unreasonable to expect that the
    // OpenGL state we maintain is going to sync up between subsequent renders.
    // Hence, we need to reset it.
    this->GetState()->Initialize(this);
  }

  // creates or resizes the framebuffer
  this->Size[0] = (this->Size[0] > 0 ? this->Size[0] : 300);
  this->Size[1] = (this->Size[1] > 0 ? this->Size[1] : 300);
  this->CreateOffScreenFramebuffer(this->Size[0], this->Size[1]);

  // push and bind
  this->GetState()->PushFramebufferBindings();
  this->OffScreenFramebuffer->Bind();
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetPixelData(
  int x1, int y1, int x2, int y2, vtkUnsignedCharArray* data, int front, int right)
{
  int y_low, y_hi;
  int x_low, x_hi;

  if (y1 < y2)
  {

    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  int width = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 3 * width * height;

  if (data->GetMaxId() + 1 != size)
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }
  return this->SetPixelData(x1, y1, x2, y2, data->GetPointer(0), front, right);
}

//------------------------------------------------------------------------------
// draw (and stretch as needed) the data to the current viewport
void vtkOpenGLRenderWindow::DrawPixels(
  int srcWidth, int srcHeight, int numComponents, int dataType, void* data)
{
  this->GetState()->vtkglDisable(GL_SCISSOR_TEST);
  this->GetState()->vtkglDisable(GL_DEPTH_TEST);
  if (!this->DrawPixelsTextureObject)
  {
    this->DrawPixelsTextureObject = vtkTextureObject::New();
  }
  else
  {
    this->DrawPixelsTextureObject->ReleaseGraphicsResources(this);
  }
  this->DrawPixelsTextureObject->SetContext(this);
  this->DrawPixelsTextureObject->Create2DFromRaw(
    srcWidth, srcHeight, numComponents, dataType, data);
  this->DrawPixelsTextureObject->CopyToFrameBuffer(nullptr, nullptr);
}

//------------------------------------------------------------------------------
// very generic call to draw pixel data to a region of the window
void vtkOpenGLRenderWindow::DrawPixels(int dstXmin, int dstYmin, int dstXmax, int dstYmax,
  int srcXmin, int srcYmin, int srcXmax, int srcYmax, int srcWidth, int srcHeight,
  int numComponents, int dataType, void* data)
{
  this->GetState()->vtkglDisable(GL_SCISSOR_TEST);
  this->GetState()->vtkglDisable(GL_DEPTH_TEST);
  if (!this->DrawPixelsTextureObject)
  {
    this->DrawPixelsTextureObject = vtkTextureObject::New();
  }
  else
  {
    this->DrawPixelsTextureObject->ReleaseGraphicsResources(this);
  }
  this->DrawPixelsTextureObject->SetContext(this);
  this->DrawPixelsTextureObject->Create2DFromRaw(
    srcWidth, srcHeight, numComponents, dataType, data);
  this->DrawPixelsTextureObject->CopyToFrameBuffer(srcXmin, srcYmin, srcXmax, srcYmax, dstXmin,
    dstYmin, dstXmax, dstYmax, this->GetSize()[0], this->GetSize()[1], nullptr, nullptr);
}

//------------------------------------------------------------------------------
// less generic version, old API
void vtkOpenGLRenderWindow::DrawPixels(
  int x1, int y1, int x2, int y2, int numComponents, int dataType, void* data)
{
  int y_low, y_hi;
  int x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  int width = x_hi - x_low + 1;
  int height = y_hi - y_low + 1;

  // call the more generic version
  this->DrawPixels(x_low, y_low, x_hi, y_hi, 0, 0, width - 1, height - 1, width, height,
    numComponents, dataType, data);
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetPixelData(
  int x1, int y1, int x2, int y2, unsigned char* data, int front, int right)
{
  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while (glGetError() != GL_NO_ERROR)
  {
    ;
  }

  FrameBufferHelper helper(FrameBufferHelper::DRAW, this, front, right);

  this->DrawPixels(x1, y1, x2, y2, 3, VTK_UNSIGNED_CHAR, data);

  // This seems to be necessary for the image to show up
  if (front)
  {
    glFlush();
  }

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

//------------------------------------------------------------------------------
float* vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2, int front, int right)
{

  int y_low, y_hi;
  int x_low, x_hi;
  int width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  width = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  float* fdata = new float[(width * height * 4)];
  vtkRecti rect(x_low, y_low, width, height);
  this->ReadPixels(rect, front, GL_RGBA, GL_FLOAT, fdata, right);
  return fdata;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetRGBAPixelData(
  int x1, int y1, int x2, int y2, int front, vtkFloatArray* data, int right)
{
  int y_low, y_hi;
  int x_low, x_hi;
  int width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  width = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;
  int size = 4 * width * height;
  if (data->GetMaxId() + 1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
  }

  vtkRecti rect(x_low, y_low, width, height);
  return this->ReadPixels(rect, front, GL_RGBA, GL_FLOAT, data->GetPointer(0), right);
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::ReleaseRGBAPixelData(float* data)
{
  delete[] data;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetRGBAPixelData(
  int x1, int y1, int x2, int y2, vtkFloatArray* data, int front, int blend, int right)
{
  int y_low, y_hi;
  int x_low, x_hi;
  int width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  width = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  int size = 4 * width * height;
  if (data->GetMaxId() + 1 != size)
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }

  return this->SetRGBAPixelData(x1, y1, x2, y2, data->GetPointer(0), front, blend, right);
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetRGBAPixelData(
  int x1, int y1, int x2, int y2, float* data, int front, int blend, int right)
{
  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while (glGetError() != GL_NO_ERROR)
  {
    ;
  }

  FrameBufferHelper helper(FrameBufferHelper::DRAW, this, front, right);
  if (!blend)
  {
    this->GetState()->vtkglDisable(GL_BLEND);
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_FLOAT, data); // TODO replace dprecated function
    this->GetState()->vtkglEnable(GL_BLEND);
  }
  else
  {
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_FLOAT, data);
  }

  // This seems to be necessary for the image to show up
  if (front)
  {
    glFlush();
  }

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

//------------------------------------------------------------------------------
unsigned char* vtkOpenGLRenderWindow::GetRGBACharPixelData(
  int x1, int y1, int x2, int y2, int front, int right)
{
  int y_low, y_hi;
  int x_low, x_hi;
  int width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  width = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  unsigned char* ucdata = new unsigned char[(width * height) * 4];
  vtkRecti rect(x_low, y_low, width, height);
  this->ReadPixels(rect, front, GL_RGBA, GL_UNSIGNED_BYTE, ucdata, right);
  return ucdata;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetRGBACharPixelData(
  int x1, int y1, int x2, int y2, int front, vtkUnsignedCharArray* data, int right)
{
  int y_low, y_hi;
  int x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  int width = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 4 * width * height;

  if (data->GetMaxId() + 1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
  }

  vtkRecti rect(x_low, y_low, width, height);
  return this->ReadPixels(rect, front, GL_RGBA, GL_UNSIGNED_BYTE, data->GetPointer(0), right);
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetRGBACharPixelData(
  int x1, int y1, int x2, int y2, vtkUnsignedCharArray* data, int front, int blend, int right)
{
  int y_low, y_hi;
  int x_low, x_hi;
  int width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi = y2;
  }
  else
  {
    y_low = y2;
    y_hi = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi = x2;
  }
  else
  {
    x_low = x2;
    x_hi = x1;
  }

  width = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  int size = 4 * width * height;
  if (data->GetMaxId() + 1 != size)
  {
    vtkErrorMacro(
      "Buffer is of wrong size. It is " << data->GetMaxId() + 1 << ", it should be: " << size);
    return VTK_ERROR;
  }

  return this->SetRGBACharPixelData(x1, y1, x2, y2, data->GetPointer(0), front, blend, right);
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetRGBACharPixelData(
  int x1, int y1, int x2, int y2, unsigned char* data, int front, int blend, int right)
{
  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while (glGetError() != GL_NO_ERROR)
  {
    ;
  }

  FrameBufferHelper helper(FrameBufferHelper::DRAW, this, front, right);

  // Disable writing on the z-buffer.
  this->GetState()->vtkglDepthMask(GL_FALSE);
  this->GetState()->vtkglDisable(GL_DEPTH_TEST);

  if (!blend)
  {
    this->GetState()->vtkglDisable(GL_BLEND);
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_UNSIGNED_CHAR, data);
    this->GetState()->vtkglEnable(GL_BLEND);
  }
  else
  {
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_UNSIGNED_CHAR, data);
  }

  // Renenable writing on the z-buffer.
  this->GetState()->vtkglDepthMask(GL_TRUE);
  this->GetState()->vtkglEnable(GL_DEPTH_TEST);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetZbufferData(int x1, int y1, int x2, int y2, float* z_data)
{
  int y_low;
  int x_low;
  int width, height;

  // set the current window
  this->MakeCurrent();

  if (y1 < y2)
  {
    y_low = y1;
  }
  else
  {
    y_low = y2;
  }

  if (x1 < x2)
  {
    x_low = x1;
  }
  else
  {
    x_low = x2;
  }

  width = abs(x2 - x1) + 1;
  height = abs(y2 - y1) + 1;

  // Error checking
  // Must clear previous errors first.
  while (glGetError() != GL_NO_ERROR)
  {
    ;
  }

  FrameBufferHelper helper(FrameBufferHelper::READ, this, 0, 0);

  // Let's determine if we're reading from an FBO.
  bool resolveMSAA = this->GetBufferNeedsResolving();

  this->GetState()->vtkglDisable(GL_SCISSOR_TEST);

  // Calling pack alignment ensures that we can grab the any size window
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  if (resolveMSAA)
  {
    vtkRecti rect(x_low, y_low, width, height);

    vtkNew<vtkOpenGLFramebufferObject> resolvedFBO;
    resolvedFBO->SetContext(this);
    this->GetState()->PushFramebufferBindings();
    resolvedFBO->PopulateFramebuffer(width, height,
      /* useTextures = */ true,
      /* numberOfColorAttachments = */ 1,
      /* colorDataType = */ VTK_UNSIGNED_CHAR,
      /* wantDepthAttachment = */ true,
      /* depthBitplanes = */ 32,
      /* multisamples = */ 0);

    // PopulateFramebuffer changes active read/write buffer bindings,
    // hence we restore the read buffer bindings to read from the original
    // frame buffer.
    this->GetState()->PopReadFramebufferBinding();

    // Now blit to resolve the MSAA and get an anti-aliased rendering in
    // resolvedFBO.
    // Note: extents are (x-min, x-max, y-min, y-max).
    const int srcExtents[4] = { rect.GetLeft(), rect.GetRight(), rect.GetBottom(), rect.GetTop() };
    const int destExtents[4] = { 0, rect.GetWidth(), 0, rect.GetHeight() };
    vtkOpenGLFramebufferObject::Blit(srcExtents, destExtents, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // Now make the resolvedFBO the read buffer and read from it.
    this->GetState()->PushReadFramebufferBinding();
    resolvedFBO->Bind(GL_READ_FRAMEBUFFER);
    resolvedFBO->ActivateReadBuffer(0);

    // read pixels from the resolvedFBO. Note, the resolvedFBO has different
    // dimensions than the render window, hence different read extents.
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, z_data);

    // restore bindings and release the resolvedFBO.
    this->GetState()->PopFramebufferBindings();
  }
  else
  {
    glReadPixels(x_low, y_low, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, z_data);
  }

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

//------------------------------------------------------------------------------
float* vtkOpenGLRenderWindow::GetZbufferData(int x1, int y1, int x2, int y2)
{
  float* z_data;

  int width, height;
  width = abs(x2 - x1) + 1;
  height = abs(y2 - y1) + 1;

  z_data = new float[width * height];
  this->GetZbufferData(x1, y1, x2, y2, z_data);

  return z_data;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer)
{
  int width, height;
  width = abs(x2 - x1) + 1;
  height = abs(y2 - y1) + 1;
  int size = width * height;
  if (buffer->GetMaxId() + 1 != size)
  {
    vtkDebugMacro("Resizing array.");
    buffer->SetNumberOfComponents(1);
    buffer->SetNumberOfValues(size);
  }
  return this->GetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer)
{
  int width, height;
  width = abs(x2 - x1) + 1;
  height = abs(y2 - y1) + 1;
  int size = width * height;
  if (buffer->GetMaxId() + 1 != size)
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }
  return this->SetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetZbufferData(int x1, int y1, int x2, int y2, float* buffer)
{
  vtkOpenGLState* ostate = this->GetState();
  ostate->vtkglDisable(GL_SCISSOR_TEST);
  ostate->vtkglEnable(GL_DEPTH_TEST);
  ostate->vtkglDepthFunc(GL_ALWAYS);
  ostate->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  if (!this->DrawPixelsTextureObject)
  {
    this->DrawPixelsTextureObject = vtkTextureObject::New();
  }
  else
  {
    this->DrawPixelsTextureObject->ReleaseGraphicsResources(this);
  }
  this->DrawPixelsTextureObject->SetContext(this);
  this->DrawPixelsTextureObject->CreateDepthFromRaw(
    x2 - x1 + 1, y2 - y1 + 1, vtkTextureObject::Float32, VTK_FLOAT, buffer);

  // compile and bind it if needed
  vtkShaderProgram* program = this->GetShaderCache()->ReadyShaderProgram(vtkTextureObjectVS,
    "//VTK::System::Dec\n"
    "in vec2 tcoordVC;\n"
    "uniform sampler2D source;\n"
    "//VTK::Output::Dec\n"
    "void main(void) {\n"
    "  gl_FragDepth = texture2D(source,tcoordVC).r; }\n",
    "");
  if (!program)
  {
    return VTK_ERROR;
  }
  vtkOpenGLVertexArrayObject* VAO = vtkOpenGLVertexArrayObject::New();

  FrameBufferHelper helper(FrameBufferHelper::DRAW, this, 0, 0);

  // bind and activate this texture
  this->DrawPixelsTextureObject->Activate();
  program->SetUniformi("source", this->DrawPixelsTextureObject->GetTextureUnit());

  this->DrawPixelsTextureObject->CopyToFrameBuffer(
    0, 0, x2 - x1, y2 - y1, x1, y1, x2, y2, this->GetSize()[0], this->GetSize()[1], program, VAO);
  this->DrawPixelsTextureObject->Deactivate();
  VAO->Delete();

  ostate->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  ostate->vtkglDepthFunc(GL_LEQUAL);

  return VTK_OK;
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::ActivateTexture(vtkTextureObject* texture)
{
  this->GetState()->ActivateTexture(texture);
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::DeactivateTexture(vtkTextureObject* texture)
{
  this->GetState()->DeactivateTexture(texture);
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetTextureUnitForTexture(vtkTextureObject* texture)
{
  return this->GetState()->GetTextureUnitForTexture(texture);
}

// ----------------------------------------------------------------------------
// Description:
// Create an offScreen window based on OpenGL framebuffer extension.
// Return if the creation was successful or not.
// \pre positive_width: width>0
// \pre positive_height: height>0
// \post valid_result: (result==0 || result==1)
int vtkOpenGLRenderWindow::CreateOffScreenFramebuffer(int width, int height)
{
  assert("pre: positive_width" && width > 0);
  assert("pre: positive_height" && height > 0);

#if defined(__APPLE__)
  // make sure requested multisamples is OK with platform
  // APPLE Intel systems seem to have buggy multisampled
  // frambuffer blits etc that cause issues
  if (this->MultiSamples > 0)
  {
    char const* tmp = reinterpret_cast<const char*>(::glGetString(GL_VENDOR));
    std::string vendor = tmp ? tmp : std::string();
    if (vendor.find("Intel") != std::string::npos)
    {
      this->MultiSamples = 0;
    }
  }
#endif

  if (this->LastMultiSamples != this->MultiSamples)
  {
    this->OffScreenFramebuffer->ReleaseGraphicsResources(this);
  }

  if (!this->OffScreenFramebuffer->GetFBOIndex())
  {
    // verify that our multisample setting doe snot exceed the hardware
    if (this->MultiSamples)
    {
#ifdef GL_MAX_SAMPLES
      int msamples = 0;
      this->GetState()->vtkglGetIntegerv(GL_MAX_SAMPLES, &msamples);
      if (this->MultiSamples > msamples)
      {
        this->MultiSamples = msamples;
      }
      if (this->MultiSamples == 1)
      {
        this->MultiSamples = 0;
      }
#else
      this->MultSamples = 0;
#endif
    }
    this->GetState()->PushFramebufferBindings();
    this->OffScreenFramebuffer->PopulateFramebuffer(width, height,
      true,                 // textures
      1, VTK_UNSIGNED_CHAR, // 1 color buffer uchar
      true, 32,             // depth buffer
      this->MultiSamples, this->StencilCapable != 0 ? true : false);
    this->LastMultiSamples = this->MultiSamples;
    this->GetState()->PopFramebufferBindings();
  }
  else
  {
    this->OffScreenFramebuffer->Resize(width, height);
  }

  return 1;
}

// ----------------------------------------------------------------------------
// Description:
// Returns its texture unit manager object. A new one will be created if one
// hasn't already been set up.
vtkTextureUnitManager* vtkOpenGLRenderWindow::GetTextureUnitManager()
{
  return this->GetState()->GetTextureUnitManager();
}

// ----------------------------------------------------------------------------
// Description:
// Block the thread until the actual rendering is finished().
// Useful for measurement only.
void vtkOpenGLRenderWindow::WaitForCompletion()
{
  glFinish();
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SaveGLState()
{
  // For now just query the active texture unit
  if (this->Initialized)
  {
    this->MakeCurrent();
    glGetIntegerv(GL_ACTIVE_TEXTURE, &this->GLStateIntegers["GL_ACTIVE_TEXTURE"]);

    if (this->GLStateIntegers["GL_ACTIVE_TEXTURE"] < 0 ||
      this->GLStateIntegers["GL_ACTIVE_TEXTURE"] >
        this->GetState()->GetTextureUnitManager()->GetNumberOfTextureUnits())
    {
      this->GLStateIntegers["GL_ACTIVE_TEXTURE"] = 0;
    }
  }
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::RestoreGLState()
{
  // Prevent making GL calls unless we have a valid context
  if (this->Initialized)
  {
    // For now just re-store the texture unit
    this->GetState()->vtkglActiveTexture(GL_TEXTURE0 + this->GLStateIntegers["GL_ACTIVE_TEXTURE"]);

    // Unuse active shader program
    this->GetShaderCache()->ReleaseCurrentShader();
  }
}

// ----------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SupportsOpenGL()
{
  if (this->OpenGLSupportTested)
  {
    return this->OpenGLSupportResult;
  }

  vtkOutputWindow* oldOW = vtkOutputWindow::GetInstance();
  oldOW->Register(this);
  vtkNew<vtkStringOutputWindow> sow;
  vtkOutputWindow::SetInstance(sow);

  vtkOpenGLRenderWindow* rw = this->NewInstance();
  rw->SetDisplayId(this->GetGenericDisplayId());
  rw->SetOffScreenRendering(1);
  rw->Initialize();
  if (rw->GlewInitValid == false)
  {
    this->OpenGLSupportMessage = "glewInit failed for this window, OpenGL not supported.";
    rw->Delete();
    vtkOutputWindow::SetInstance(oldOW);
    oldOW->Delete();
    return 0;
  }

#ifdef GLEW_OK

  else if (GLEW_VERSION_3_2 || GLEW_VERSION_3_1)
  {
    this->OpenGLSupportResult = 1;
    this->OpenGLSupportMessage = "The system appears to support OpenGL 3.2/3.1";
  }

#endif

  if (this->OpenGLSupportResult)
  {
    // even if glew thinks we have support we should actually try linking a
    // shader program to make sure
    vtkShaderProgram* newShader = rw->GetShaderCache()->ReadyShaderProgram(
      // simple vert shader
      "//VTK::System::Dec\n"
      "in vec4 vertexMC;\n"
      "void main() { gl_Position = vertexMC; }\n",
      // frag shader that used gl_PrimitiveId
      "//VTK::System::Dec\n"
      "//VTK::Output::Dec\n"
      "void main(void) {\n"
      "  gl_FragData[0] = vec4(float(gl_PrimitiveID)/100.0,1.0,1.0,1.0);\n"
      "}\n",
      // no geom shader
      "");
    if (newShader == nullptr)
    {
      this->OpenGLSupportResult = 0;
      this->OpenGLSupportMessage = "The system appeared to have OpenGL Support but a test shader "
                                   "program failed to compile and link";
    }
  }

  rw->Delete();

  this->OpenGLSupportMessage += "vtkOutputWindow Text Folows:\n\n" + sow->GetOutput();
  vtkOutputWindow::SetInstance(oldOW);
  oldOW->Delete();

  this->OpenGLSupportTested = true;

  return this->OpenGLSupportResult;
}

//------------------------------------------------------------------------------
vtkOpenGLBufferObject* vtkOpenGLRenderWindow::GetTQuad2DVBO()
{
  if (!this->TQuad2DVBO || !this->TQuad2DVBO->GetHandle())
  {
    if (!this->TQuad2DVBO)
    {
      this->TQuad2DVBO = vtkOpenGLBufferObject::New();
      this->TQuad2DVBO->SetType(vtkOpenGLBufferObject::ArrayBuffer);
    }
    float verts[16] = { 1.f, 1.f, 1.f, 1.f, -1.f, 1.f, 0.f, 1.f, 1.f, -1.f, 1.f, 0.f, -1.f, -1.f,
      0.f, 0.f };

    bool res = this->TQuad2DVBO->Upload(verts, 16, vtkOpenGLBufferObject::ArrayBuffer);
    if (!res)
    {
      vtkGenericWarningMacro("Error uploading fullscreen quad vertex data.");
    }
  }
  return this->TQuad2DVBO;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetNoiseTextureUnit()
{
  if (!this->NoiseTextureObject)
  {
    this->NoiseTextureObject = vtkTextureObject::New();
    this->NoiseTextureObject->SetContext(this);
  }

  if (this->NoiseTextureObject->GetHandle() == 0)
  {
    vtkNew<vtkPerlinNoise> generator;
    generator->SetFrequency(64, 64, 1.0);
    generator->SetAmplitude(0.5);

    int const bufferSize = 64 * 64;
    float* noiseTextureData = new float[bufferSize];
    for (int i = 0; i < bufferSize; i++)
    {
      int const x = i % 64;
      int const y = i / 64;
      noiseTextureData[i] = static_cast<float>(generator->EvaluateFunction(x, y, 0.0) + 0.5);
    }

    // Prepare texture
    this->NoiseTextureObject->Create2DFromRaw(64, 64, 1, VTK_FLOAT, noiseTextureData);

    this->NoiseTextureObject->SetWrapS(vtkTextureObject::Repeat);
    this->NoiseTextureObject->SetWrapT(vtkTextureObject::Repeat);
    this->NoiseTextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->NoiseTextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
    delete[] noiseTextureData;
  }

  int result = this->GetTextureUnitForTexture(this->NoiseTextureObject);

  if (result >= 0)
  {
    return result;
  }

  this->NoiseTextureObject->Activate();
  return this->GetTextureUnitForTexture(this->NoiseTextureObject);
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::Render()
{
  this->Superclass::Render();

  if (this->FirstRenderTime < 0)
  {
    this->FirstRenderTime = vtkTimerLog::GetUniversalTime();
  }
  this->GetShaderCache()->SetElapsedTime(vtkTimerLog::GetUniversalTime() - this->FirstRenderTime);

  if (this->NoiseTextureObject && this->GetTextureUnitForTexture(this->NoiseTextureObject) >= 0)
  {
    this->NoiseTextureObject->Deactivate();
  }
}
