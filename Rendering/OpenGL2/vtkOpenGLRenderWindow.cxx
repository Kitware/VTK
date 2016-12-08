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
#include "vtk_glew.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkOpenGLHelper.h"

#include <cassert>

#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOutputWindow.h"
#include "vtkRendererCollection.h"
#include "vtkShaderProgram.h"
#include "vtkStdString.h"
#include "vtkStringOutputWindow.h"
#include "vtkTextureObject.h"
#include "vtkTextureUnitManager.h"
#include "vtkUnsignedCharArray.h"
#include "vtkOpenGLResourceFreeCallback.h"

#include "vtkTextureObjectVS.h"  // a pass through shader

#include <sstream>
using std::ostringstream;

vtkCxxSetObjectMacro(vtkOpenGLRenderWindow, TextureUnitManager, vtkTextureUnitManager);

// Initialize static member that controls global maximum number of multisamples
// (off by default on Apple because it causes problems on some Mac models).
#if defined(__APPLE__)
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 0;
#else
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 8;
#endif

const char* defaultWindowName = "Visualization Toolkit - OpenGL";

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

// used for OpenGL32 Support
static bool vtkOpenGLRenderWindowContextSupportsOpenGL32 = false;

bool vtkOpenGLRenderWindow::GetContextSupportsOpenGL32()
{
  return vtkOpenGLRenderWindowContextSupportsOpenGL32;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SetContextSupportsOpenGL32(bool val)
{
  if (val == vtkOpenGLRenderWindowContextSupportsOpenGL32)
  {
    return;
  }
  vtkOpenGLRenderWindowContextSupportsOpenGL32 = val;
}

//----------------------------------------------------------------------------
const char *vtkOpenGLRenderWindow::GetRenderingBackend()
{
  return "OpenGL2";
}

// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow::vtkOpenGLRenderWindow()
{
  this->Initialized = false;

  this->ShaderCache = vtkOpenGLShaderCache::New();

  this->TextureUnitManager = 0;

  this->MultiSamples = vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
  delete [] this->WindowName;
  this->WindowName = new char[strlen(defaultWindowName) + 1];
  strcpy(this->WindowName, defaultWindowName);

  this->OffScreenUseFrameBuffer = 0;
  this->FrameBufferObject = 0;
  this->HardwareBufferSize[0] = 0;
  this->HardwareBufferSize[1] = 0;
  this->HardwareOffScreenBuffersBind = false;

  this->BackLeftBuffer = static_cast<unsigned int>(GL_BACK_LEFT);
  this->BackRightBuffer = static_cast<unsigned int>(GL_BACK_RIGHT);
  this->FrontLeftBuffer = static_cast<unsigned int>(GL_FRONT_LEFT);
  this->FrontRightBuffer = static_cast<unsigned int>(GL_FRONT_RIGHT);
  this->BackBuffer = static_cast<unsigned int>(GL_BACK);
  this->FrontBuffer = static_cast<unsigned int>(GL_FRONT);

  #ifndef VTK_LEGACY_REMOVE
  this->LastGraphicError = static_cast<unsigned int>(GL_NO_ERROR);
  #endif

  this->DrawPixelsTextureObject = NULL;

  this->OwnContext = 1;
  this->MaximumHardwareLineWidth = 1.0;

  this->OpenGLSupportTested = false;
  this->OpenGLSupportResult = 0;
  this->OpenGLSupportMessage = "Not tested yet";

  this->NumberOfFrameBuffers = 0;
  this->DepthRenderBufferObject = 0;
}

// free up memory & close the window
// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow::~vtkOpenGLRenderWindow()
{
  if(this->DrawPixelsTextureObject != 0)
  {
    this->DrawPixelsTextureObject->UnRegister(this);
    this->DrawPixelsTextureObject = NULL;
  }
  this->TextureResourceIds.clear();
  if(this->TextureUnitManager!=0)
  {
    this->TextureUnitManager->SetContext(0);
  }

  this->SetTextureUnitManager(0);

  this->GLStateIntegers.clear();

  this->ShaderCache->UnRegister(this);
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::ReleaseGraphicsResources(vtkRenderWindow *renWin)
{
  // release the registered resources

  std::set<vtkGenericOpenGLResourceFreeCallback *>::iterator it
   = this->Resources.begin();
  while (it != this->Resources.end() )
  {
    (*it)->Release();
    it = this->Resources.begin();
  }

  vtkCollectionSimpleIterator rsit;
  this->Renderers->InitTraversal(rsit);
  vtkRenderer *aren;
  while ( (aren = this->Renderers->GetNextRenderer(rsit)) )
  {
    if (aren->GetRenderWindow() == this)
    {
      aren->ReleaseGraphicsResources(renWin);
    }
  }

  if(this->DrawPixelsTextureObject != 0)
  {
     this->DrawPixelsTextureObject->ReleaseGraphicsResources(renWin);
  }

  this->ShaderCache->ReleaseGraphicsResources(renWin);

  if (this->TextureResourceIds.size())
  {
    vtkErrorMacro("There are still active textures when there should not be.");
    typedef std::map<const vtkTextureObject *, int>::const_iterator TRIter;
    TRIter found = this->TextureResourceIds.begin();
    for ( ; found != this->TextureResourceIds.end(); found++)
    {
      vtkErrorMacro("Leaked for texture object: " << const_cast<vtkTextureObject *>(found->first));
    }
  }
  this->Initialized = false;
}

// ----------------------------------------------------------------------------
vtkMTimeType vtkOpenGLRenderWindow::GetContextCreationTime()
{
  return this->ContextCreationTime.GetMTime();
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
  return this->BackBuffer;
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
  return this->FrontBuffer;
}

// Update system if needed due to stereo rendering.
void vtkOpenGLRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
  {
    switch (this->StereoType)
    {
      case VTK_STEREO_CRYSTAL_EYES:
        // not clear this is supposed to be empty,
        // but it has been that way forever.
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_CHECKERBOARD:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        this->StereoStatus = 1;
        break;
    }
  }
  else if ((!this->StereoRender) && this->StereoStatus)
  {
    switch (this->StereoType)
    {
      case VTK_STEREO_CRYSTAL_EYES:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_CHECKERBOARD:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        this->StereoStatus = 0;
        break;
    }
  }
}

void vtkOpenGLRenderWindow::SetSize(int a[2])
{
  this->SetSize(a[0], a[1]);
}

void vtkOpenGLRenderWindow::SetSize(int x, int y)
{
  if (this->Size[0] == x
    && this->Size[1] == y)
  {
    // Nothing should happend in the superclass but never knows...
    this->Superclass::SetSize(x, y);
    return;
  }

  this->Superclass::SetSize(x, y);
  if (this->HardwareOffScreenBuffersBind)
  {
    // We activate the offscreen buffers again so they will be
    // recreated at the new window size
    this->SetUseOffScreenBuffers(true);
  }
}

void vtkOpenGLRenderWindow::OpenGLInit()
{
  OpenGLInitContext();
  OpenGLInitState();
}

void vtkOpenGLRenderWindow::OpenGLInitState()
{
  glDepthFunc( GL_LEQUAL );
  glEnable( GL_DEPTH_TEST );

  // initialize blending for transparency
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                      GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  if (this->LineSmoothing)
  {
#ifdef GL_LINE_SMOOTH
    glEnable(GL_LINE_SMOOTH);
#endif
  }
  else
  {
#ifdef GL_LINE_SMOOTH
    glDisable(GL_LINE_SMOOTH);
#endif
  }

  if (this->PolygonSmoothing)
  {
#ifdef GL_POLYGON_SMOOTH
    glEnable(GL_POLYGON_SMOOTH);
#endif
  }
  else
  {
#ifdef GL_POLYGON_SMOOTH
    glDisable(GL_POLYGON_SMOOTH);
#endif
  }

  // Default OpenGL is 4 bytes but it is only safe with RGBA format.
  // If format is RGB, row alignment is 4 bytes only if the width is divisible
  // by 4. Let's do it the safe way: 1-byte alignment.
  // If an algorithm really need 4 bytes alignment, it should set it itself,
  // this is the recommended way in "Avoiding 16 Common OpenGL Pitfalls",
  // section 7:
  // http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
#ifdef GL_UNPACK_ALIGNMENT
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
#endif
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  // Set the number of alpha bit planes used by the window
  int rgba[4];
  this->GetColorBufferSizes(rgba);
  this->SetAlphaBitPlanes(rgba[3]);

  this->InitializeTextureInternalFormats();
}

int vtkOpenGLRenderWindow::GetDefaultTextureInternalFormat(
  int vtktype, int numComponents,
  bool needInt, bool needFloat)
{
  // 0 = none
  // 1 = float
  // 2 = int
  if (vtktype >= VTK_UNICODE_STRING)
  {
    return 0;
  }
  if (needInt)
  {
    return this->TextureInternalFormats[vtktype][2][numComponents];
  }
  if (needFloat)
  {
    return this->TextureInternalFormats[vtktype][1][numComponents];
  }
  return this->TextureInternalFormats[vtktype][0][numComponents];
}

void vtkOpenGLRenderWindow::InitializeTextureInternalFormats()
{
  // 0 = none
  // 1 = float
  // 2 = int

  // initialize to zero
  for (int dtype = 0; dtype < VTK_UNICODE_STRING; dtype++)
  {
    for (int ctype = 0; ctype < 3; ctype++)
    {
      for (int comp = 0; comp <= 4; comp++)
      {
        this->TextureInternalFormats[dtype][ctype][comp] = 0;
      }
    }
  }

  this->TextureInternalFormats[VTK_VOID][0][1] = GL_DEPTH_COMPONENT;

#ifdef GL_R8
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][1] = GL_R8;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][2] = GL_RG8;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][3] = GL_RGB8;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][4] = GL_RGBA8;
#else
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][1] = GL_LUMINANCE;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][2] = GL_LUMINANCE_ALPHA;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][3] = GL_RGB;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][4] = GL_RGBA;
#endif

