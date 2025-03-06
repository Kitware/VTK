// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRManagerOpenGLGraphics.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenXRManager.h"

#define XR_USE_GRAPHICS_API_OPENGL
#include <vtkOpenXRPlatform.h>

// include what we need for the helper window
#ifdef VTK_USE_X
// We need to cast to a XOpenGLRenderWindow for vtkXVisualInfo->visualid
#include "vtkXOpenGLRenderWindow.h"

// From vtkXOpenGLRenderWindow.cxx :
// Work-around to get forward declarations of C typedef of anonymous
// structs working. We do not want to include XUtil.h in the header as
// it populates the global namespace.
#include <X11/Xutil.h>
VTK_ABI_NAMESPACE_BEGIN
struct vtkXVisualInfo : public XVisualInfo
{
};
VTK_ABI_NAMESPACE_END
#endif // VTK_USE_X

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenXRManagerOpenGLGraphics::PIMPL
{
public:
  // OpenGL swapchains
  std::vector<SwapchainImagesOpenGL> ColorSwapchains;
  std::vector<SwapchainImagesOpenGL> DepthSwapchains;
};

vtkStandardNewMacro(vtkOpenXRManagerOpenGLGraphics);

//------------------------------------------------------------------------------
vtkOpenXRManagerOpenGLGraphics::vtkOpenXRManagerOpenGLGraphics()
{
  this->Private = new PIMPL;
}

