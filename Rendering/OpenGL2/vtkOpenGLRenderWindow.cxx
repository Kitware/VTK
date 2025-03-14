// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLRenderWindow.h"
#include "vtk_glad.h"

#include "vtkOpenGLHelper.h"

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLQuadHelper.h"
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
#include "vtkRenderingOpenGLConfigure.h"
#include "vtkShaderProgram.h"
#include "vtkStringOutputWindow.h"
#include "vtkTextureObject.h"
#include "vtkTextureUnitManager.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#if defined(_WIN32)
#include "vtkWin32OpenGLRenderWindow.h"
#endif
#if defined(VTK_USE_X)
#include "vtkXOpenGLRenderWindow.h"
#include "vtkglad/include/glad/glx.h"
#endif
#if defined(VTK_OPENGL_HAS_EGL)
#include "vtkEGLRenderWindow.h"
#include "vtkglad/include/glad/egl.h"
#endif
#include "vtkOSOpenGLRenderWindow.h"

#include "vtksys/SystemTools.hxx"

#include "BlueNoiseTexture64x64.h"
#include "vtkTextureObjectVS.h" // a pass through shader

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>
using std::ostringstream;

#include <cassert>

// Initialize static member that controls global maximum number of multisamples
// (off by default on Apple because it causes problems on some Mac models).
#if defined(__APPLE__)
VTK_ABI_NAMESPACE_BEGIN
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 0;
VTK_ABI_NAMESPACE_END
#else
VTK_ABI_NAMESPACE_BEGIN
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 8;
VTK_ABI_NAMESPACE_END
#endif

// Some linux drivers have issues reading a multisampled texture,
// so we check the driver's "Renderer" against this list of strings.
VTK_ABI_NAMESPACE_BEGIN
struct vtkOpenGLRenderWindowDriverInfo
{
  const char* Vendor;
  const char* Version;
  const char* Renderer;
};
static const vtkOpenGLRenderWindowDriverInfo vtkOpenGLRenderWindowMSAATextureBug[] = {
  // OpenGL Vendor: Intel
  // OpenGL Version: 4.6 (Core Profile) Mesa 20.1.3
  // OpenGL Renderer: Mesa Intel® HD Graphics 630 (KBL GT2)
  { "Intel", "", "Mesa Intel" },
  // OpenGL Vendor: X.Org
  // OpenGL Version: 4.6 (Core Profile) Mesa 20.0.8
  // OpenGL Renderer: AMD RAVEN (DRM 3.35.0, 5.4.0-42-generic, LLVM 10.0.0)
  { "X.Org", "", "AMD" },

  // xref https://gitlab.freedesktop.org/mesa/mesa/-/issues/11999
  // OpenGL Vendor: Mesa
  // OpenGL Version: 4.3 (Core Profile) Mesa 24.0.9-0ubuntu0.1
  // OpenGL Renderer: NV137
  { "Mesa", "", "NV" },
};

static const char* defaultWindowName = "Visualization Toolkit - OpenGL";

static const char* ResolveShader =
  R"***(//VTK::System::Dec
  in vec2 texCoord;
  uniform sampler2DMS tex;
  uniform int samplecount;
  //VTK::Output::Dec

  void main()
  {
    float gamma = 2.2;

    // for each sample in the multi sample buffer...
    ivec2 itexcoords = ivec2(floor(textureSize(tex) * texCoord));
    vec3 accumulate = vec3(0.0,0.0,0.0);
    float alpha = 0.0;

    for (int i = 0; i < samplecount; i++)
    {
      vec4 sampleValue = texelFetch(tex, itexcoords, i);
      // apply gamma correction and sum
      accumulate += pow(sampleValue.rgb, vec3(gamma));
      alpha += sampleValue.a;
    }

    // divide and reverse gamma correction
    accumulate /= float(samplecount);
    gl_FragData[0] = vec4(pow(accumulate, vec3(1.0/gamma)), alpha/float(samplecount));
  }
  )***";

static const char* DepthBlitShader =
  R"***(//VTK::System::Dec
  in vec2 texCoord;
  uniform sampler2D tex;
  uniform vec2 texLL;
  uniform vec2 texSize;
  //VTK::Output::Dec

  void main()
  {
    gl_FragDepth = texture(tex, texCoord*texSize + texLL).r;
  }
  )***";

static const char* DepthReadShader =
  R"***(//VTK::System::Dec
  in vec2 texCoord;
  uniform sampler2D tex;
  //VTK::Output::Dec

  void main()
  {
    // define the number of bits in the depth attachment as an integer `depthSize`.
    //VTK::DepthSize::Impl
    float maxNBitUintValue = float(1 << depthSize) - 1.0f;
    float depth = texture(tex, texCoord).r;
    // scale up to max n-bit unsigned integer.
    float z = floor(depth * maxNBitUintValue);
    // extract 8-bit unsigned integers from the n-bit unsigned integer.
    // assume maxNBits == 32, we'll skip unnecessary 8-bit values during reconstruction.
    // gl_FragData[0] = vec4(z & 0xFF, (z >> 8) & 0xFF, (z >> 16) & 0xFF, (z >> 24) & 0xFF);
    float r = mod(z, 256.0f);
    z -= r;
    z /= 256.0f;
    float g = mod(z, 256.0f);
    z -= g;
    z /= 256.0f;
    float b = mod(z, 256.0f);
    z -= b;
    z /= 256.0f;
    float a = mod(z, 256.0f);
    gl_FragData[0] = vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
  }
  )***";

static const char* FlipShader =
  R"***(//VTK::System::Dec
  in vec2 texCoord;
  uniform sampler2D tex;
  //VTK::Output::Dec

  void main()
  {
    gl_FragData[0] = texture(tex, texCoord);
  }
  )***";