#ifdef GL_R16
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][0][1] = GL_R16;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][0][2] = GL_RG16;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][0][3] = GL_RGB16;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][0][4] = GL_RGBA16;
#endif

#ifdef GL_R8_SNORM
  this->TextureInternalFormats[VTK_SIGNED_CHAR][0][1] = GL_R8_SNORM;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][0][2] = GL_RG8_SNORM;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][0][3] = GL_RGB8_SNORM;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][0][4] = GL_RGBA8_SNORM;
#endif

#ifdef GL_R16_SNORM
  this->TextureInternalFormats[VTK_SHORT][0][1] = GL_R16_SNORM;
  this->TextureInternalFormats[VTK_SHORT][0][2] = GL_RG16_SNORM;
  this->TextureInternalFormats[VTK_SHORT][0][3] = GL_RGB16_SNORM;
  this->TextureInternalFormats[VTK_SHORT][0][4] = GL_RGBA16_SNORM;
#endif

  bool haveFloatTextures = false;
  bool haveIntTextures = false;
#if GL_ES_VERSION_2_0 != 1
  if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
  {
    haveFloatTextures = true;
    haveIntTextures = true;
  }
  else
  {
    haveFloatTextures= (glewIsSupported("GL_ARB_texture_float") != 0
     && glewIsSupported("GL_ARB_texture_rg") != 0);
    haveIntTextures= (glewIsSupported("GL_EXT_texture_integer") != 0);
  }
#else
  haveFloatTextures = true;
#if GL_ES_VERSION_3_0 == 1
  haveIntTextures = true;