//------------------------------------------------------------------------------
vtkOpenXRManagerOpenGLGraphics::~vtkOpenXRManagerOpenGLGraphics()
{
  delete this->Private;
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerOpenGLGraphics::SetNumberOfSwapchains(uint32_t viewCount)
{
  this->Private->ColorSwapchains.resize(viewCount);
  this->Private->DepthSwapchains.resize(viewCount);
};

//------------------------------------------------------------------------------
void vtkOpenXRManagerOpenGLGraphics::GetColorSwapchainImage(
  uint32_t scIndex, uint32_t imgIndex, void* texture)
{
  *(GLuint*)texture = this->Private->ColorSwapchains[scIndex].Images[imgIndex].image;
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerOpenGLGraphics::GetDepthSwapchainImage(
  uint32_t scIndex, uint32_t imgIndex, void* texture)
{
  *(GLuint*)texture = this->Private->DepthSwapchains[scIndex].Images[imgIndex].image;
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerOpenGLGraphics::EnumerateColorSwapchainImages(
  XrSwapchain swapchain, uint32_t scIndex)
{
  this->EnumerateSwapchainImages(swapchain, this->Private->ColorSwapchains[scIndex]);
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerOpenGLGraphics::EnumerateDepthSwapchainImages(
  XrSwapchain swapchain, uint32_t scIndex)
{
  this->EnumerateSwapchainImages(swapchain, this->Private->DepthSwapchains[scIndex]);
}

//------------------------------------------------------------------------------
const std::vector<int64_t>& vtkOpenXRManagerOpenGLGraphics::GetSupportedColorFormats()
{
  const static std::vector<int64_t> supportedColorFormats = { GL_RGBA32F, GL_RGBA16F, GL_RGBA16,
    GL_SRGB8_ALPHA8_EXT };
  return supportedColorFormats;
}

//------------------------------------------------------------------------------
const std::vector<int64_t>& vtkOpenXRManagerOpenGLGraphics::GetSupportedDepthFormats()
{
  const static std::vector<int64_t> supportedDepthFormats = { GL_DEPTH_COMPONENT16,
    GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT32F };
  return supportedDepthFormats;
}

//------------------------------------------------------------------------------
void vtkOpenXRManagerOpenGLGraphics::EnumerateSwapchainImages(
  XrSwapchain swapchain, SwapchainImagesOpenGL& swapchainImages)
{
  uint32_t chainLength = this->GetChainLength(swapchain);

  swapchainImages.Images.resize(chainLength, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });

  vtkOpenXRManager::GetInstance().XrCheckOutput(vtkOpenXRManager::ErrorOutput,
    xrEnumerateSwapchainImages(swapchain, (uint32_t)swapchainImages.Images.size(), &chainLength,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchainImages.Images.data())),
    "Failed to enumerate swapchain images");
}

//------------------------------------------------------------------------------
bool vtkOpenXRManagerOpenGLGraphics::CreateGraphicsBinding(vtkOpenGLRenderWindow* helperWindow)
{
#ifdef VTK_USE_X
  // Create the XrGraphicsBindingOpenGLXlibKHR structure
  // That will be in the next chain of the XrSessionCreateInfo
  // We need to fill xDisplay, visualId, glxFBConfig, glxDrawable and glxContext

  // clang-format off
  // XXX(c++20): use `make_shared` (aggregate initialization support)
  auto graphicsBindingGLX =
    std::shared_ptr<XrGraphicsBindingOpenGLXlibKHR>(new XrGraphicsBindingOpenGLXlibKHR{  // NOLINT(modernize-make-shared)
      XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR, // .type
      nullptr,                                  // .next
      nullptr,                                  // .xDisplay. a valid X11 display
      0,                                        // .visualid. a valid X11 visual id
      0,                                        // .glxFBConfig. a valid X11 OpenGL GLX GLXFBConfig
      0,                                        // .glxDrawable. a valid X11 OpenGL GLX GLXDrawable
      0                                         // .glxContext. a valid X11 OpenGL GLX GLXContext
    });
  // clang-format on
  this->GraphicsBinding = graphicsBindingGLX;

  vtkNew<vtkXOpenGLRenderWindow> xoglRenWin;
  vtkXOpenGLRenderWindow* glxHelperWindow = vtkXOpenGLRenderWindow::SafeDownCast(helperWindow);

  if (glxHelperWindow == nullptr)
  {
    xoglRenWin->InitializeFromCurrentContext();
    glxHelperWindow = xoglRenWin;
  }

  vtkXVisualInfo* v = glxHelperWindow->GetDesiredVisualInfo();
  GLXFBConfig* fbConfig = reinterpret_cast<GLXFBConfig*>(glxHelperWindow->GetGenericFBConfig());

  graphicsBindingGLX->xDisplay = glxHelperWindow->GetDisplayId();
  graphicsBindingGLX->glxDrawable = glxHelperWindow->GetWindowId();
  graphicsBindingGLX->glxContext = glXGetCurrentContext();
  graphicsBindingGLX->visualid = v->visualid;
  graphicsBindingGLX->glxFBConfig = *fbConfig;

#elif defined(_WIN32)
  (void)helperWindow; // Hide unused parameter warning
  // clang-format off
  // XXX(c++20): use `make_shared` (aggregate initialization support)
  auto graphicsBindingGLWin32 =
    std::shared_ptr<XrGraphicsBindingOpenGLWin32KHR>(new XrGraphicsBindingOpenGLWin32KHR{  // NOLINT(modernize-make-shared
      XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, // .type
      nullptr,                                   // .next
      0,                                         // .hdC : a valid Windows HW device context handle.
      0 // .hGLRChandle : a valid Windows OpenGL rendering context
    });
  // clang-format on
  this->GraphicsBinding = graphicsBindingGLWin32;

  graphicsBindingGLWin32->hDC = wglGetCurrentDC();
  graphicsBindingGLWin32->hGLRC = wglGetCurrentContext();

#else
  (void)helperWindow; // Hide unused parameter warning
  vtkErrorMacro(<< "Only X11 and Win32 are supported at the moment.");
  return false;
#endif

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManagerOpenGLGraphics::CheckGraphicsRequirements(XrInstance instance, XrSystemId id)
{
  XrGraphicsRequirementsOpenGLKHR openGLReqs = {
    XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR, // .type
    nullptr,                                  // .next
    0,                                        // .minApiVersionSupported
    0                                         // .maxApiVersionSupported
  };

  xr::GraphicsExtensionDispatchTable extensions;
  extensions.PopulateDispatchTable(instance);

  if (!vtkOpenXRManager::GetInstance().XrCheckOutput(vtkOpenXRManager::ErrorOutput,
        extensions.xrGetOpenGLGraphicsRequirementsKHR(instance, id, &openGLReqs),
        "Failed to get OpenGL graphics requirements!"))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
const char* vtkOpenXRManagerOpenGLGraphics::GetBackendExtensionName()
{
  return XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
}
VTK_ABI_NAMESPACE_END