#if defined(VTK_REPORT_OPENGL_ERRORS) && defined(GLAD_GL)
static void GLAPIENTRY vtkOpenGLMessageHandler(GLenum source, GLenum type, GLuint id,
  GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
{
  std::string messageType;
  switch (type)
  {
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      messageType = "DEPRECATED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_ERROR:
      messageType = "ERROR";
      break;
    case GL_DEBUG_TYPE_MARKER:
      messageType = "MARKER";
      break;
    case GL_DEBUG_TYPE_OTHER:
      messageType = "OTHER";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      messageType = "PERFORMANCE";
      break;
    case GL_DEBUG_TYPE_POP_GROUP:
      messageType = "POP_GROUP";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      messageType = "PORTABILITY";
      break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
      messageType = "PUSH_GROUP";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      messageType = "UNDEFINED_BEHAVIOR";
      break;
    default:
      messageType = "UNKNOWN";
      break;
  }
  std::string messageSeverity;
  switch (severity)
  {
    case GL_DEBUG_SEVERITY_HIGH:
      messageSeverity = "HIGH";
      break;
    case GL_DEBUG_SEVERITY_LOW:
      messageSeverity = "LOW";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      messageSeverity = "MEDIUM";
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      messageSeverity = "NOTIFICATION";
      break;
    default:
      messageSeverity = "UNKNOWN";
      break;
  }

  std::string sourceType;
  switch (source)
  {
    case GL_DEBUG_SOURCE_API:
      sourceType = "SOURCE_API";
      break;
    case GL_DEBUG_SOURCE_APPLICATION:
      sourceType = "SOURCE_APPLICATION";
      break;
    case GL_DEBUG_SOURCE_OTHER:
      sourceType = "SOURCE_OTHER";
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      sourceType = "SOURCE_SHADER_COMPILER";
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      sourceType = "SOURCE_THIRD_PARTY";
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      sourceType = "SOURCE_WINDOW_SYSTEM";
      break;
    default:
      sourceType = "UNKNOWN";
      break;
  }

  std::ostringstream oss;
  oss << "GL Message: id=" << id << " source=" << sourceType << "(0x" << std::hex << source << ")"
      << std::dec << " type=" << messageType << "(0x" << std::hex << type << std::dec << ")"
      << " severity=" << messageSeverity << "(0x" << std::hex << severity << std::dec << ")"
      << " message=" << message;
  if (severity == GL_DEBUG_SEVERITY_HIGH)
  {
    vtkLog(WARNING, << oss.str());
  }
  else
  {
    vtkLog(TRACE, << oss.str());
  }
}
#endif

#ifdef GL_ES_VERSION_3_0
namespace
{
// helpers to go from GL_ defines to vtkType*
template <GLint GLType>
struct GLTypeToVTKHelper
{
};

template <>
struct GLTypeToVTKHelper<GL_UNSIGNED_BYTE>
{
  using vtk_type = vtkTypeUInt8;
};

template <>
struct GLTypeToVTKHelper<GL_UNSIGNED_INT>
{
  using vtk_type = vtkTypeUInt32;
};

template <>
struct GLTypeToVTKHelper<GL_FLOAT>
{
  using vtk_type = vtkTypeFloat32;
};

template <>
struct GLTypeToVTKHelper<GL_INT>
{
  using vtk_type = vtkTypeInt32;
};

int GetNumberOfColorComponents(const GLint& glFormat)
{
  switch (glFormat)
  {
    case GL_RGB:
      return 3;
    case GL_RGBA:
    case GL_RGBA_INTEGER:
      return 4;
    default:
      return 0;
  }
}

template <typename T1, typename T2>
bool RGBToRGBA(const T1* const rgb, T2* rgba, const std::size_t& n)
{
  constexpr bool is_T1_uint_or_int = std::is_integral<T1>::value || std::is_unsigned<T1>::value;
  constexpr bool is_T2_uint_or_int = std::is_integral<T2>::value || std::is_unsigned<T2>::value;
  constexpr bool is_T1_float32 = std::is_floating_point<T1>::value;
  constexpr bool is_T2_float32 = std::is_floating_point<T2>::value;
  if ((is_T1_uint_or_int && is_T2_uint_or_int) || (is_T1_float32 && is_T2_float32))
  {
    std::size_t idx = 0;
    while (idx < n)
    {
      (*rgba++) = rgb[idx++];
      (*rgba++) = rgb[idx++];
      (*rgba++) = rgb[idx++];
      (*rgba++) = is_T2_uint_or_int ? 255 : 1.0f;
    }
  }
  else if (is_T1_uint_or_int && is_T2_float32)
  {
    std::size_t idx = 0;
    while (idx < n)
    {
      (*rgba++) = rgb[idx++] / 255.0f;
      (*rgba++) = rgb[idx++] / 255.0f;
      (*rgba++) = rgb[idx++] / 255.0f;
      (*rgba++) = 1.0f;
      idx++;
    }
  }
  else if (is_T1_float32 && is_T2_uint_or_int)
  {
    std::size_t idx = 0;
    while (idx < n)
    {
      (*rgba++) = static_cast<T2>(rgb[idx++] * 255.0f);
      (*rgba++) = static_cast<T2>(rgb[idx++] * 255.0f);
      (*rgba++) = static_cast<T2>(rgb[idx++] * 255.0f);
      (*rgba++) = 1.0f;
      idx++;
    }
  }
  else
  {
    return false;
  }
  return true;
}

template <typename T1, typename T2>
bool RGBAToRGB(const T1* const rgba, T2* rgb, const std::size_t& n)
{
  constexpr bool is_T1_uint_or_int = std::is_integral<T1>::value || std::is_unsigned<T1>::value;
  constexpr bool is_T2_uint_or_int = std::is_integral<T2>::value || std::is_unsigned<T2>::value;
  constexpr bool is_T1_float32 = std::is_floating_point<T1>::value;
  constexpr bool is_T2_float32 = std::is_floating_point<T2>::value;
  if ((is_T1_uint_or_int && is_T2_uint_or_int) || (is_T1_float32 && is_T2_float32))
  {
    std::size_t idx = 0;
    while (idx < n)
    {
      (*rgb++) = rgba[idx++];
      (*rgb++) = rgba[idx++];
      (*rgb++) = rgba[idx++];
      idx++;
    }
  }
  else if (is_T1_uint_or_int && is_T2_float32)
  {
    std::size_t idx = 0;
    while (idx < n)
    {
      (*rgb++) = rgba[idx++] / 255.0f;
      (*rgb++) = rgba[idx++] / 255.0f;
      (*rgb++) = rgba[idx++] / 255.0f;
      idx++;
    }
  }
  else if (is_T1_float32 && is_T2_uint_or_int)
  {
    std::size_t idx = 0;
    while (idx < n)
    {
      (*rgb++) = static_cast<T2>(rgba[idx++] * 255.0f);
      (*rgb++) = static_cast<T2>(rgba[idx++] * 255.0f);
      (*rgb++) = static_cast<T2>(rgba[idx++] * 255.0f);
      idx++;
    }
  }
  else
  {
    return false;
  }
  return true;
}

/**
 * Read pixels with parameters <sourceFormat, SourceGLType> and cast into <destFormat,
 * DestinationGLType> Acceptable glformats: GL_RGB, GL_RGBA, GL_RGBA_INTEGER Acceptable gltypes:
 * GL_UNSIGNED_BYTE, GL_UNSIGNED_INT, GL_INT, GL_FLOAT
 */
template <GLint SourceGLType, GLint DestinationGLType>
bool ConvertGLColor(GLint sourceFormat, const vtkRecti& rect, void* data, GLint destFormat)
{
  using SourceTypeNative = typename GLTypeToVTKHelper<SourceGLType>::vtk_type;
  using DestinationTypeNative = typename GLTypeToVTKHelper<DestinationGLType>::vtk_type;

  const int numSrcComponents = ::GetNumberOfColorComponents(sourceFormat);
  const int numDestComponents = ::GetNumberOfColorComponents(destFormat);

  // read pixels in the source color format.
  std::vector<SourceTypeNative> srcPixels(rect.GetWidth() * rect.GetHeight() * numSrcComponents);
  glReadPixels(rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight(), sourceFormat,
    SourceGLType, srcPixels.data());

  // cast pixels to destination color format
  auto dstPixels = static_cast<DestinationTypeNative*>(data);
  if (sourceFormat == destFormat)
  {
    std::copy(srcPixels.begin(), srcPixels.end(), dstPixels);
    return true;
  }
  else if (numSrcComponents == 3 && numDestComponents == 4)
  {
    return RGBToRGBA(srcPixels.data(), dstPixels, srcPixels.size());
  }
  else if (numSrcComponents == 4 && numDestComponents == 3)
  {
    return RGBAToRGB(srcPixels.data(), dstPixels, srcPixels.size());
  }
  else
  {
    return false;
  }
}

// Convenient when both source, dest types are the same.
template <GLint SameGLType>
bool ConvertGLColor(GLint sourceFormat, const vtkRecti& rect, void* data, GLint destFormat)
{
  return ConvertGLColor<SameGLType, SameGLType>(sourceFormat, rect, data, destFormat);
}
}
#endif

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(int val)
{
  if (val == vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples)
  {
    return;
  }
  vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = val;
}

//------------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples()
{
  return vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
}

//------------------------------------------------------------------------------
const char* vtkOpenGLRenderWindow::GetRenderingBackend()
{
  return "OpenGL2";
}

//------------------------------------------------------------------------------
vtkOpenGLRenderWindow::vtkOpenGLRenderWindow()
{
  this->State = vtkOpenGLState::New();
  this->FrameBlitMode = BlitToHardware;
  this->ResolveQuad = nullptr;
  this->DepthBlitQuad = nullptr;
  this->FlipQuad = nullptr;
  this->DepthReadQuad = nullptr;
  this->FramebufferFlipY = false;

  this->Initialized = false;

  this->MultiSamples = vtksys::SystemTools::HasEnv("VTK_TESTING")
    ? 0
    : vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;

  delete[] this->WindowName;
  this->WindowName = new char[strlen(defaultWindowName) + 1];
  strcpy(this->WindowName, defaultWindowName);

  this->RenderFramebuffer = vtkOpenGLFramebufferObject::New();
  this->RenderFramebuffer->SetContext(this);
  this->DisplayFramebuffer = vtkOpenGLFramebufferObject::New();
  this->DisplayFramebuffer->SetContext(this);
  this->ResolveFramebuffer = vtkOpenGLFramebufferObject::New();
  this->ResolveFramebuffer->SetContext(this);
  this->DepthFramebuffer = vtkOpenGLFramebufferObject::New();
  this->DepthFramebuffer->SetContext(this);

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
  this->RenderBufferTargetDepthSize = 32;

  this->TQuad2DVBO = nullptr;
  this->NoiseTextureObject = nullptr;
  this->FirstRenderTime = -1;
  this->LastMultiSamples = -1;

  this->ScreenSize[0] = 0;
  this->ScreenSize[1] = 0;
}

// free up memory & close the window
//------------------------------------------------------------------------------
vtkOpenGLRenderWindow::~vtkOpenGLRenderWindow()
{
  if (this->RenderFramebuffer)
  {
    this->RenderFramebuffer->Delete();
    this->RenderFramebuffer = nullptr;
  }
  if (this->DisplayFramebuffer)
  {
    this->DisplayFramebuffer->Delete();
    this->DisplayFramebuffer = nullptr;
  }
  if (this->ResolveFramebuffer)
  {
    this->ResolveFramebuffer->Delete();
    this->ResolveFramebuffer = nullptr;
  }
  if (this->DepthFramebuffer)
  {
    this->DepthFramebuffer->Delete();
    this->DepthFramebuffer = nullptr;
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

#if !(defined(__APPLE__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__))
//------------------------------------------------------------------------------
vtkOpenGLRenderWindow* vtkOpenGLRenderWindow::New()
{
  const char* backend = std::getenv("VTK_DEFAULT_OPENGL_WINDOW");
#if defined(_WIN32)
  if ((backend == nullptr) || (std::string(backend) == "vtkWin32OpenGLRenderWindow"))
  {
    vtkNew<vtkWin32OpenGLRenderWindow> win32RenderWindow;
    win32RenderWindow->SetOffScreenRendering(true);
    win32RenderWindow->Initialize();
    if (win32RenderWindow->Initialized)
    {
      return win32RenderWindow->NewInstance();
    }
  }
#endif
#if defined(VTK_USE_X)
  if ((backend == nullptr) || (std::string(backend) == "vtkXOpenGLRenderWindow"))
  {
    // No need to complain if GLX failed to load because vtkXOpenGLRenderWindow will
    // print the exact reason as a warning anyway.
    vtkNew<vtkXOpenGLRenderWindow> xRenderWindow;
    xRenderWindow->SetOffScreenRendering(true);
    xRenderWindow->Initialize();
    if (xRenderWindow->Initialized)
    {
      return xRenderWindow->NewInstance();
    }
  }
#endif
#if defined(VTK_OPENGL_HAS_EGL)
  if ((backend == nullptr) || (std::string(backend) == "vtkEGLRenderWindow"))
  {
    // Load core egl functions.
    if (!gladLoaderLoadEGL(EGL_NO_DISPLAY))
    {
      vtkGenericWarningMacro(<< "Failed to load EGL! Please install the EGL library from your "
                                "distribution's package manager.");
    }
    else
    {
      vtkNew<vtkEGLRenderWindow> eglRenderWindow;
      eglRenderWindow->Initialize();
      if (eglRenderWindow->Initialized)
      {
        return eglRenderWindow->NewInstance();
      }
    }
  }
#endif
  if ((backend == nullptr) || (std::string(backend) == "vtkOSOpenGLRenderWindow"))
  {
    // OSMesa support is always built, don't check for initialization it might work if user has
    // libOSMesa.so or osmesa.dll.
    return vtkOSOpenGLRenderWindow::New();
  }
  if (backend != nullptr)
  {
    vtkGenericWarningMacro(<< "Failed to create a vtkOpenGLRenderWindow subclass with "
                              "VTK_DEFAULT_OPENGL_WINDOW="
                           << backend);
  }
  // OSMesa support is always built, it might work if user has libOSMesa.so or osmesa.dll.
  return vtkOSOpenGLRenderWindow::New();
}
#endif

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

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::ReleaseGraphicsResources(vtkWindow* renWin)
{
  this->PushContext();

  delete this->ResolveQuad;
  this->ResolveQuad = nullptr;

  delete this->DepthBlitQuad;
  this->DepthBlitQuad = nullptr;

  delete this->FlipQuad;
  this->FlipQuad = nullptr;

  delete this->DepthReadQuad;
  this->DepthReadQuad = nullptr;

  this->RenderFramebuffer->ReleaseGraphicsResources(renWin);
  this->DisplayFramebuffer->ReleaseGraphicsResources(renWin);
  this->ResolveFramebuffer->ReleaseGraphicsResources(renWin);
  this->DepthFramebuffer->ReleaseGraphicsResources(renWin);

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

//------------------------------------------------------------------------------
vtkMTimeType vtkOpenGLRenderWindow::GetContextCreationTime()
{
  return this->ContextCreationTime.GetMTime();
}

//------------------------------------------------------------------------------
vtkOpenGLShaderCache* vtkOpenGLRenderWindow::GetShaderCache()
{
  return this->GetState()->GetShaderCache();
}

//------------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectCache* vtkOpenGLRenderWindow::GetVBOCache()
{
  return this->GetState()->GetVBOCache();
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
  this->GetState()->vtkglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  this->GetState()->vtkglPixelStorei(GL_PACK_ALIGNMENT, 1);
  // Set the number of alpha bit planes used by the window
  int rgba[4];
  this->GetColorBufferSizes(rgba);
  this->SetAlphaBitPlanes(rgba[3]);
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderWindow::IsPrimIDBugPresent()
{
  if (this->Initialized)
  {
    const char* glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    if (!strcmp(glVendor, "Apple"))
    {
      if (strstr(glVersion, "Metal") != nullptr)
      {
        return true;
      }
    }
  }
  return false;
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

//------------------------------------------------------------------------------
bool vtkOpenGLRenderWindow::InitializeFromCurrentContext()
{
  this->OpenGLInit();
  this->OwnContext = 0;
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::OpenGLInitContext()
{
  this->ContextCreationTime.Modified();

  // When a new OpenGL context is created, force an update
  if (!this->Initialized)
  {
#if defined(GLAD_GL)
    if (this->SymbolLoader.LoadFunction != nullptr)
    {
      if (gladLoadGLUserPtr(this->SymbolLoader.LoadFunction, this->SymbolLoader.UserData) > 0)
      {
        this->Initialized = true;
      }
      else
      {
        vtkWarningMacro(<< "Failed to initialize OpenGL functions!");
      }
    }
    else
    {
      if (gladLoaderLoadGL() > 0)
      {
        this->Initialized = true;
      }
      else
      {
        vtkWarningMacro(<< "Failed to initialize OpenGL functions!");
      }
    }
#else // gles
    this->Initialized = true;
#endif
    if (!this->Initialized)
    {
      vtkWarningMacro(<< "Unable to find a valid OpenGL 3.2 or later implementation. "
                         "Please update your video card driver to the latest version. "
                         "If you are using Mesa please make sure you have version 11.2 or "
                         "later and make sure your driver in Mesa supports OpenGL 3.2 such "
                         "as llvmpipe or openswr. If you are on windows and using Microsoft "
                         "remote desktop note that it only supports OpenGL 3.2 with nvidia "
                         "quadro cards. You can use other remoting software such as nomachine "
                         "to avoid this issue.");
      return;
    }

    // Enable debug output if OpenGL version supports attaching debug callbacks.
#if defined(VTK_REPORT_OPENGL_ERRORS) && defined(GLAD_GL)
    if (GLAD_GL_ARB_debug_output)
    {
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
      glDebugMessageCallback(vtkOpenGLMessageHandler, this);
    }
#endif
    // get this system's supported maximum line width
    // we do it here and store it to avoid repeated glGet
    // calls when the result should not change
    this->MaximumHardwareLineWidth = 1.0;
#if defined(GL_SMOOTH_LINE_WIDTH_RANGE) && defined(GL_ALIASED_LINE_WIDTH_RANGE)
    GLfloat lineWidthRange[2];
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
    if (attachment == GL_NONE)
    {
      // when using vtkGenericOpenGLRenderWindow through QVTKOpenGLNativeWidget,
      // or a subclass of QOpenGLWidget, the rendering takes place in an offscreen buffer
      // and is then transferred to the default framebuffer object which is setup
      // with glDrawBuffers(GL_NONE). So treat it as if it were GL_BACK_LEFT
      // before querying the color buffer sizes.
      attachment = GL_BACK_LEFT;
    }

    // make sure we clear any errors before we start
    // otherwise we may get incorrect results
    while (glGetError() != GL_NO_ERROR)
    {
    }

    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &size);
    if (auto error = glGetError())
    {
      vtkWarningMacro(<< "Failed to get red color buffer size (" << error << ')');
    }
    else
    {
      rgba[0] = static_cast<int>(size);
    }
    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &size);
    if (auto error = glGetError())
    {
      vtkWarningMacro(<< "Failed to get green color buffer size (" << error << ')');
    }
    else
    {
      rgba[1] = static_cast<int>(size);
    }
    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &size);
    if (auto error = glGetError())
    {
      vtkWarningMacro(<< "Failed to get blue color buffer size (" << error << ')');
    }
    else
    {
      rgba[2] = static_cast<int>(size);
    }
    glGetFramebufferAttachmentParameteriv(
      GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &size);
    if (auto error = glGetError())
    {
      vtkWarningMacro(<< "Failed to get alpha color buffer size (" << error << ')');
    }
    else
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
  if (GLAD_GL_ARB_direct_state_access)
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
#else
  (void)attachmentPoint;
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
  if (this->RenderFramebuffer->GetMultiSamples())
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
#ifdef VTK_REPORT_OPENGL_ERRORS
  while (glGetError() != GL_NO_ERROR)
  {
  }
#endif

  this->GetState()->vtkglDisable(GL_SCISSOR_TEST);

  // Calling pack alignment ensures that we can grab the any size window
  this->GetState()->vtkglPixelStorei(GL_PACK_ALIGNMENT, 1);

  this->GetState()->PushReadFramebufferBinding();

  if (front)
  {
    this->DisplayFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->DisplayFramebuffer->ActivateReadBuffer(right ? 1 : 0);
  }
  else
  {
    this->RenderFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->RenderFramebuffer->ActivateReadBuffer(0);

    // Let's determine if we're reading from an FBO.
    bool resolveMSAA = this->GetBufferNeedsResolving();

    if (resolveMSAA)
    {
      this->GetState()->PushDrawFramebufferBinding();
      int* fbsize = this->RenderFramebuffer->GetLastSize();
      this->ResolveFramebuffer->Resize(fbsize[0], fbsize[1]);
      this->ResolveFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);

      // Now blit to resolve the MSAA and get an anti-aliased rendering in
      // resolvedFBO.
      this->GetState()->vtkglBlitFramebuffer(rect.GetLeft(), rect.GetBottom(), rect.GetRight(),
        rect.GetTop(), rect.GetLeft(), rect.GetBottom(), rect.GetRight(), rect.GetTop(),
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
      this->GetState()->PopDrawFramebufferBinding();

      // Now make the resolvedFBO the read buffer and read from it.
      this->ResolveFramebuffer->Bind(GL_READ_FRAMEBUFFER);
      this->ResolveFramebuffer->ActivateReadBuffer(0);
    }
  }
#ifdef GL_ES_VERSION_3_0
  // Open GL ES is very strict about the internal formats and data types that can be
  // used in `glReadPixels`. Even the slightest mistake will result in GL_INVALID_OPERATION
  // These restrictions are documented in the `Errors` section here -
  // https://docs.gl/es3/glReadPixels This block of code queries the pixel format and data type of
  // the currently bound read framebuffer. If those parameters agree with the arguments to this
  // function, we're good. Otherwise, we need to use current parameters and cast as needed. Example,
  // current bound read frame buffer is <GL_RGBA, GL_UNSIGNED_BYTE>, whereas this function was
  // invoked with <GL_RGB, GL_FLOAT>. In this case, `glReadPixels` will be invoked with <GL_RGBA,
  // GL_UNSIGNED_BYTE>. The result will be cast into <GL_RGB, GL_FLOAT>. Such color down-cast will
  // skip the alpha channel. Color up-cast sets alpha = 255
  bool castOk = false;
  GLint currentGlType = 0;
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &currentGlType);
  GLint currentGlFormat = 0;
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &currentGlFormat);
  if (currentGlType == gltype)
  {
    if (currentGlFormat == glformat)
    {
      glReadPixels(rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight(), glformat,
        gltype, data);
      castOk = true;
    }
    else if (gltype == GL_UNSIGNED_BYTE)
    {
      castOk = ConvertGLColor<GL_UNSIGNED_BYTE>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_UNSIGNED_INT)
    {
      castOk = ConvertGLColor<GL_UNSIGNED_INT>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_INT)
    {
      castOk = ConvertGLColor<GL_INT>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_FLOAT)
    {
      castOk = ConvertGLColor<GL_FLOAT>(currentGlFormat, rect, data, glformat);
    }
  }
  else if (currentGlType == GL_UNSIGNED_BYTE)
  {
    if (gltype == GL_FLOAT)
    {
      castOk = ConvertGLColor<GL_UNSIGNED_BYTE, GL_FLOAT>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_UNSIGNED_INT)
    {
      castOk =
        ConvertGLColor<GL_UNSIGNED_BYTE, GL_UNSIGNED_INT>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_INT)
    {
      castOk = ConvertGLColor<GL_UNSIGNED_BYTE, GL_INT>(currentGlFormat, rect, data, glformat);
    }
  }
  else if (currentGlType == GL_UNSIGNED_INT)
  {
    if (gltype == GL_FLOAT)
    {
      castOk = ConvertGLColor<GL_UNSIGNED_INT, GL_FLOAT>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_UNSIGNED_BYTE)
    {
      castOk =
        ConvertGLColor<GL_UNSIGNED_INT, GL_UNSIGNED_BYTE>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_INT)
    {
      castOk = ConvertGLColor<GL_UNSIGNED_INT, GL_INT>(currentGlFormat, rect, data, glformat);
    }
  }
  else if (currentGlType == GL_FLOAT)
  {
    if (gltype == GL_UNSIGNED_INT)
    {
      castOk = ConvertGLColor<GL_FLOAT, GL_UNSIGNED_INT>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_UNSIGNED_BYTE)
    {
      castOk = ConvertGLColor<GL_FLOAT, GL_UNSIGNED_BYTE>(currentGlFormat, rect, data, glformat);
    }
    else if (gltype == GL_INT)
    {
      castOk = ConvertGLColor<GL_FLOAT, GL_INT>(currentGlFormat, rect, data, glformat);
    }
  }
  if (auto error = glGetError())
  {
    vtkErrorMacro(<< "Failed to read pixels in <glformat, gltype>=" << '<' << glformat << ','
                  << gltype << ">. Error code : " << error);
  }
  else if (!castOk)
  {
    vtkErrorMacro(<< "Failed to cast pixel format and/or type from <glformat, gltype> "
                  << currentGlFormat << ',' << currentGlType << " to " << glformat << ','
                  << gltype);
  }
#else
  glReadPixels(
    rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight(), glformat, gltype, data);
#endif

  this->GetState()->PopReadFramebufferBinding();

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
void vtkOpenGLRenderWindow::SetOpenGLSymbolLoader(VTKOpenGLLoaderFunction loader, void* userData)
{
  this->SymbolLoader.LoadFunction = loader;
  this->SymbolLoader.UserData = userData;
}

void vtkOpenGLRenderWindow::TextureDepthBlit(vtkTextureObject* source, int srcX, int srcY,
  int srcX2, int srcY2, int destX, int destY, int destX2, int destY2)
{
  // blit upper right is exclusive
  vtkOpenGLState::ScopedglViewport viewportSaver(this->GetState());
  this->GetState()->vtkglViewport(destX, destY, destX2 - destX, destY2 - destY);
  this->TextureDepthBlit(source, srcX, srcY, srcX2, srcY2);
}

void vtkOpenGLRenderWindow::TextureDepthBlit(vtkTextureObject* source)
{
  this->TextureDepthBlit(source, 0, 0, source->GetWidth(), source->GetHeight());
}

void vtkOpenGLRenderWindow::TextureDepthBlit(
  vtkTextureObject* source, int srcX, int srcY, int srcX2, int srcY2)
{
  assert("pre: must have both source and destination FO" && source);

  if (!this->DepthBlitQuad)
  {
    this->DepthBlitQuad =
      new vtkOpenGLQuadHelper(this, nullptr, DepthBlitShader, "", this->FramebufferFlipY);
    if (!this->DepthBlitQuad->Program || !this->DepthBlitQuad->Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program for depth blits");
    }
  }
  else
  {
    this->GetShaderCache()->ReadyShaderProgram(this->DepthBlitQuad->Program);
  }

  if (this->DepthBlitQuad->Program && this->DepthBlitQuad->Program->GetCompiled())
  {
    auto ostate = this->GetState();
    // save any state we mess with
    vtkOpenGLState::ScopedglEnableDisable stsaver(ostate, GL_SCISSOR_TEST);
    ostate->vtkglDisable(GL_SCISSOR_TEST);

    vtkOpenGLState::ScopedglColorMask colorMaskSaver(ostate);
    ostate->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    vtkOpenGLState::ScopedglDepthMask depthMaskSaver(ostate);
    ostate->vtkglDepthMask(GL_TRUE);

    vtkOpenGLState::ScopedglDepthFunc depthTestSaver(ostate);
    this->GetState()->vtkglDepthFunc(GL_ALWAYS);

    source->Activate();
    double width = source->GetWidth();
    double height = source->GetHeight();
    this->DepthBlitQuad->Program->SetUniformi("tex", source->GetTextureUnit());
    float tmp[2] = { static_cast<float>(srcX / width), static_cast<float>(srcY / height) };
    this->DepthBlitQuad->Program->SetUniform2f("texLL", tmp);
    tmp[0] = (srcX2 - srcX) / width;
    tmp[1] = (srcY2 - srcY) / height;
    this->DepthBlitQuad->Program->SetUniform2f("texSize", tmp);

    this->DepthBlitQuad->Render();
    source->Deactivate();
  }
}

//------------------------------------------------------------------------------
// for crystal eyes in stereo we have to blit here as well
void vtkOpenGLRenderWindow::StereoMidpoint()
{
  this->Superclass::StereoMidpoint();
  if (this->SwapBuffers && this->StereoType == VTK_STEREO_CRYSTAL_EYES)
  {
    this->GetState()->PushFramebufferBindings();

    this->DisplayFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->DisplayFramebuffer->ActivateDrawBuffer(0);

    int* fbsize = this->DisplayFramebuffer->GetLastSize();
    this->GetState()->vtkglViewport(0, 0, fbsize[0], fbsize[1]);
    this->GetState()->vtkglScissor(0, 0, fbsize[0], fbsize[1]);

    // resolve and flip renderframebuffer if needed. If true is returned then the color buffer has
    // already been copied to the displayframebuffer.
    bool copiedColor = this->ResolveFlipRenderFramebuffer();

    this->RenderFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->RenderFramebuffer->ActivateReadBuffer(0);

    this->GetState()->vtkglBlitFramebuffer(0, 0, fbsize[0], fbsize[1], 0, 0, fbsize[0], fbsize[1],
      (copiedColor ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    this->GetState()->PopFramebufferBindings();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::Frame()
{
  if (this->SwapBuffers)
  {
    this->GetState()->PushFramebufferBindings();
    this->DisplayFramebuffer->Bind();
    this->DisplayFramebuffer->ActivateDrawBuffer(
      (this->StereoRender && this->StereoType == VTK_STEREO_CRYSTAL_EYES) ? 1 : 0);

    int* fbsize = this->DisplayFramebuffer->GetLastSize();
    this->GetState()->vtkglViewport(0, 0, fbsize[0], fbsize[1]);
    this->GetState()->vtkglScissor(0, 0, fbsize[0], fbsize[1]);

    // resolve and flip renderframebuffer if needed. If true is returned then the color buffer has
    // already been copied to the displayframebuffer.
    bool copiedColor = this->ResolveFlipRenderFramebuffer();

    this->RenderFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->RenderFramebuffer->ActivateReadBuffer(0);

    if (this->FramebufferFlipY)
    {
      this->TextureDepthBlit(this->RenderFramebuffer->GetDepthAttachmentAsTextureObject());
    }
    else
    {
      this->GetState()->vtkglBlitFramebuffer(0, 0, fbsize[0], fbsize[1], 0, 0, fbsize[0], fbsize[1],
        (copiedColor ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    this->GetState()->vtkglViewport(0, 0, this->Size[0], this->Size[1]);
    this->GetState()->vtkglScissor(0, 0, this->Size[0], this->Size[1]);
    this->GetState()->PopFramebufferBindings();

    if (!this->UseOffScreenBuffers)
    {
      if (this->FrameBlitMode == BlitToHardware)
      {
        this->BlitDisplayFramebuffersToHardware();
      }
      if (this->FrameBlitMode == BlitToCurrent)
      {
        this->BlitDisplayFramebuffer();
      }
      if (this->FrameBlitMode == BlitToCurrentWithDepth)
      {
        this->BlitDisplayFramebufferColorAndDepth();
      }
    }
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderWindow::ResolveFlipRenderFramebuffer()
{
  bool copiedColor = false;

  // Some linux drivers have issues reading a multisampled texture
  bool useTexture = false;
  if (this->MultiSamples > 1 && this->RenderFramebuffer->GetColorAttachmentAsTextureObject(0))
  {
    useTexture = true;
    // can set VTK_FORCE_MSAA=0/1 to override driver exclusion
    const char* useMSAAEnv = std::getenv("VTK_FORCE_MSAA");
    if (useMSAAEnv)
    {
      useTexture = strlen(useMSAAEnv) ? (std::atoi(useMSAAEnv) == 1) : true;
    }
    else
    {
      const std::string& vendorString = this->GetState()->GetVendor();
      const std::string& versionString = this->GetState()->GetVersion();
      const std::string& rendererString = this->GetState()->GetRenderer();
      size_t numExceptions =
        sizeof(vtkOpenGLRenderWindowMSAATextureBug) / sizeof(vtkOpenGLRenderWindowDriverInfo);
      for (size_t i = 0; i < numExceptions; i++)
      {
        if (vendorString.find(vtkOpenGLRenderWindowMSAATextureBug[i].Vendor) == 0 &&
          versionString.find(vtkOpenGLRenderWindowMSAATextureBug[i].Version) == 0 &&
          rendererString.find(vtkOpenGLRenderWindowMSAATextureBug[i].Renderer) == 0)
        {
          useTexture = false;
          break;
        }
      }
    }
  }

  // if we have a MSAA buffer we have to resolve it using a shader as opposed to
  // a normal blit due to linear/gamma colorspace issues
  if (useTexture)
  {
    if (!this->ResolveQuad)
    {
      this->ResolveQuad =
        new vtkOpenGLQuadHelper(this, nullptr, ResolveShader, "", this->FramebufferFlipY);
      if (!this->ResolveQuad->Program || !this->ResolveQuad->Program->GetCompiled())
      {
        vtkErrorMacro("Couldn't build the shader program for resolving msaa.");
      }
    }
    else
    {
      this->GetShaderCache()->ReadyShaderProgram(this->ResolveQuad->Program);
    }

    if (this->ResolveQuad->Program && this->ResolveQuad->Program->GetCompiled())
    {
      this->GetState()->vtkglDisable(GL_DEPTH_TEST);
      this->GetState()->vtkglDisable(GL_BLEND);
      auto tex = this->RenderFramebuffer->GetColorAttachmentAsTextureObject(0);
      tex->Activate();
      this->ResolveQuad->Program->SetUniformi("samplecount", this->MultiSamples);
      this->ResolveQuad->Program->SetUniformi("tex", tex->GetTextureUnit());
      this->ResolveQuad->Render();
      tex->Deactivate();
      copiedColor = true;
      this->GetState()->vtkglEnable(GL_DEPTH_TEST);
      this->GetState()->vtkglEnable(GL_BLEND);
    }
  }

  if (!this->MultiSamples && this->FramebufferFlipY &&
    this->RenderFramebuffer->GetColorAttachmentAsTextureObject(0))
  {
    if (!this->FlipQuad)
    {
      this->FlipQuad =
        new vtkOpenGLQuadHelper(this, nullptr, FlipShader, "", this->FramebufferFlipY);
      if (!this->FlipQuad->Program || !this->FlipQuad->Program->GetCompiled())
      {
        vtkErrorMacro("Couldn't build the shader program for flipping render framebuffer.");
      }
    }
    else
    {
      this->GetShaderCache()->ReadyShaderProgram(this->FlipQuad->Program);
    }

    this->GetState()->vtkglDisable(GL_DEPTH_TEST);

    if (this->FlipQuad->Program && this->FlipQuad->Program->GetCompiled())
    {
      this->GetState()->vtkglDisable(GL_DEPTH_TEST);
      this->GetState()->vtkglDisable(GL_BLEND);
      auto tex = this->RenderFramebuffer->GetColorAttachmentAsTextureObject(0);
      tex->Activate();
      this->FlipQuad->Program->SetUniformi("tex", tex->GetTextureUnit());
      this->FlipQuad->Render();
      tex->Deactivate();
      copiedColor = true;
      this->GetState()->vtkglEnable(GL_DEPTH_TEST);
      this->GetState()->vtkglEnable(GL_BLEND);
    }
  }

  return copiedColor;
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderWindow::ReadDepthComponent(int depthSize)
{
  bool readDepthBuffer = false;
  if (!this->DepthReadQuad)
  {
    std::string shader = DepthReadShader;
    std::ostringstream os;
    os << "int depthSize = " << depthSize << ';';
    vtkShaderProgram::Substitute(shader, "//VTK::DepthSize::Impl", os.str());
    this->DepthReadQuad = new vtkOpenGLQuadHelper(this, nullptr, shader.c_str(), "");
    if (!this->DepthReadQuad->Program || !this->DepthReadQuad->Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program for reading depth component.");
    }
  }
  else
  {
    this->GetShaderCache()->ReadyShaderProgram(this->DepthReadQuad->Program);
  }

  if (this->DepthReadQuad->Program && this->DepthReadQuad->Program->GetCompiled())
  {
    auto ostate = this->GetState();
    vtkOpenGLState::ScopedglEnableDisable depthTestSaver(ostate, GL_DEPTH_TEST);
    ostate->vtkglDisable(GL_DEPTH_TEST);
    vtkOpenGLState::ScopedglEnableDisable blendSaver(ostate, GL_BLEND);
    ostate->vtkglDisable(GL_BLEND);
    auto tex = this->GetBufferNeedsResolving()
      ? this->ResolveFramebuffer->GetDepthAttachmentAsTextureObject()
      : this->RenderFramebuffer->GetDepthAttachmentAsTextureObject();
    tex->Activate();
    this->DepthReadQuad->Program->SetUniformi("tex", tex->GetTextureUnit());
    this->DepthReadQuad->Render();
    tex->Deactivate();
    readDepthBuffer = true;
  }

  return readDepthBuffer;
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::BlitDisplayFramebuffersToHardware()
{
  auto ostate = this->GetState();
  ostate->PushFramebufferBindings();
  ostate->vtkglViewport(0, 0, this->Size[0], this->Size[1]);
  ostate->vtkglScissor(0, 0, this->Size[0], this->Size[1]);

  ostate->vtkglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  if (this->StereoRender && this->StereoType == VTK_STEREO_CRYSTAL_EYES)
  {
    // bind the read buffer to detach the display framebuffer to be safe
    ostate->vtkglBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    this->TextureDepthBlit(this->DisplayFramebuffer->GetDepthAttachmentAsTextureObject());

    this->DisplayFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->DisplayFramebuffer->ActivateReadBuffer(1);
    ostate->vtkglDrawBuffer(this->DoubleBuffer ? GL_BACK_RIGHT : GL_FRONT_RIGHT);
    ostate->vtkglBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0],
      this->Size[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }

  ostate->vtkglDrawBuffer(this->DoubleBuffer ? GL_BACK_LEFT : GL_FRONT_LEFT);
  // bind the read buffer to detach the display framebuffer to be safe
  ostate->vtkglBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  this->TextureDepthBlit(this->DisplayFramebuffer->GetDepthAttachmentAsTextureObject());

  this->DisplayFramebuffer->Bind(GL_READ_FRAMEBUFFER);
  this->DisplayFramebuffer->ActivateReadBuffer(0);
  ostate->vtkglBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0],
    this->Size[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);

  this->GetState()->PopFramebufferBindings();
}

void vtkOpenGLRenderWindow::BlitDisplayFramebuffer()
{
  this->BlitDisplayFramebuffer(0, 0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0],
    this->Size[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void vtkOpenGLRenderWindow::BlitDisplayFramebufferColorAndDepth()
{
  this->BlitDisplayFramebuffer(0, 0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0],
    this->Size[1], GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void vtkOpenGLRenderWindow::BlitDisplayFramebuffer(int right, int srcX, int srcY, int srcWidth,
  int srcHeight, int destX, int destY, int destWidth, int destHeight, int bufferMode,
  int interpolation)
{
  // ON APPLE OSX you must turn off scissor test for DEPTH blits to work
  auto ostate = this->GetState();
  vtkOpenGLState::ScopedglEnableDisable stsaver(ostate, GL_SCISSOR_TEST);
  ostate->vtkglDisable(GL_SCISSOR_TEST);

  ostate->PushReadFramebufferBinding();
  this->DisplayFramebuffer->Bind(GL_READ_FRAMEBUFFER);
  this->DisplayFramebuffer->ActivateReadBuffer(right ? 1 : 0);
  ostate->vtkglViewport(destX, destY, destWidth, destHeight);
  ostate->vtkglScissor(destX, destY, destWidth, destHeight);
  ostate->vtkglBlitFramebuffer(srcX, srcY, srcX + srcWidth, srcY + srcHeight, destX, destY,
    destX + destWidth, destY + destHeight, bufferMode, interpolation);
  ostate->PopReadFramebufferBinding();
}

void vtkOpenGLRenderWindow::BlitToRenderFramebuffer(bool includeDepth)
{
  this->BlitToRenderFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0],
    this->Size[1], GL_COLOR_BUFFER_BIT | (includeDepth ? GL_DEPTH_BUFFER_BIT : 0), GL_NEAREST);
}

void vtkOpenGLRenderWindow::BlitToRenderFramebuffer(int srcX, int srcY, int srcWidth, int srcHeight,
  int destX, int destY, int destWidth, int destHeight, int bufferMode, int interpolation)
{
  // Ensure the offscreen framebuffer is created and updated to the right size
  this->CreateFramebuffers(this->Size[0], this->Size[1]);

  // depending on what is current bound this can be tricky, especially between multisampled
  // buffers
  auto ostate = this->GetState();
  ostate->PushFramebufferBindings();

  ostate->vtkglViewport(destX, destY, destWidth, destHeight);
  ostate->vtkglScissor(destX, destY, destWidth, destHeight);

  // ON APPLE OSX you must turn off scissor test for DEPTH blits to work
  vtkOpenGLState::ScopedglEnableDisable stsaver(ostate, GL_SCISSOR_TEST);
  ostate->vtkglDisable(GL_SCISSOR_TEST);

  // if we are multisampled, then we might have a problem
  if (this->MultiSamples > 1)
  {
    // be safe and always resolve
    int* fbsize = this->RenderFramebuffer->GetLastSize();
    this->ResolveFramebuffer->Resize(fbsize[0], fbsize[1]);
    this->ResolveFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->ResolveFramebuffer->ActivateDrawBuffer(0);

    ostate->vtkglBlitFramebuffer(srcX, srcY, srcX + srcWidth, srcY + srcHeight, destX, destY,
      destX + destWidth, destY + destHeight, bufferMode, interpolation);

    // Now make the resolvedFBO the read buffer and read from it.
    this->ResolveFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->ResolveFramebuffer->ActivateReadBuffer(0);
  }

  this->RenderFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
  this->RenderFramebuffer->ActivateDrawBuffer(0);
  ostate->vtkglBlitFramebuffer(srcX, srcY, srcX + srcWidth, srcY + srcHeight, destX, destY,
    destX + destWidth, destY + destHeight, bufferMode, interpolation);
  ostate->PopFramebufferBindings();
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

  // default to our standard alpha blend eqn, some vtk classes rely on this
  // and do not set it themselves
  this->GetState()->vtkglEnable(GL_BLEND);
  this->GetState()->vtkglBlendFuncSeparate(
    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // creates or resizes the framebuffer
  this->Size[0] = (this->Size[0] > 0 ? this->Size[0] : 300);
  this->Size[1] = (this->Size[1] > 0 ? this->Size[1] : 300);
  this->CreateFramebuffers(this->Size[0], this->Size[1]);

  // push and bind
  this->GetState()->PushFramebufferBindings();
  this->RenderFramebuffer->Bind();
  this->RenderFramebuffer->ActivateDrawBuffer(0);
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
  }

  this->GetState()->PushDrawFramebufferBinding();

  if (front)
  {
    this->DisplayFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->DisplayFramebuffer->ActivateDrawBuffer(right ? 1 : 0);
  }
  else
  {
    this->RenderFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->RenderFramebuffer->ActivateDrawBuffer(0);
  }

  this->DrawPixels(x1, y1, x2, y2, 3, VTK_UNSIGNED_CHAR, data);

  this->GetState()->PopDrawFramebufferBinding();

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
  }

  this->GetState()->PushDrawFramebufferBinding();

  if (front)
  {
    this->DisplayFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->DisplayFramebuffer->ActivateDrawBuffer(right ? 1 : 0);
  }
  else
  {
    this->RenderFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->RenderFramebuffer->ActivateDrawBuffer(0);
  }

  if (!blend)
  {
    this->GetState()->vtkglDisable(GL_BLEND);
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_FLOAT, data); // TODO replace deprecated function
    this->GetState()->vtkglEnable(GL_BLEND);
  }
  else
  {
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_FLOAT, data);
  }

  this->GetState()->PopDrawFramebufferBinding();

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
  }

  this->GetState()->PushDrawFramebufferBinding();

  if (front)
  {
    this->DisplayFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->DisplayFramebuffer->ActivateDrawBuffer(right ? 1 : 0);
  }
  else
  {
    this->RenderFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->RenderFramebuffer->ActivateDrawBuffer(0);
  }

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

  this->GetState()->PopDrawFramebufferBinding();

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
  }

  this->GetState()->vtkglDisable(GL_SCISSOR_TEST);

  // Calling pack alignment ensures that we can grab the any size window
  this->GetState()->vtkglPixelStorei(GL_PACK_ALIGNMENT, 1);

  this->GetState()->PushReadFramebufferBinding();

  this->RenderFramebuffer->Bind(GL_READ_FRAMEBUFFER);
  this->RenderFramebuffer->ActivateReadBuffer(0);

  // Let's determine if we're reading from an FBO.
  bool resolveMSAA = this->GetBufferNeedsResolving();

  if (resolveMSAA)
  {
    this->GetState()->PushDrawFramebufferBinding();
    int* fbsize = this->RenderFramebuffer->GetLastSize();
    this->ResolveFramebuffer->Resize(fbsize[0], fbsize[1]);
    this->ResolveFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);

    // Now blit to resolve the MSAA and get an anti-aliased rendering in
    // resolvedFBO.
    // this is a safe blit as we own both of these texture backed framebuffers
    this->GetState()->vtkglBlitFramebuffer(x_low, y_low, x_low + width, y_low + height, x_low,
      y_low, x_low + width, y_low + height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    this->GetState()->PopDrawFramebufferBinding();

    // Now make the resolvedFBO the read buffer and read from it.
    this->ResolveFramebuffer->Bind(GL_READ_FRAMEBUFFER);
    this->ResolveFramebuffer->ActivateReadBuffer(0);
  }

#ifdef GL_ES_VERSION_3_0
  {
    const int depthSize = this->GetDepthBufferSize();
    this->GetState()->PushDrawFramebufferBinding();
    auto readFramebuffer = resolveMSAA ? this->ResolveFramebuffer : this->RenderFramebuffer;
    int* fbsize = readFramebuffer->GetLastSize();
    this->DepthFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->DepthFramebuffer->ActivateDrawBuffer(0);
    this->GetState()->vtkglViewport(0, 0, fbsize[0], fbsize[1]);
    this->GetState()->vtkglScissor(0, 0, fbsize[0], fbsize[1]);

    bool readDepth = this->ReadDepthComponent(depthSize);
    this->GetState()->PopDrawFramebufferBinding();
    this->GetState()->PopReadFramebufferBinding();
    if (!readDepth)
    {
      vtkErrorMacro(<< "Failed to read depth component!");
      return VTK_ERROR;
    }
    else
    {
      const auto maxDepthValueAsInteger = float(1 << depthSize) - 1.0f;
      this->GetState()->PushReadFramebufferBinding();
      this->DepthFramebuffer->Bind(GL_READ_FRAMEBUFFER);
      this->DepthFramebuffer->ActivateReadBuffer(0);
      std::vector<vtkTypeUInt8> z_data_quarters(width * height * 4, 0);
      const vtkRecti rect(x_low, y_low, width, height);
      ConvertGLColor<GL_UNSIGNED_BYTE>(GL_RGBA, rect, z_data_quarters.data(), GL_RGBA);
      this->GetState()->PopReadFramebufferBinding();
      for (int i = 0, j = 0; i < width * height; ++i)
      {
        vtkTypeUInt32 z_int = z_data_quarters[j++];
        z_int += (z_data_quarters[j++] << 8);
#if defined(GL_DEPTH_COMPONENT24) || defined(GL_DEPTH_COMPONENT32)
        z_int += (z_data_quarters[j++] << 16);
#else
        ++j;
#endif
#ifdef GL_DEPTH_COMPONENT32
        z_int += (z_data_quarters[j++] << 24);
#else
        ++j;
#endif
        z_data[i] = z_int / maxDepthValueAsInteger;
      }
    }
  }
#else
  glReadPixels(x_low, y_low, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, z_data);

  this->GetState()->PopReadFramebufferBinding();
#endif

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

  this->GetState()->PushDrawFramebufferBinding();

  this->RenderFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
  this->RenderFramebuffer->ActivateDrawBuffer(0);

  // bind and activate this texture
  this->DrawPixelsTextureObject->Activate();
  program->SetUniformi("source", this->DrawPixelsTextureObject->GetTextureUnit());

  this->DrawPixelsTextureObject->CopyToFrameBuffer(
    0, 0, x2 - x1, y2 - y1, x1, y1, x2, y2, this->GetSize()[0], this->GetSize()[1], program, VAO);
  this->DrawPixelsTextureObject->Deactivate();
  VAO->Delete();

  this->GetState()->PopDrawFramebufferBinding();

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

//------------------------------------------------------------------------------
// Description:
// Create an offScreen window based on OpenGL framebuffer extension.
// Return if the creation was successful or not.
// \pre positive_width: width>0
// \pre positive_height: height>0
// \post valid_result: (result==0 || result==1)
int vtkOpenGLRenderWindow::CreateFramebuffers(int width, int height)
{
  assert("pre: positive_width" && width > 0);
  assert("pre: positive_height" && height > 0);

#if defined(__APPLE__)
  // make sure requested multisamples is OK with platform
  // APPLE Intel systems seem to have buggy multisampled
  // framebuffer blits etc that cause issues
  if (this->MultiSamples > 0)
  {
    if (this->GetState()->GetVendor().find("Intel") != std::string::npos)
    {
      this->MultiSamples = 0;
    }
  }
#endif

  if (this->LastMultiSamples != this->MultiSamples)
  {
    this->RenderFramebuffer->ReleaseGraphicsResources(this);
  }

  if (!this->RenderFramebuffer->GetFBOIndex())
  {
    // verify that our multisample setting does not exceed the hardware
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
      this->MultiSamples = 0;
#endif
    }
    this->GetState()->PushFramebufferBindings();
    this->RenderFramebuffer->PopulateFramebuffer(width, height,
#ifdef GL_TEXTURE_2D_MULTISAMPLE
      true, // textures
#else
      this->MultiSamples ? false : true, // textures
#endif
      1, VTK_UNSIGNED_CHAR,                    // 1 color buffer uchar
      true, this->RenderBufferTargetDepthSize, // depth buffer
      this->MultiSamples, this->StencilCapable != 0);
    this->LastMultiSamples = this->MultiSamples;
    this->GetState()->PopFramebufferBindings();
  }
  else
  {
    this->RenderFramebuffer->Resize(width, height);
  }

  if (!this->DisplayFramebuffer->GetFBOIndex())
  {
    this->GetState()->PushFramebufferBindings();
    this->DisplayFramebuffer->PopulateFramebuffer(width, height,
      true,                                    // textures
      2, VTK_UNSIGNED_CHAR,                    // 1 color buffer uchar
      true, this->RenderBufferTargetDepthSize, // depth buffer
      0, this->StencilCapable != 0);
    this->GetState()->PopFramebufferBindings();
  }
  else
  {
    this->DisplayFramebuffer->Resize(width, height);
  }

  if (!this->ResolveFramebuffer->GetFBOIndex())
  {
    this->GetState()->PushFramebufferBindings();
    this->ResolveFramebuffer->PopulateFramebuffer(width, height,
      true,                                    // textures
      1, VTK_UNSIGNED_CHAR,                    // 1 color buffer uchar
      true, this->RenderBufferTargetDepthSize, // depth buffer
      0, this->StencilCapable != 0);
    this->GetState()->PopFramebufferBindings();
  }

  if (!this->DepthFramebuffer->GetFBOIndex())
  {
    this->GetState()->PushFramebufferBindings();
    this->DepthFramebuffer->PopulateFramebuffer(width, height,
      true,                 // textures
      1, VTK_UNSIGNED_CHAR, // 1 color buffer uchar
      false, 0,             // depth buffer
      0, this->StencilCapable != 0);
    this->GetState()->PopFramebufferBindings();
  }
  else
  {
    this->DepthFramebuffer->Resize(width, height);
  }
  return 1;
}

//------------------------------------------------------------------------------
// Description:
// Returns its texture unit manager object. A new one will be created if one
// hasn't already been set up.
vtkTextureUnitManager* vtkOpenGLRenderWindow::GetTextureUnitManager()
{
  return this->GetState()->GetTextureUnitManager();
}

//------------------------------------------------------------------------------
// Description:
// Block the thread until the actual rendering is finished().
// Useful for measurement only.
void vtkOpenGLRenderWindow::WaitForCompletion()
{
  glFinish();
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SaveGLState()
{
  // For now just query the active texture unit
  if (this->Initialized)
  {
    this->MakeCurrent();
    vtkOpenGLRenderUtilities::MarkDebugEvent("Saving OpenGL State");
    this->GetState()->Reset();
    this->GetState()->Push();
    vtkOpenGLRenderUtilities::MarkDebugEvent("Saved OpenGL State");
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderWindow::RestoreGLState()
{
  // Prevent making GL calls unless we have a valid context
  if (this->Initialized)
  {
    vtkOpenGLRenderUtilities::MarkDebugEvent("Restoring OpenGL State");
    this->GetState()->Pop();
    // Unuse active shader program
    this->GetShaderCache()->ReleaseCurrentShader();
    vtkOpenGLRenderUtilities::MarkDebugEvent("Restored OpenGL State");
  }
}

//------------------------------------------------------------------------------
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
  if (!rw->Initialized)
  {
    this->OpenGLSupportMessage =
      "Failed to initialize OpenGL for this window, OpenGL not supported.";
    rw->Delete();
    vtkOutputWindow::SetInstance(oldOW);
    oldOW->Delete();
    return 0;
  }

#if defined(GLAD_GL)
  else if (GLAD_GL_VERSION_3_2 || GLAD_GL_VERSION_3_1)
  {
    this->OpenGLSupportResult = 1;
    this->OpenGLSupportMessage = "The system appears to support OpenGL 3.2/3.1";
  }
#elif defined(GLAD_GLES2)
  else if (GLAD_GL_ES_VERSION_3_2)
  {
    this->OpenGLSupportResult = 1;
    this->OpenGLSupportMessage = "The system appears to support OpenGL ES 3.2";
  }
  else if (GLAD_GL_ES_VERSION_3_1)
  {
    this->OpenGLSupportResult = 1;
    this->OpenGLSupportMessage = "The system appears to support OpenGL ES 3.1";
  }
  else if (GLAD_GL_ES_VERSION_3_0)
  {
    this->OpenGLSupportResult = 1;
    this->OpenGLSupportMessage = "The system appears to support OpenGL ES 3.0";
  }
  else if (GLAD_GL_ES_VERSION_2_0)
  {
    this->OpenGLSupportResult = 1;
    this->OpenGLSupportMessage = "The system appears to support OpenGL ES 2.0";
  }
#endif

  if (this->OpenGLSupportResult)
  {
    // even if glad thinks we have support we should actually try linking a
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

  this->OpenGLSupportMessage += "vtkOutputWindow Text Follows:\n\n" + sow->GetOutput();
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
    vtkNew<vtkJPEGReader> imgReader;

    imgReader->SetMemoryBuffer(BlueNoiseTexture64x64);
    imgReader->SetMemoryBufferLength(sizeof(BlueNoiseTexture64x64));
    imgReader->Update();
    vtkImageData* textureReader = imgReader->GetOutput();

    int const bufferSize = 64 * 64;
    float* noiseTextureData = new float[bufferSize];
    for (int i = 0; i < bufferSize; i++)
    {
      int const x = i % 64;
      int const y = i / 64;
      noiseTextureData[i] = textureReader->GetScalarComponentAsFloat(x, y, 0, 0) / 255.0f;
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
  if (this->Initialized)
  {
    vtkOpenGLRenderUtilities::MarkDebugEvent("Starting vtkOpenGLRenderWindow::Render");
  }
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
  if (this->Initialized)
  {
    vtkOpenGLRenderUtilities::MarkDebugEvent("Completed vtkOpenGLRenderWIndow::Render");
  }
}
VTK_ABI_NAMESPACE_END