#endif
#endif

  if (haveIntTextures)
  {
#ifdef GL_R8I
    this->TextureInternalFormats[VTK_SIGNED_CHAR][2][1] = GL_R8I;
    this->TextureInternalFormats[VTK_SIGNED_CHAR][2][2] = GL_RG8I;
    this->TextureInternalFormats[VTK_SIGNED_CHAR][2][3] = GL_RGB8I;
    this->TextureInternalFormats[VTK_SIGNED_CHAR][2][4] = GL_RGBA8I;
    this->TextureInternalFormats[VTK_UNSIGNED_CHAR][2][1] = GL_R8UI;
    this->TextureInternalFormats[VTK_UNSIGNED_CHAR][2][2] = GL_RG8UI;
    this->TextureInternalFormats[VTK_UNSIGNED_CHAR][2][3] = GL_RGB8UI;
    this->TextureInternalFormats[VTK_UNSIGNED_CHAR][2][4] = GL_RGBA8UI;

    this->TextureInternalFormats[VTK_SHORT][2][1] = GL_R16I;
    this->TextureInternalFormats[VTK_SHORT][2][2] = GL_RG16I;
    this->TextureInternalFormats[VTK_SHORT][2][3] = GL_RGB16I;
    this->TextureInternalFormats[VTK_SHORT][2][4] = GL_RGBA16I;
    this->TextureInternalFormats[VTK_UNSIGNED_SHORT][2][1] = GL_R16UI;
    this->TextureInternalFormats[VTK_UNSIGNED_SHORT][2][2] = GL_RG16UI;
    this->TextureInternalFormats[VTK_UNSIGNED_SHORT][2][3] = GL_RGB16UI;
    this->TextureInternalFormats[VTK_UNSIGNED_SHORT][2][4] = GL_RGBA16UI;

    this->TextureInternalFormats[VTK_INT][2][1] = GL_R32I;
    this->TextureInternalFormats[VTK_INT][2][2] = GL_RG32I;
    this->TextureInternalFormats[VTK_INT][2][3] = GL_RGB32I;
    this->TextureInternalFormats[VTK_INT][2][4] = GL_RGBA32I;
    this->TextureInternalFormats[VTK_UNSIGNED_INT][2][1] = GL_R32UI;
    this->TextureInternalFormats[VTK_UNSIGNED_INT][2][2] = GL_RG32UI;
    this->TextureInternalFormats[VTK_UNSIGNED_INT][2][3] = GL_RGB32UI;
    this->TextureInternalFormats[VTK_UNSIGNED_INT][2][4] = GL_RGBA32UI;
#endif
  }

  // on mesa we may not have float textures even though we think we do
  // this is due to Mesa being iompacted by a patent issue with SGI
#if GL_ES_VERSION_2_0 != 1
  if (haveFloatTextures)
  {
    const char *glVersion =
      reinterpret_cast<const char *>(glGetString(GL_VERSION));
    if (glVersion && strstr(glVersion,"Mesa") != NULL &&
        !GLEW_ARB_texture_float)
    {
      haveFloatTextures = false;
      // mesa without float support cannot even use
      // uchar textures with underlying float data
      // so pretty much anything with float data
      // is out of luck so return
      return;
    }
  }
#endif

  if (haveFloatTextures)
  {
#ifdef GL_R32F
    this->TextureInternalFormats[VTK_FLOAT][1][1] = GL_R32F;
    this->TextureInternalFormats[VTK_FLOAT][1][2] = GL_RG32F;
    this->TextureInternalFormats[VTK_FLOAT][1][3] = GL_RGB32F;
    this->TextureInternalFormats[VTK_FLOAT][1][4] = GL_RGBA32F;

    this->TextureInternalFormats[VTK_SHORT][1][1] = GL_R32F;
    this->TextureInternalFormats[VTK_SHORT][1][2] = GL_RG32F;
    this->TextureInternalFormats[VTK_SHORT][1][3] = GL_RGB32F;
    this->TextureInternalFormats[VTK_SHORT][1][4] = GL_RGBA32F;
#endif
  }
}

void vtkOpenGLRenderWindow::GetOpenGLVersion(int &major, int &minor)
{
  int glMajorVersion = 2;
  int glMinorVersion = 0;

  if (this->Initialized)
  {
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
    glGetIntegerv(GL_MAJOR_VERSION, & glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, & glMinorVersion);
#endif
  }

  major = glMajorVersion;
  minor = glMinorVersion;
}

void vtkOpenGLRenderWindow::OpenGLInitContext()
{
  this->ContextCreationTime.Modified();

  // When a new OpenGL context is created, force an update
  if (!this->Initialized)
  {
#ifdef GLEW_OK
    GLenum result = glewInit();
    bool m_valid = (result == GLEW_OK);
    if (!m_valid)
    {
      vtkErrorMacro("GLEW could not be initialized.");
      return;
    }

    if (!GLEW_VERSION_3_2 && !GLEW_VERSION_3_1)
    {
      if (!GLEW_EXT_gpu_shader4)
      {
        vtkErrorMacro("GL version 2.1 with the gpu_shader4 extension is not "
        "supported by your graphics driver but is required for the new "
        "OpenGL rendering backend. Please update your OpenGL driver. "
        "If you are using Mesa please make sure you have version 10.6.5 or "
        "later and make sure your driver in Mesa supports OpenGL 3.2.");
        return;
      }
      vtkWarningMacro(
        "VTK is designed to work with OpenGL version 3.2 but it appears "
        "it has been given a context that does not support 3.2. VTK will "
        "run in a compatibility mode designed to work with earlier versions "
        "of OpenGL but some features may not work.");
    }
    else
    {
      this->SetContextSupportsOpenGL32(true);
    }
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
      glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE,lineWidthRange);
      if (glGetError() == GL_NO_ERROR)
      {
        this->MaximumHardwareLineWidth = lineWidthRange[1];
      }
    }
    else
    {
      glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE,lineWidthRange);
      if (glGetError() == GL_NO_ERROR)
      {
        this->MaximumHardwareLineWidth = lineWidthRange[1];
      }
    }
#endif
  }
}

void vtkOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkOpenGLRenderWindow::GetDepthBufferSize()
{
  GLint size;

  if ( this->Mapped )
  {
    this->MakeCurrent();
    size = 0;
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
    if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
    {
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        GL_DEPTH,
        GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &size);
    }
    else
#endif
    {
      glGetIntegerv( GL_DEPTH_BITS, &size );
    }
    return static_cast<int>(size);
  }
  else
  {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
  }
}

int vtkOpenGLRenderWindow::GetColorBufferSizes(int *rgba)
{
  GLint size;

  if (rgba==NULL)
  {
    return 0;
  }
  rgba[0] = 0;
  rgba[1] = 0;
  rgba[2] = 0;
  rgba[3] = 0;

  if ( this->Mapped)
  {
    this->MakeCurrent();
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
    if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
    {
      GLint fboBind = 0;
      glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fboBind);
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
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        attachment,
        GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &size);
      if (glGetError() == GL_NO_ERROR)
      {
        rgba[0] = static_cast<int>(size);
      }
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        attachment,
        GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &size);
      if (glGetError() == GL_NO_ERROR)
      {
        rgba[1] = static_cast<int>(size);
      }
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        attachment,
        GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &size);
      if (glGetError() == GL_NO_ERROR)
      {
        rgba[2] = static_cast<int>(size);
      }
      glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        attachment,
        GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &size);
      if (glGetError() == GL_NO_ERROR)
      {
        rgba[3] = static_cast<int>(size);
      }
    }
    else
#endif
    {
      glGetIntegerv( GL_RED_BITS, &size );
      rgba[0] = static_cast<int>(size);
      glGetIntegerv( GL_GREEN_BITS, &size  );
      rgba[1] = static_cast<int>(size);
      glGetIntegerv( GL_BLUE_BITS, &size );
      rgba[2] = static_cast<int>(size);
      glGetIntegerv( GL_ALPHA_BITS, &size );
      rgba[3] = static_cast<int>(size);
    }
    return rgba[0]+rgba[1]+rgba[2]+rgba[3];
  }
  else
  {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    rgba[0] = 8;
    rgba[1] = 8;
    rgba[2] = 8;
    rgba[3] = 8;
    return 32;
  }
}

unsigned char* vtkOpenGLRenderWindow::GetPixelData(int x1, int y1,
                                                   int x2, int y2,
                                                   int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  unsigned char *data =
    new unsigned char[(x_hi - x_low + 1)*(y_hi - y_low + 1)*3];
  this->GetPixelData(x1, y1, x2, y2, front, data);
  return data;
}

int vtkOpenGLRenderWindow::GetPixelData(int x1, int y1,
                                        int x2, int y2,
                                        int front,
                                        vtkUnsignedCharArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 3*width*height;

  if ( data->GetMaxId()+1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(3);
    data->SetNumberOfValues(size);
  }
  return this->GetPixelData(x1, y1, x2, y2, front, data->GetPointer(0));

}

int vtkOpenGLRenderWindow::GetPixelData(int x1, int y1,
                                        int x2, int y2,
                                        int front, unsigned char* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  // set the current window
  this->MakeCurrent();

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  if (front)
  {
    glReadBuffer(static_cast<GLenum>(this->GetFrontLeftBuffer()));
  }
  else
  {
    glReadBuffer(static_cast<GLenum>(this->GetBackLeftBuffer()));
  }

  glDisable( GL_SCISSOR_TEST );

  // Calling pack alignment ensures that we can grab the any size window
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels(x_low, y_low, x_hi-x_low+1, y_hi-y_low+1, GL_RGB,
               GL_UNSIGNED_BYTE, data);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }

}

int vtkOpenGLRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
                                        vtkUnsignedCharArray *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {

    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 3*width*height;

  if ( data->GetMaxId()+1 != size)
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }
  return this->SetPixelData(x1, y1, x2, y2, data->GetPointer(0), front);

}


// draw (and stretch as needed) the data to the current viewport
void vtkOpenGLRenderWindow::DrawPixels(
  int srcWidth, int srcHeight, int numComponents, int dataType, void *data)
{
  glDisable( GL_SCISSOR_TEST );
  glDisable(GL_DEPTH_TEST);
  if (!this->DrawPixelsTextureObject)
  {
    this->DrawPixelsTextureObject = vtkTextureObject::New();
  }
  else
  {
    this->DrawPixelsTextureObject->ReleaseGraphicsResources(this);
  }
  this->DrawPixelsTextureObject->SetContext(this);
  this->DrawPixelsTextureObject->Create2DFromRaw(srcWidth, srcHeight,
        numComponents, dataType, data);
  this->DrawPixelsTextureObject->CopyToFrameBuffer(NULL, NULL);
}

// very generic call to draw pixel data to a region of the window
void vtkOpenGLRenderWindow::DrawPixels(
  int dstXmin, int dstYmin, int dstXmax, int dstYmax,
  int srcXmin, int srcYmin, int srcXmax, int srcYmax,
  int srcWidth, int srcHeight, int numComponents, int dataType, void *data)
{
  glDisable( GL_SCISSOR_TEST );
  glDisable(GL_DEPTH_TEST);
  if (!this->DrawPixelsTextureObject)
  {
    this->DrawPixelsTextureObject = vtkTextureObject::New();
  }
  else
  {
    this->DrawPixelsTextureObject->ReleaseGraphicsResources(this);
  }
  this->DrawPixelsTextureObject->SetContext(this);
  this->DrawPixelsTextureObject->Create2DFromRaw(srcWidth, srcHeight,
        numComponents, dataType, data);
  this->DrawPixelsTextureObject->CopyToFrameBuffer(
      srcXmin, srcYmin, srcXmax, srcYmax,
      dstXmin, dstYmin, dstXmax, dstYmax,
      this->GetSize()[0], this->GetSize()[1],
      NULL, NULL);
}

// less generic verison, old API
void vtkOpenGLRenderWindow::DrawPixels(int x1, int y1, int x2, int y2, int numComponents, int dataType, void *data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  int width = x_hi-x_low+1;
  int height = y_hi-y_low+1;

  // call the more generic version
  this->DrawPixels(x_low, y_low, x_hi, y_hi,
    0, 0, width-1, height-1, width, height, numComponents, dataType, data);
}

int vtkOpenGLRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
                                        unsigned char *data, int front)
{
  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  GLint buffer;

#ifdef GL_DRAW_BUFFER
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);
#endif

  if (front)
  {
    glDrawBuffer(this->GetFrontBuffer());
  }
  else
  {
    glDrawBuffer(this->GetBackBuffer());
  }

  this->DrawPixels(x1, y1, x2, y2, 3, VTK_UNSIGNED_CHAR, data);

  // This seems to be necessary for the image to show up
  if (front)
  {
    glFlush();
  }

  glDrawBuffer(buffer);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

float* vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                               int front)
{

  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  float *data = new float[ (width*height*4) ];
  this->GetRGBAPixelData(x1, y1, x2, y2, front, data);

  return data;

}

int vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            int front, vtkFloatArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  int size = 4*width*height;

  if ( data->GetMaxId()+1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
  }
  return this->GetRGBAPixelData(x1, y1, x2, y2, front, data->GetPointer(0));

}

int vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            int front, float* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  // set the current window
  this->MakeCurrent();

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  if (front)
  {
    glReadBuffer(static_cast<GLenum>(this->GetFrontLeftBuffer()));
  }
  else
  {
    glReadBuffer(static_cast<GLenum>(this->GetBackLeftBuffer()));
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;


  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_FLOAT, data);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

void vtkOpenGLRenderWindow::ReleaseRGBAPixelData(float *data)
{
  delete[] data;
}

int vtkOpenGLRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            vtkFloatArray *data, int front,
                                            int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  int size = 4*width*height;
  if ( data->GetMaxId()+1 != size )
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }

  return this->SetRGBAPixelData(x1, y1, x2, y2, data->GetPointer(0), front,
                                blend);
}

int vtkOpenGLRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            float *data, int front, int blend)
{
  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  GLint buffer;
#ifdef GL_DRAW_BUFFER
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);
#endif

  if (front)
  {
    glDrawBuffer(this->GetFrontBuffer());
  }
  else
  {
    glDrawBuffer(this->GetBackBuffer());
  }

  if (!blend)
  {
    glDisable(GL_BLEND);
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_FLOAT, data); // TODO replace dprecated function
    glEnable(GL_BLEND);
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

  glDrawBuffer(buffer);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

unsigned char *vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                                           int x2, int y2,
                                                           int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }


  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  unsigned char *data = new unsigned char[ (width*height)*4 ];
  this->GetRGBACharPixelData(x1, y1, x2, y2, front, data);

  return data;
}

int vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                                int x2, int y2,
                                                int front,
                                                vtkUnsignedCharArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 4*width*height;

  if ( data->GetMaxId()+1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
  }
  return this->GetRGBACharPixelData(x1, y1, x2, y2, front,
                                    data->GetPointer(0));
}

int vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                                int x2, int y2,
                                                int front,
                                                unsigned char* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;


  // set the current window
  this->MakeCurrent();


  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }


  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }


  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  if (front)
  {
    glReadBuffer(static_cast<GLenum>(this->GetFrontLeftBuffer()));
  }
  else
  {
    glReadBuffer(static_cast<GLenum>(this->GetBackLeftBuffer()));
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  glDisable( GL_SCISSOR_TEST );

  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                data);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }

}


int vtkOpenGLRenderWindow::SetRGBACharPixelData(int x1,int y1,int x2,int y2,
                                                vtkUnsignedCharArray *data,
                                                int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  int size = 4*width*height;
  if ( data->GetMaxId()+1 != size )
  {
    vtkErrorMacro("Buffer is of wrong size. It is " << data->GetMaxId()+1
                  << ", it should be: " << size);
    return VTK_ERROR;
  }

  return this->SetRGBACharPixelData(x1, y1, x2, y2, data->GetPointer(0),
                                    front, blend);

}

int vtkOpenGLRenderWindow::SetRGBACharPixelData(int x1, int y1, int x2,
                                                int y2, unsigned char *data,
                                                int front, int blend)
{
  // set the current window
  this->MakeCurrent();


  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  GLint buffer;
#ifdef GL_DRAW_BUFFER
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);
#endif

  if (front)
  {
    glDrawBuffer(this->GetFrontBuffer());
  }
  else
  {
    glDrawBuffer(this->GetBackBuffer());
  }


  // Disable writing on the z-buffer.
  glDepthMask(GL_FALSE);
  glDisable(GL_DEPTH_TEST);

  if (!blend)
  {
    glDisable(GL_BLEND);
    this->DrawPixels(x1,y1,x2,y2,4, VTK_UNSIGNED_CHAR, data);
    glEnable(GL_BLEND);
  }
  else
  {
    this->DrawPixels(x1,y1,x2,y2,4, VTK_UNSIGNED_CHAR, data);
  }

  // Renenable writing on the z-buffer.
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);

  glDrawBuffer(buffer);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}


int vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2,
                                           float* z_data )
{
  int             y_low;
  int             x_low;
  int             width, height;

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

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_SCISSOR_TEST );
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );

  glReadPixels( x_low, y_low,
                width, height,
                GL_DEPTH_COMPONENT, GL_FLOAT,
                z_data );

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

float *vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2  )
{
  float           *z_data;

  int             width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  z_data = new float[width*height];
  this->GetZbufferData(x1, y1, x2, y2, z_data);

  return z_data;
}

int vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2,
                                           vtkFloatArray *buffer )
{
  int  width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;
  int size = width*height;
  if ( buffer->GetMaxId()+1 != size)
  {
    vtkDebugMacro("Resizing array.");
    buffer->SetNumberOfComponents(1);
    buffer->SetNumberOfValues(size);
  }
  return this->GetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

int vtkOpenGLRenderWindow::SetZbufferData( int x1, int y1, int x2, int y2,
                                           vtkFloatArray *buffer )
{
  int width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;
  int size = width*height;
  if ( buffer->GetMaxId()+1 != size )
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }
  return this->SetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

int vtkOpenGLRenderWindow::SetZbufferData( int x1, int y1,
                                           int x2, int y2,
                                           float *buffer )
{
//  glDrawBuffer(this->GetBackBuffer());
  glDisable( GL_SCISSOR_TEST );
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_ALWAYS);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  if (!this->DrawPixelsTextureObject)
  {
    this->DrawPixelsTextureObject = vtkTextureObject::New();
  }
  else
  {
    this->DrawPixelsTextureObject->ReleaseGraphicsResources(this);
  }
  this->DrawPixelsTextureObject->SetContext(this);
  this->DrawPixelsTextureObject->CreateDepthFromRaw(x2-x1+1, y2-y1+1,
        vtkTextureObject::Float32, VTK_FLOAT, buffer);

  // compile and bind it if needed
  vtkShaderProgram *program =
    this->GetShaderCache()->ReadyShaderProgram(
      vtkTextureObjectVS,
      "//VTK::System::Dec\n"
      "varying vec2 tcoordVC;\n"
      "uniform sampler2D source;\n"
      "//VTK::Output::Dec\n"
      "void main(void) {\n"
      "  gl_FragDepth = texture2D(source,tcoordVC).r; }\n",
      "");
  vtkOpenGLVertexArrayObject *VAO = vtkOpenGLVertexArrayObject::New();

  // bind and activate this texture
  this->DrawPixelsTextureObject->Activate();
  program->SetUniformi("source",
    this->DrawPixelsTextureObject->GetTextureUnit());

  this->DrawPixelsTextureObject->CopyToFrameBuffer(
    0, 0, x2-x1, y2-y1,
    x1, y1, x2, y2,
    this->GetSize()[0], this->GetSize()[1],
    program, VAO);
  this->DrawPixelsTextureObject->Deactivate();
  VAO->Delete();
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthFunc(GL_LEQUAL);

  return VTK_OK;
}

void vtkOpenGLRenderWindow::ActivateTexture(vtkTextureObject *texture)
{
  // Only add if it isn't already there
  typedef std::map<const vtkTextureObject *, int>::const_iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found == this->TextureResourceIds.end())
  {
    int activeUnit =  this->GetTextureUnitManager()->Allocate();
    if (activeUnit < 0)
    {
      vtkErrorMacro("Hardware does not support the number of textures defined.");
      return;
    }
    this->TextureResourceIds.insert(std::make_pair(texture, activeUnit));
    glActiveTexture(GL_TEXTURE0 + activeUnit);
  }
  else
  {
    glActiveTexture(GL_TEXTURE0 + found->second);
  }
}

void vtkOpenGLRenderWindow::DeactivateTexture(vtkTextureObject *texture)
{
  // Only deactivate if it isn't already there
  typedef std::map<const vtkTextureObject *, int>::iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found != this->TextureResourceIds.end())
  {
    this->GetTextureUnitManager()->Free(found->second);
    this->TextureResourceIds.erase(found);
  }
}

int vtkOpenGLRenderWindow::GetTextureUnitForTexture(vtkTextureObject *texture)
{
  // Only deactivate if it isn't already there
  typedef std::map<const vtkTextureObject *, int>::const_iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found != this->TextureResourceIds.end())
  {
    return found->second;
  }

  return -1;
}

// ----------------------------------------------------------------------------
// Description:
// Create an offScreen window based on OpenGL framebuffer extension.
// Return if the creation was successful or not.
// \pre positive_width: width>0
// \pre positive_height: height>0
// \pre not_initialized: !OffScreenUseFrameBuffer
// \post valid_result: (result==0 || result==1)
//                     && (result implies OffScreenUseFrameBuffer)
int vtkOpenGLRenderWindow::CreateHardwareOffScreenWindow(int width, int height)
{
  assert("pre: positive_width" && width>0);
  assert("pre: positive_height" && height>0);
  assert("pre: not_initialized" && !this->OffScreenUseFrameBuffer);

  this->CreateAWindow();
  this->MakeCurrent();
  this->OpenGLInit();

  int result = this->CreateHardwareOffScreenBuffers(width, height);
  if (!result)
  {
    this->DestroyWindow();
  }
  else
  {
    this->BindHardwareOffScreenBuffers();
    this->OffScreenUseFrameBuffer = 1;
  }

  // A=>B = !A || B
  assert("post: valid_result" && (result==0 || result==1)
         && (!result || OffScreenUseFrameBuffer));
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Create an offScreen rendering buffer based on OpenGL framebuffer extension.
// Return if the creation was successful or not.
int vtkOpenGLRenderWindow::CreateHardwareOffScreenBuffers(int width, int height,
                                                          bool bind)
{
  assert("pre: positive_width" && width>0);
  assert("pre: positive_height" && height>0);

  // This implementation currently ignores multisampling configurations:
  // the following code causes tests to fail, commenting it out
  // if (this->MultiSamples > 1)
  //   {
  //   vtkDebugMacro(<<"Multisampling is not currently supported by the "
  //                 "accelerated offscreen rendering backend. Falling back to "
  //                 "a platform-specific offscreen solution...");
  //   return 0;
  //   }

  this->MakeCurrent();

  if (this->FrameBufferObject &&
    this->HardwareBufferSize[0] == width &&
    this->HardwareBufferSize[1] == height)
  {
    if (bind)
    {
      this->BindHardwareOffScreenBuffers();
    }
    return 1;
  }
  if (this->FrameBufferObject)
  {
    this->DestroyHardwareOffScreenBuffers();
  }

  this->FrameBufferObject = 0;
  this->HardwareBufferSize[0] = 0;
  this->HardwareBufferSize[1] = 0;

  int glMajorVersion = 2;
#if GL_ES_VERSION_2_0 != 1
  glGetIntegerv(GL_MAJOR_VERSION, & glMajorVersion);
  if (glMajorVersion < 3 &&
    !glewIsSupported("GL_EXT_framebuffer_object") &&
    !glewIsSupported("GL_ARB_framebuffer_object"))
  {
    vtkDebugMacro( << " extension GL_EXT_framebuffer_object is not supported. "
      "Hardware accelerated offscreen rendering is not available" );
    return 0;
  }
#endif

  int result = 0;

  this->NumberOfFrameBuffers = 1;
#ifdef GL_STEREO
  GLboolean isStereo;
  glGetBooleanv(GL_STEREO, &isStereo);
  if (isStereo)
  {
    this->NumberOfFrameBuffers = 2;
  }
#endif

  // Up to 2 for stereo
  GLuint textureObjects[2] = { 0, 0 };
  GLuint frameBufferObject;
  GLuint depthRenderBufferObject;
  glGenFramebuffers(1, &frameBufferObject); // color
  glGenRenderbuffers(1, &depthRenderBufferObject); // depth
  glGenTextures(this->NumberOfFrameBuffers, textureObjects);
  // Bind the color buffer
  glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);

  GLenum target = GL_TEXTURE_2D;

  for (int i = 0; i < this->NumberOfFrameBuffers; i++)
  {
    glBindTexture(target, textureObjects[i]);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_RGBA8
    glTexImage2D(target,0,GL_RGBA8,width,height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#else
    glTexImage2D(target,0,GL_RGBA,width,height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#endif
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0+i,
                           target, textureObjects[i], 0);
  }
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    vtkDebugMacro(<< "Hardware does not support GPU Offscreen rendering.");
    glBindTexture(target, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &frameBufferObject);
    glDeleteRenderbuffers(1, &depthRenderBufferObject);
    glDeleteTextures(this->NumberOfFrameBuffers, textureObjects);
    bind = true;
  }
  else
  {
    // Set up the depth (and stencil), render buffer
    glBindRenderbuffer(GL_RENDERBUFFER,
                       depthRenderBufferObject);
#ifdef GL_DEPTH_STENCIL
    if (this->StencilCapable)
    {
      glRenderbufferStorage(GL_RENDERBUFFER,
                            GL_DEPTH_STENCIL, width, height);
    }
    else
#endif
    {
#ifdef GL_DEPTH_COMPONENT24
      glRenderbufferStorage(GL_RENDERBUFFER,
                            GL_DEPTH_COMPONENT24, width, height);
#else
      glRenderbufferStorage(GL_RENDERBUFFER,
                            GL_DEPTH_COMPONENT16, width, height);
#endif
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              depthRenderBufferObject);
    if (this->StencilCapable)
    {
      glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                GL_STENCIL_ATTACHMENT,
                                GL_RENDERBUFFER,
                                depthRenderBufferObject);
    }

    // Last check to see if the FBO is supported or not.
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      vtkDebugMacro(<< "Hardware does not support GPU Offscreen rendering with"
                       "this depth/stencil configuration.");
      glBindTexture(target, 0);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDeleteFramebuffers(1, &frameBufferObject);
      glDeleteRenderbuffers(1, &depthRenderBufferObject);
      glDeleteTextures(this->NumberOfFrameBuffers, textureObjects);
      bind = true;
    }
    else
    {
      result = 1;
      // Save GL objects by static casting to standard C types. GL* types
      // are not allowed in VTK header files.
      this->FrameBufferObject =
        static_cast<unsigned int>(frameBufferObject);
      this->DepthRenderBufferObject =
        static_cast<unsigned int>(depthRenderBufferObject);
      for (int i = 0; i < this->NumberOfFrameBuffers; i++)
      {
        this->TextureObjects[i] = static_cast<unsigned int>(textureObjects[i]);
      }
      this->HardwareBufferSize[0] = width;
      this->HardwareBufferSize[1] = height;
      this->HardwareOffScreenBuffersBind = true;

      if (bind)
      {
        unsigned int buffer =
          static_cast<unsigned int>(GL_COLOR_ATTACHMENT0);
        this->BackLeftBuffer  = buffer;
        this->FrontLeftBuffer = buffer;
        this->BackBuffer      = buffer;
        this->FrontBuffer     = buffer;

        if (this->NumberOfFrameBuffers == 2)
        {
          buffer = static_cast<unsigned int>(GL_COLOR_ATTACHMENT0+1);
          this->BackRightBuffer = buffer;
          this->FrontRightBuffer = buffer;
        }
      }
    }
  }

  if (!bind)
  {
    // Rebind the hardware on-screen buffer for now
    this->UnbindHardwareOffScreenBuffers();
  }

  // A=>B = !A || B
  assert("post: valid_result" && (result==0 || result==1)
         && (!result || this->FrameBufferObject));
  return result;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::BindHardwareOffScreenBuffers()
{
  if (!this->FrameBufferObject || this->HardwareOffScreenBuffersBind)
  {
    return;
  }

  this->MakeCurrent();
  glBindFramebuffer(GL_FRAMEBUFFER,
    static_cast<GLuint>(this->FrameBufferObject));

  unsigned int buffer =
    static_cast<unsigned int>(GL_COLOR_ATTACHMENT0);
  this->BackLeftBuffer  = buffer;
  this->FrontLeftBuffer = buffer;
  this->BackBuffer      = buffer;
  this->FrontBuffer     = buffer;

  if (this->NumberOfFrameBuffers == 2)
  {
    buffer = static_cast<unsigned int>(GL_COLOR_ATTACHMENT0+1);
    this->BackRightBuffer = buffer;
    this->FrontRightBuffer = buffer;
  }

  this->HardwareOffScreenBuffersBind = true;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::UnbindHardwareOffScreenBuffers()
{
  if (!this->FrameBufferObject || !this->HardwareOffScreenBuffersBind)
  {
    return;
  }

  this->MakeCurrent();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Restore framebuffer names.
  this->BackLeftBuffer = static_cast<unsigned int>(GL_BACK_LEFT);
  this->BackRightBuffer = static_cast<unsigned int>(GL_BACK_RIGHT);
  this->FrontLeftBuffer = static_cast<unsigned int>(GL_FRONT_LEFT);
  this->FrontRightBuffer = static_cast<unsigned int>(GL_FRONT_RIGHT);
  this->BackBuffer = static_cast<unsigned int>(GL_BACK);
  this->FrontBuffer = static_cast<unsigned int>(GL_FRONT);

  this->HardwareOffScreenBuffersBind = false;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::DestroyHardwareOffScreenBuffers()
{
  if (!this->FrameBufferObject)
  {
    return;
  }
  this->UnbindHardwareOffScreenBuffers();

  GLuint frameBufferObject = static_cast<GLuint>(this->FrameBufferObject);
  glDeleteFramebuffers(1, &frameBufferObject);
  this->FrameBufferObject = 0;

  GLuint depthRenderBufferObject =
    static_cast<GLuint>(this->DepthRenderBufferObject);
  glDeleteFramebuffers(1, &depthRenderBufferObject);
  this->DepthRenderBufferObject = 0;

  GLuint textureObjects[4];
  for (int i = 0; i < this->NumberOfFrameBuffers; i++)
  {
    textureObjects[i] = static_cast<GLuint>(this->TextureObjects[i]);
  }

  glDeleteTextures(this->NumberOfFrameBuffers, textureObjects);

  this->DepthRenderBufferObject = 0;
  this->FrameBufferObject = 0;
  this->HardwareBufferSize[0] = 0;
  this->HardwareBufferSize[1] = 0;
}

// ----------------------------------------------------------------------------
// Description:
// Destroy an offscreen window based on OpenGL framebuffer extension.
// \pre initialized: OffScreenUseFrameBuffer
// \post destroyed: !OffScreenUseFrameBuffer
void vtkOpenGLRenderWindow::DestroyHardwareOffScreenWindow()
{
  assert("pre: initialized" && this->OffScreenUseFrameBuffer);

  this->UnbindHardwareOffScreenBuffers();
  this->DestroyHardwareOffScreenBuffers();

  this->OffScreenUseFrameBuffer = 0;

  this->DestroyWindow();
}

// ----------------------------------------------------------------------------
// Description:
// Returns its texture unit manager object. A new one will be created if one
// hasn't already been set up.
vtkTextureUnitManager *vtkOpenGLRenderWindow::GetTextureUnitManager()
{
  if(this->TextureUnitManager==0)
  {
    vtkTextureUnitManager *manager=vtkTextureUnitManager::New();

    // This does not form a reference loop since vtkOpenGLHardwareSupport does
    // not keep a reference to the render window.
    manager->SetContext(this);
    this->SetTextureUnitManager(manager);
    manager->Delete();
  }
  return this->TextureUnitManager;
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

    // GetTextureUnitManager() will create a new texture unit
    // manager if one does not exist
    if (this->GLStateIntegers["GL_ACTIVE_TEXTURE"] < 0 ||
        this->GLStateIntegers["GL_ACTIVE_TEXTURE"] >
        this->GetTextureUnitManager()->GetNumberOfTextureUnits())
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
    glActiveTexture(GL_TEXTURE0 + this->GLStateIntegers["GL_ACTIVE_TEXTURE"]);

    // Unuse active shader program
    this->GetShaderCache()->ReleaseCurrentShader();
  }
}

// ----------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetUseOffScreenBuffers(bool offScreen)
{
  if (this->OffScreenRendering ||
    (!offScreen && !this->HardwareOffScreenBuffersBind))
  {
    return 1;
  }


  if (!offScreen)
  {
    if (!this->OffScreenUseFrameBuffer)
    {
      this->UnbindHardwareOffScreenBuffers();
    }
    return 1;
  }

  if (this->OffScreenUseFrameBuffer)
  {
    return 1;
  }
  // We are currently in on screen rendering mode.
  // Create offscreen buffers at the screen size.
  int* size = this->GetSize();
  return this->CreateHardwareOffScreenBuffers(size[0], size[1], true);
}

// ----------------------------------------------------------------------------
bool vtkOpenGLRenderWindow::GetUseOffScreenBuffers()
{
  return this->HardwareOffScreenBuffersBind || this->OffScreenRendering;
}

// ----------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SupportsOpenGL()
{
  if (this->OpenGLSupportTested)
  {
    return this->OpenGLSupportResult;
  }

  vtkOutputWindow *oldOW = vtkOutputWindow::GetInstance();
  oldOW->Register(this);
  vtkNew<vtkStringOutputWindow> sow;
  vtkOutputWindow::SetInstance(sow.Get());

  vtkOpenGLRenderWindow *rw = this->NewInstance();
  rw->SetDisplayId(this->GetGenericDisplayId());
  rw->SetOffScreenRendering(1);
  rw->Initialize();
  if (rw->GetContextSupportsOpenGL32())
  {
    this->OpenGLSupportResult = 1;
    this->OpenGLSupportMessage =
      "The system appears to support OpenGL 3.2";
  }

#ifdef GLEW_OK

  else if (GLEW_VERSION_3_2 || GLEW_VERSION_3_1 || GLEW_EXT_gpu_shader4)
  {
    this->OpenGLSupportResult = 1;
    this->OpenGLSupportMessage =
      "The system appears to support OpenGL 3.2/3.1 or has 2.1 with the required extension";
  }

#endif

  if (this->OpenGLSupportResult)
  {
    // even if glew thinks we have support we should actually try linking a
    // shader program to make sure
    vtkShaderProgram *newShader =
      rw->GetShaderCache()->ReadyShaderProgram(
        // simple vert shader
        "//VTK::System::Dec\n"
        "attribute vec4 vertexMC;\n"
        "void main() { gl_Position = vertexMC; }\n",
        // frag shader that used gl_PrimitiveId
        "//VTK::System::Dec\n"
        "//VTK::Output::Dec\n"
        "void main(void) {\n"
        "  gl_FragData[0] = vec4(float(gl_PrimitiveID)/100.0,1.0,1.0,1.0);\n"
        "}\n",
        // no geom shader
        "");
    if (newShader == NULL)
    {
      this->OpenGLSupportResult = 0;
      this->OpenGLSupportMessage =
        "The system appeared to have OpenGL Support but a test shader program failed to compile and link";
    }
  }

  rw->Delete();

  this->OpenGLSupportMessage +=
    "vtkOutputWindow Text Folows:\n\n" +
    sow->GetOutput();
  vtkOutputWindow::SetInstance(oldOW);
  oldOW->Delete();

  this->OpenGLSupportTested = true;

  return this->OpenGLSupportResult;
}
