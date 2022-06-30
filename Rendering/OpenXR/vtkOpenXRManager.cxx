/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRManager.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenXRUtilities.h"
#include "vtkWindows.h" // Does nothing if we are not on windows

// include what we need for the helper window
#ifdef VTK_USE_X
// We need to cast to a XOpenGLRenderWindow for vtkXVisualInfo->visualid
#include "vtkXOpenGLRenderWindow.h"

// From vtkXOpenGLRenderWindow.cxx :
// Work-around to get forward declarations of C typedef of anonymous
// structs working. We do not want to include XUtil.h in the header as
// it populates the global namespace.
#include "GL/glx.h"
#include <X11/Xutil.h>
struct vtkXVisualInfo : public XVisualInfo
{
};
#endif // VTK_USE_X

#define VTK_CHECK_NULL_XRHANDLE(handle, msg)                                                       \
  if (handle == XR_NULL_HANDLE)                                                                    \
  {                                                                                                \
    vtkErrorWithObjectMacro(nullptr, << msg << " is a null handle.");                              \
    return false;                                                                                  \
  }

//------------------------------------------------------------------------------
bool vtkOpenXRManager::Initialize(vtkOpenGLRenderWindow* helperWindow)
{
  if (!this->CreateInstance())
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed to CreateInstance");
    return false;
  }

  // Create the SubactionPaths (left / right hand and head)
  if (!this->CreateSubactionPaths())
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed to CreateSubactionPaths");
    return false;
  }

  if (!this->CreateSystem())
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed to CreateSystem");
    return false;
  }

  if (!this->CheckGraphicsRequirements())
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed in CheckGraphicsRequirements");
    return false;
  }

  if (!this->CreateGraphicsBinding(helperWindow))
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed to CreateGraphicsBinding");
    return false;
  }

  if (!this->CreateSession())
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed to CreateSession");
    return false;
  }

  if (!this->CreateReferenceSpace())
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed to CreateReferenceSpace");
    return false;
  }

  if (!this->CreateSwapchains())
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed to CreateSwapChains");
    return false;
  }

  if (!this->LoadControllerModels())
  {
    vtkWarningWithObjectMacro(nullptr, "Initialize failed to LoadController Models");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkOpenXRManager::Finalize()
{
  this->DestroyActionSets();
  xrEndSession(this->Session);
  xrDestroySession(this->Session);
  xrDestroyInstance(this->Instance);
}

//------------------------------------------------------------------------------
std::tuple<uint32_t, uint32_t> vtkOpenXRManager::GetRecommendedImageRectSize()
{
  if (this->RenderResources->ConfigViews.size() == 0)
  {
    return std::make_tuple(0, 0);
  }
  return std::make_tuple(this->RenderResources->ConfigViews[0].recommendedImageRectWidth,
    this->RenderResources->ConfigViews[0].recommendedImageRectHeight);
};

//------------------------------------------------------------------------------
std::string vtkOpenXRManager::GetOpenXRPropertiesAsString()
{
  XrInstanceProperties instanceProperties = {
    XR_TYPE_INSTANCE_PROPERTIES, // .type
    nullptr,                     // .next
  };
  if (!this->XrCheckWarn(xrGetInstanceProperties(this->Instance, &instanceProperties),
        "Failed to get instance info"))
  {
    return "";
  }

  std::string properties = std::string(instanceProperties.runtimeName) + " " +
    std::to_string(XR_VERSION_MAJOR(instanceProperties.runtimeVersion)) + "." +
    std::to_string(XR_VERSION_MINOR(instanceProperties.runtimeVersion)) + "." +
    std::to_string(XR_VERSION_PATCH(instanceProperties.runtimeVersion));

  return std::move(properties);
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::BeginSession()
{
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::BeginSession, Session");

  XrSessionBeginInfo session_begin_info = {
    XR_TYPE_SESSION_BEGIN_INFO, // .type
    nullptr,                    // .next
    this->ViewType              // .primaryViewConfigurationType
  };
  if (!this->XrCheckWarn(
        xrBeginSession(this->Session, &session_begin_info), "Failed to begin session!"))
  {
    return false;
  }

  vtkDebugWithObjectMacro(nullptr, "Session started.");

  this->SessionRunning = true;

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::WaitAndBeginFrame()
{
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::WaitAndBeginFrame, Session");

  // Wait frame
  XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
  XrFrameState frameState{ XR_TYPE_FRAME_STATE };

  if (!this->XrCheckError(
        xrWaitFrame(this->Session, &frameWaitInfo, &frameState), "Failed to wait frame."))
  {
    return false;
  }

  // Begin frame
  XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
  if (!this->XrCheckError(xrBeginFrame(this->Session, &frameBeginInfo), "Failed to begin frame."))
  {
    return false;
  }

  // Store the value of shouldRender to avoid a render
  this->ShouldRenderCurrentFrame = frameState.shouldRender;

  // Store the value of frame predicted display time that is used in EndFrame
  this->PredictedDisplayTime = frameState.predictedDisplayTime;

  if (this->ShouldRenderCurrentFrame)
  {
    // Locate the views : this will update view pose and projection fov for each view
    XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = this->ViewType;
    viewLocateInfo.displayTime = frameState.predictedDisplayTime;
    viewLocateInfo.space = this->ReferenceSpace;
    const uint32_t viewCount = this->GetViewCount();
    uint32_t viewCountOutput;
    if (!this->XrCheckError(
          xrLocateViews(this->Session, &viewLocateInfo, &this->RenderResources->ViewState,
            viewCount, &viewCountOutput, this->RenderResources->Views.data()),
          "Failed to locate views !"))
    {
      return false;
    }

    if (viewCountOutput != viewCount)
    {
      vtkWarningWithObjectMacro(nullptr, << "ViewCountOutput (" << viewCountOutput
                                         << ") is different than ViewCount (" << viewCount
                                         << ") !");
    }
  }

  return true;
}

// loads the controller models using an extension if it is present.
// todo needs to be tied into the models class and
// the gltf conversion completed right now it is here as an example
// to start from.
bool vtkOpenXRManager::LoadControllerModels()
{
  if (!this->OptionalExtensions.ControllerModelExtensionSupported)
  {
    return true;
  }

  auto lPath = this->GetXrPath("/user/hand/left");

  XrControllerModelKeyStateMSFT controllerModelKeyState;
  this->XrCheckError(
    this->Extensions.xrGetControllerModelKeyMSFT(this->Session, lPath, &controllerModelKeyState),
    "Failed to get controller model key!");

  // get the size
  uint32_t bufferCountOutput = 0;
  this->XrCheckError(this->Extensions.xrLoadControllerModelMSFT(this->Session,
                       controllerModelKeyState.modelKey, 0, &bufferCountOutput, nullptr),
    "Failed to get controller model size!");

  // get the data
  uint32_t bufferCapacityInput = bufferCountOutput;
  uint8_t* buffer = new uint8_t[bufferCountOutput];
  this->XrCheckError(
    this->Extensions.xrLoadControllerModelMSFT(this->Session, controllerModelKeyState.modelKey,
      bufferCapacityInput, &bufferCountOutput, buffer),
    "Failed to get controller model!");

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::PrepareRendering(
  const uint32_t eye, GLuint& colorTextureId, GLuint& depthTextureId)
{
  const vtkOpenXRManager::SwapchainOpenGL_t& colorSwapchain =
    this->RenderResources->ColorSwapchains[eye];
  const vtkOpenXRManager::SwapchainOpenGL_t& depthSwapchain =
    this->RenderResources->DepthSwapchains[eye];

  // Use the full size of the allocated swapchain image (could render smaller some frames to hit
  // framerate)
  const XrRect2Di imageRect = { { 0, 0 },
    { (int32_t)colorSwapchain.Width, (int32_t)colorSwapchain.Height } };

  if (this->OptionalExtensions.DepthExtensionSupported)
  {
    if (colorSwapchain.Width != depthSwapchain.Width)
    {
      vtkErrorWithObjectMacro(nullptr, << "Color swapchain width (" << colorSwapchain.Width
                                       << ") differs from depth swapchain width ("
                                       << depthSwapchain.Width << ").");
      return false;
    }
    if (colorSwapchain.Height != depthSwapchain.Height)
    {
      vtkErrorWithObjectMacro(nullptr, << "Color swapchain height (" << colorSwapchain.Height
                                       << ") differs from depth swapchain height ("
                                       << depthSwapchain.Height << ").");
      return false;
    }
  }

  // Store the texture to render into it during the render method
  const uint32_t colorSwapchainImageIndex =
    this->WaitAndAcquireSwapchainImage(colorSwapchain.Swapchain);
  colorTextureId = colorSwapchain.Images[colorSwapchainImageIndex].image;

  this->RenderResources->ProjectionLayerViews[eye] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
  this->RenderResources->ProjectionLayerViews[eye].pose = this->RenderResources->Views[eye].pose;
  this->RenderResources->ProjectionLayerViews[eye].fov = this->RenderResources->Views[eye].fov;
  this->RenderResources->ProjectionLayerViews[eye].subImage.swapchain = colorSwapchain.Swapchain;
  this->RenderResources->ProjectionLayerViews[eye].subImage.imageRect = imageRect;
  this->RenderResources->ProjectionLayerViews[eye].subImage.imageArrayIndex = 0;

  if (this->OptionalExtensions.DepthExtensionSupported)
  {
    const uint32_t depthSwapchainImageIndex =
      this->WaitAndAcquireSwapchainImage(depthSwapchain.Swapchain);
    depthTextureId = depthSwapchain.Images[depthSwapchainImageIndex].image;

    this->RenderResources->DepthInfoViews[eye] = { XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR };
    this->RenderResources->DepthInfoViews[eye].minDepth = 0;
    this->RenderResources->DepthInfoViews[eye].maxDepth = 1;
    this->RenderResources->DepthInfoViews[eye].nearZ = 0.1;
    this->RenderResources->DepthInfoViews[eye].farZ = 20.0;
    this->RenderResources->DepthInfoViews[eye].subImage.swapchain = depthSwapchain.Swapchain;
    this->RenderResources->DepthInfoViews[eye].subImage.imageRect = imageRect;
    this->RenderResources->DepthInfoViews[eye].subImage.imageArrayIndex = 0;

    // Chain depth info struct to the corresponding projection layer view's next pointer
    this->RenderResources->ProjectionLayerViews[eye].next =
      &this->RenderResources->DepthInfoViews[eye];
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkOpenXRManager::ReleaseSwapchainImage(const uint32_t eye)
{
  XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };

  this->XrCheckError(
    xrReleaseSwapchainImage(this->RenderResources->ColorSwapchains[eye].Swapchain, &releaseInfo),
    "Failed to release color swapchain image!");

  if (this->OptionalExtensions.DepthExtensionSupported)
  {
    this->XrCheckError(
      xrReleaseSwapchainImage(this->RenderResources->DepthSwapchains[eye].Swapchain, &releaseInfo),
      "Failed to release depth swapchain image!");
  }
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::EndFrame()
{
  // The projection layer consists of projection layer views.
  XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
  std::vector<XrCompositionLayerBaseHeader*> layers;

  // If the frame has been rendered, then we must submit the ProjectionLayerViews:
  if (this->ShouldRenderCurrentFrame)
  {
    // Inform the runtime that the app's submitted alpha channel has valid data for use during
    // composition. The primary display on HoloLens has an additive environment blend mode. It will
    // ignore the alpha channel. However, mixed reality capture uses the alpha channel if this bit
    // is set to blend content with the environment. layer.layerFlags =
    // XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    layer.layerFlags = 0;
    layer.space = this->ReferenceSpace;
    layer.viewCount = (uint32_t)this->RenderResources->ProjectionLayerViews.size();
    layer.views = this->RenderResources->ProjectionLayerViews.data();

    // Add the layer to the submitted layers
    layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
  }
  // Reset should render state
  this->ShouldRenderCurrentFrame = false;

  // Submit the composition layers for the predicted display time.
  // If the frame shouldn't be rendered, submit an empty vector
  XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
  frameEndInfo.displayTime = this->PredictedDisplayTime;
  frameEndInfo.environmentBlendMode = this->EnvironmentBlendMode;
  frameEndInfo.layerCount = (uint32_t)layers.size();
  frameEndInfo.layers = layers.data();
  xrEndFrame(this->Session, &frameEndInfo);

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::PollEvent(XrEventDataBuffer& eventData)
{
  eventData.type = XR_TYPE_EVENT_DATA_BUFFER;
  eventData.next = nullptr;
  return xrPollEvent(this->Instance, &eventData) == XR_SUCCESS;
}

//------------------------------------------------------------------------------
uint32_t vtkOpenXRManager::WaitAndAcquireSwapchainImage(const XrSwapchain& swapchainHandle)
{
  VTK_CHECK_NULL_XRHANDLE(
    swapchainHandle, "vtkOpenXRManager::WaitAndAcquireSwapchainImage, swapchain");

  uint32_t swapchainImageIndex;
  XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
  this->XrCheckWarn(xrAcquireSwapchainImage(swapchainHandle, &acquireInfo, &swapchainImageIndex),
    "Failed to acquire swapchain image !");

  XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
  waitInfo.timeout = XR_INFINITE_DURATION;
  this->XrCheckWarn(
    xrWaitSwapchainImage(swapchainHandle, &waitInfo), "Failed to wait swapchain image !");

  return swapchainImageIndex;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::XrCheckError(const XrResult& result, const std::string& message)
{
  if (XR_FAILED(result))
  {
    char xRResultString[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(this->Instance, result, xRResultString);
    vtkErrorWithObjectMacro(nullptr, << message << " [" << xRResultString << "].");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::XrCheckWarn(const XrResult& result, const std::string& message)
{
  if (XR_FAILED(result))
  {
    char xRResultString[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(this->Instance, result, xRResultString);
    vtkWarningWithObjectMacro(nullptr, << message << " [" << xRResultString << "].");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenXRManager::PrintInstanceProperties()
{
  XrInstanceProperties instanceProperties = {
    XR_TYPE_INSTANCE_PROPERTIES, // .type
    nullptr,                     // .next
  };

  this->XrCheckWarn(
    xrGetInstanceProperties(this->Instance, &instanceProperties), "Failed to get instance info");

  std::cout << "Runtime Name: " << instanceProperties.runtimeName;
  std::cout << "Runtime Version: " << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "."
            << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "."
            << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << std::endl;
}

//------------------------------------------------------------------------------
void vtkOpenXRManager::PrintSystemProperties(XrSystemProperties* systemProperties)
{
  std::cout << "System Properties for system id:" << systemProperties->systemId << ", with name \""
            << systemProperties->systemName << "\""
            << ", vendorID=" << systemProperties->vendorId << std::endl;

  std::cout << "\tMax Layers          : " << systemProperties->graphicsProperties.maxLayerCount
            << std::endl;
  std::cout << "\tMax Swapchain Height: "
            << systemProperties->graphicsProperties.maxSwapchainImageHeight << std::endl;
  std::cout << "\tMax Swapchain Width : "
            << systemProperties->graphicsProperties.maxSwapchainImageWidth << std::endl;
  std::cout << "\tOrientation Tracking: "
            << (systemProperties->trackingProperties.orientationTracking ? "True" : "False")
            << std::endl;
  std::cout << "\tPosition Tracking   : "
            << (systemProperties->trackingProperties.positionTracking ? "True" : "False")
            << std::endl;

  const XrBaseInStructure* next = static_cast<XrBaseInStructure*>(systemProperties->next);
  while (next)
  {
    if (next->type == XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT)
    {
      XrSystemHandTrackingPropertiesEXT* ht =
        static_cast<XrSystemHandTrackingPropertiesEXT*>(systemProperties->next);
      std::cout << "\tHand Tracking       : " << ht->supportsHandTracking << std::endl;
    }
    next = next->next;
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRManager::PrintSupportedViewConfigs()
{
  uint32_t viewConfigCount;
  this->XrCheckWarn(
    xrEnumerateViewConfigurations(this->Instance, this->SystemId, 0, &viewConfigCount, nullptr),
    "Failed to get view configuration count");

  std::cout << "Runtime supports " << viewConfigCount << " view configurations" << std::endl;

  std::vector<XrViewConfigurationType> viewConfigs(viewConfigCount);
  this->XrCheckWarn(xrEnumerateViewConfigurations(this->Instance, this->SystemId, viewConfigCount,
                      &viewConfigCount, viewConfigs.data()),
    "Failed to enumerate view configurations!");

  for (uint32_t i = 0; i < viewConfigCount; ++i)
  {
    XrViewConfigurationProperties props = { XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
    this->XrCheckWarn(
      xrGetViewConfigurationProperties(this->Instance, this->SystemId, viewConfigs[i], &props),
      "Failed to get view configuration info " + i);

    std::cout << "Type "
              << vtkOpenXRUtilities::GetViewConfigurationTypeAsString(props.viewConfigurationType)
              << ": FOV mutable: " << (props.fovMutable ? "True" : "False") << std::endl;
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRManager::PrintViewConfigViewInfo(
  const std::vector<XrViewConfigurationView>& viewconfigViews)
{
  for (size_t i = 0; i < viewconfigViews.size(); ++i)
  {
    const auto& vcfgv = viewconfigViews[i];
    std::cout << "View Configuration View " << i << std::endl;
    std::cout << "\tResolution       : Recommended: " << vcfgv.recommendedImageRectWidth << "x"
              << vcfgv.recommendedImageRectHeight << ", Max: " << vcfgv.maxImageRectWidth << "x"
              << vcfgv.maxImageRectHeight << std::endl;
    std::cout << "\tSwapchain Samples: Recommended: " << vcfgv.recommendedSwapchainSampleCount
              << ", Max: " << vcfgv.maxSwapchainSampleCount << std::endl;
  }
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::PrintReferenceSpaces()
{
  uint32_t refSpaceCount;
  this->XrCheckError(xrEnumerateReferenceSpaces(this->Session, 0, &refSpaceCount, nullptr),
    "Getting number of reference spaces failed!");

  std::vector<XrReferenceSpaceType> refSpaces(refSpaceCount);
  this->XrCheckError(
    xrEnumerateReferenceSpaces(this->Session, refSpaceCount, &refSpaceCount, refSpaces.data()),
    "Enumerating reference spaces failed!");

  std::cout << "Runtime supports " << refSpaceCount << " reference spaces:" << std::endl;
  for (uint32_t i = 0; i < refSpaceCount; i++)
  {
    if (refSpaces[i] == XR_REFERENCE_SPACE_TYPE_LOCAL)
    {
      std::cout << "\tXR_REFERENCE_SPACE_TYPE_LOCAL" << std::endl;
    }
    else if (refSpaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE)
    {
      std::cout << "\tXR_REFERENCE_SPACE_TYPE_STAGE" << std::endl;
    }
    else if (refSpaces[i] == XR_REFERENCE_SPACE_TYPE_VIEW)
    {
      std::cout << "\tXR_REFERENCE_SPACE_TYPE_VIEW" << std::endl;
    }
    else
    {
      std::cout << "\tOther (extension?) refspace : " << refSpaces[i] << std::endl;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
std::vector<const char*> vtkOpenXRManager::SelectExtensions()
{
  // Fetch the list of extensions supported by the runtime.
  uint32_t extensionCount;
  this->XrCheckError(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
    "Failed to enumerate number of extension properties");

  std::vector<XrExtensionProperties> extensionProperties(
    extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
  this->XrCheckError(xrEnumerateInstanceExtensionProperties(
                       nullptr, extensionCount, &extensionCount, extensionProperties.data()),
    "Failed to enumerate extension properties");

  std::vector<const char*> enabledExtensions;
  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto EnableExtensionIfSupported = [&](const char* extensionName) {
    for (uint32_t i = 0; i < extensionCount; i++)
    {
      if (strcmp(extensionProperties[i].extensionName, extensionName) == 0)
      {
        enabledExtensions.push_back(extensionName);
        return true;
      }
    }
    return false;
  };

  // Don't forget here to use the name of the extension (uppercase with suffix EXTENSION_NAME)
  this->HasOpenGLExtension = EnableExtensionIfSupported(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

  this->OptionalExtensions.ControllerModelExtensionSupported =
    EnableExtensionIfSupported(XR_MSFT_CONTROLLER_MODEL_EXTENSION_NAME);

  EnableExtensionIfSupported(XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME);

  this->PrintOptionalExtensions();

  return enabledExtensions;
}

//------------------------------------------------------------------------------
void vtkOpenXRManager::PrintOptionalExtensions()
{
  if (this->OptionalExtensions.DepthExtensionSupported)
  {
    std::cout << "Optional extensions DepthExtension is supported" << std::endl;
  }
  if (this->OptionalExtensions.ControllerModelExtensionSupported)
  {
    std::cout << "Optional extensions ControllerModelExtensionSupported is supported" << std::endl;
  }
  if (this->OptionalExtensions.UnboundedRefSpaceSupported)
  {
    std::cout << "Optional extensions UnboundedRefSpaceSupported is supported" << std::endl;
  }
  if (this->OptionalExtensions.SpatialAnchorSupported)
  {
    std::cout << "Optional extensions SpatialAnchorSupported is supported" << std::endl;
  }
  if (this->OptionalExtensions.HandTrackingSupported)
  {
    std::cout << "Optional extensions HandTrackingSupported is supported" << std::endl;
  }
}

//------------------------------------------------------------------------------
// Instance and extensions
//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateInstance()
{
  // Start by selection available extensions
  const std::vector<const char*> enabledExtensions = this->SelectExtensions();

  // For instance, only OpenGL extension is supported so is mandatory
  if (!this->HasOpenGLExtension)
  {
    vtkErrorWithObjectMacro(nullptr, << "OpenGL extension is not supported. Aborting.");
    return false;
  }

  // Create the instance with enabled extensions.
  XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
  createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
  createInfo.enabledExtensionNames = enabledExtensions.data();

  XrApplicationInfo applicationInfo = {
    "OpenXR with VTK",      // .applicationName
    1,                      // .applicationVersion
    "",                     // .engineName
    1,                      // .engineVersion
    XR_CURRENT_API_VERSION, // .apiVersion
  };

  createInfo.applicationInfo = applicationInfo;

  if (!this->XrCheckError(
        xrCreateInstance(&createInfo, &this->Instance), "Failed to create XR instance."))
  {
    return false;
  }

  // This will define the pointer function of enabled extensions (see XrExtensions.h)
  this->Extensions.PopulateDispatchTable(this->Instance);

  this->PrintInstanceProperties();

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateSubactionPaths()
{
  if (!this->XrCheckError(xrStringToPath(this->Instance, "/user/hand/left",
                            &this->SubactionPaths[vtkOpenXRManager::ControllerIndex::Left]),
        "Failed to create left hand subaction path"))
  {
    return false;
  }
  if (!this->XrCheckError(xrStringToPath(this->Instance, "/user/hand/right",
                            &this->SubactionPaths[vtkOpenXRManager::ControllerIndex::Right]),
        "Failed to create right hand subaction path"))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// System
//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateSystem()
{
  VTK_CHECK_NULL_XRHANDLE(this->Instance, "vtkOpenXRManager::CreateSystem, Instance");

  // --- Create XrSystem
  XrSystemGetInfo system_get_info = {
    XR_TYPE_SYSTEM_GET_INFO, // .type
    nullptr,                 // .next
    this->FormFactor,        // .formFactor
  };

  this->XrCheckError(xrGetSystem(this->Instance, &system_get_info, &this->SystemId),
    "Failed to get system for HMD form factor.");

  vtkDebugWithObjectMacro(
    nullptr, "Successfully got XrSystem with id " << this->SystemId << " for HMD form factor.");

  // checking system properties is generally  optional, but we are interested in hand tracking
  // support
  {
    XrSystemProperties systemProperties = {
      XR_TYPE_SYSTEM_PROPERTIES, // .type
      nullptr,                   // .next
      XR_NULL_SYSTEM_ID,         // .systemId
      0,                         // .vendorId
      "",                        // .systemName
      { 0 },                     // .graphicsProperties,
      { 0 }                      // .trackingProperties,
    };

    XrSystemHandTrackingPropertiesEXT ht = {
      XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT, // .type
      nullptr,                                     // .next
      false                                        // .supportsHandTracking
    };

    if (this->OptionalExtensions.HandTrackingSupported)
    {
      systemProperties.next = &ht;
    }

    this->XrCheckError(xrGetSystemProperties(this->Instance, this->SystemId, &systemProperties),
      "Failed to get System properties");

    this->OptionalExtensions.HandTrackingSupported =
      this->OptionalExtensions.HandTrackingSupported && ht.supportsHandTracking;

    this->PrintSystemProperties(&systemProperties);
  }

  // Choose an environment blend mode
  {
    // Query the list of supported environment blend modes for the current system
    uint32_t count;

    xrEnumerateEnvironmentBlendModes(
      this->Instance, this->SystemId, this->ViewType, 0, &count, nullptr);
    this->XrCheckError(xrEnumerateEnvironmentBlendModes(
                         this->Instance, this->SystemId, this->ViewType, 0, &count, nullptr),
      "Failed to get environment blend modes count");
    if (count == 0)
    {
      vtkErrorWithObjectMacro(
        nullptr, "A system must support at least one environment blend mode.");
    }

    std::vector<XrEnvironmentBlendMode> environmentBlendModes(count);
    xrEnumerateEnvironmentBlendModes(
      this->Instance, this->SystemId, this->ViewType, count, &count, environmentBlendModes.data());
    this->XrCheckError(xrEnumerateEnvironmentBlendModes(this->Instance, this->SystemId,
                         this->ViewType, count, &count, environmentBlendModes.data()),
      "Failed to enumerate environment blend modes");

    // Pick the system's preferred one
    this->EnvironmentBlendMode = environmentBlendModes[0];
  }

  this->PrintSupportedViewConfigs();

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CheckGraphicsRequirements()
{
  XrGraphicsRequirementsOpenGLKHR openGLReqs = {
    XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR, // .type
    nullptr,                                  // .next
    0,                                        // .minApiVersionSupported
    0                                         // .maxApiVersionSupported
  };

  // this function pointer was loaded with xrGetInstanceProcAddr (see XrExtensions.h)
  if (!this->XrCheckError(this->Extensions.xrGetOpenGLGraphicsRequirementsKHR(
                            this->Instance, this->SystemId, &openGLReqs),
        "Failed to get OpenGL graphics requirements!"))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateGraphicsBinding(vtkOpenGLRenderWindow* helperWindow)
{
#ifdef VTK_USE_X
  // Create the XrGraphicsBindingOpenGLXlibKHR structure
  // That will be in the next chain of the XrSessionCreateInfo
  // We need to fill xDisplay, visualId, glxFBConfig, glxDrawable and glxContext

  auto graphicsBindingGLX =
    std::shared_ptr<XrGraphicsBindingOpenGLXlibKHR>(new XrGraphicsBindingOpenGLXlibKHR{
      XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR, // .type
      nullptr,                                  // .next
      nullptr,                                  // .xDisplay. a valid X11 display
      0,                                        // .visualid. a valid X11 visual id
      0,                                        // .glxFBConfig. a valid X11 OpenGL GLX GLXFBConfig
      0,                                        // .glxDrawable. a valid X11 OpenGL GLX GLXDrawable
      0                                         // .glxContext. a valid X11 OpenGL GLX GLXContext
    });
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

#elif _WIN32
  auto graphicsBindingGLWin32 =
    std::shared_ptr<XrGraphicsBindingOpenGLWin32KHR>(new XrGraphicsBindingOpenGLWin32KHR{
      XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, // .type
      nullptr,                                   // .next
      { 0 },                                     // .hdC : a valid Windows HW device context handle.
      { 0 } // . hGLRChandle : a valid Windows OpenGL rendering context
    });
  this->GraphicsBinding = graphicsBindingGLWin32;

  graphicsBindingGLWin32->hDC = wglGetCurrentDC();
  graphicsBindingGLWin32->hGLRC = wglGetCurrentContext();

#else
  vtkErrorWithObjectMacro(nullptr, << "Only X11 and Win32 are supported at the moment.");
  return false;
#endif

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateSession()
{
  VTK_CHECK_NULL_XRHANDLE(this->Instance, "vtkOpenXRManager::CreateSession, Instance");

  // --- Create session
  this->SessionState = XR_SESSION_STATE_UNKNOWN;

  XrSessionCreateInfo sessionCreateInfo = {
    XR_TYPE_SESSION_CREATE_INFO, // .type
    this->GraphicsBinding.get(), // .next
    0,                           // .createFlags
    this->SystemId               // .systemId
  };

  if (!this->XrCheckError(xrCreateSession(this->Instance, &sessionCreateInfo, &this->Session),
        "Failed to create session"))
  {
    return false;
  }

#ifdef XR_USE_GRAPHICS_API_OPENGL
  vtkDebugWithObjectMacro(nullptr, "Successfully created a session with OpenGL!");
#elif XR_USE_GRAPHICS_API_D3D11
  vtkDebugWithObjectMacro(nullptr, "Successfully created a session with DirectX!");
#endif

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateReferenceSpace()
{
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::CreateReferenceSpace, Session");

  // Many runtimes support at least STAGE and LOCAL but not all do.
  // Sophisticated apps might check if the chosen one is supported and try another one if not.
  // Here we will get an error from xrCreateReferenceSpace() and exit.
  if (!this->PrintReferenceSpaces())
  {
    return false;
  }

  XrReferenceSpaceCreateInfo refSpaceCreateInfo = {
    XR_TYPE_REFERENCE_SPACE_CREATE_INFO,  // .type
    nullptr,                              // .next
    this->ReferenceSpaceType,             // .referenceSpaceType
    vtkOpenXRUtilities::GetIdentityPose() // .poseInReferenceSpace
  };

  this->XrCheckError(
    xrCreateReferenceSpace(this->Session, &refSpaceCreateInfo, &this->ReferenceSpace),
    "Failed to create play space!");

  return true;
}

//------------------------------------------------------------------------------
const std::vector<int64_t>& vtkOpenXRManager::GetSupportedColorFormats()
{
  const static std::vector<int64_t> supportedColorFormats = { GL_RGBA32F, GL_RGBA16F, GL_RGBA16,
    GL_SRGB8_ALPHA8_EXT };

  return supportedColorFormats;
}

//------------------------------------------------------------------------------
const std::vector<int64_t>& vtkOpenXRManager::GetSupportedDepthFormats()
{
  const static std::vector<int64_t> supportedDepthFormats = { GL_DEPTH_COMPONENT16,
    GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT32F };

  return supportedDepthFormats;
}

//------------------------------------------------------------------------------
std::tuple<int64_t, int64_t> vtkOpenXRManager::SelectSwapchainPixelFormats()
{
  // Query the runtime's preferred swapchain formats.
  uint32_t swapchainFormatsCount;
  this->XrCheckError(xrEnumerateSwapchainFormats(this->Session, 0, &swapchainFormatsCount, nullptr),
    "Failed to get number of supported swapchain formats");

  vtkDebugWithObjectMacro(
    nullptr, "Runtime supports " << swapchainFormatsCount << " swapchain formats");

  std::vector<int64_t> swapchainFormats(swapchainFormatsCount);
  this->XrCheckError(xrEnumerateSwapchainFormats(this->Session, swapchainFormatsCount,
                       &swapchainFormatsCount, swapchainFormats.data()),
    "Failed to enumerate swapchain formats");

  // Choose the first runtime-preferred format that this app supports.
  auto selectPixelFormat = [&](const std::vector<int64_t>& runtimePreferredFormats,
                             const std::vector<int64_t>& applicationSupportedFormats,
                             const std::string& formatName) {
    auto found =
      std::find_first_of(std::begin(runtimePreferredFormats), std::end(runtimePreferredFormats),
        std::begin(applicationSupportedFormats), std::end(applicationSupportedFormats));
    if (found == std::end(runtimePreferredFormats))
    {
      vtkErrorWithObjectMacro(
        nullptr, << "No runtime swapchain " << formatName << " format in the list is supported.");
      return (int64_t)-1;
    }
    return *found;
  };

  int64_t colorSwapchainFormat =
    selectPixelFormat(swapchainFormats, this->GetSupportedColorFormats(), "color");
  int64_t depthSwapchainFormat = -1;
  if (this->OptionalExtensions.DepthExtensionSupported)
  {
    depthSwapchainFormat =
      selectPixelFormat(swapchainFormats, this->GetSupportedDepthFormats(), "depth");
    if (depthSwapchainFormat == -1)
    {
      vtkDebugWithObjectMacro(
        nullptr, "Disabling depth extension as no depth format are supported");
      this->OptionalExtensions.DepthExtensionSupported = false;
    }
  }

  return std::make_tuple(colorSwapchainFormat, depthSwapchainFormat);
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateSwapchains()
{
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::CreateSwapchains, Session");

  this->RenderResources = std::unique_ptr<RenderResources_t>(new RenderResources_t());

  // Select color and depth swapchain pixel formats.
  int64_t colorSwapchainFormat, depthSwapchainFormat;
  std::tie(colorSwapchainFormat, depthSwapchainFormat) = this->SelectSwapchainPixelFormats();

  // Query and cache view configuration views.
  this->CreateConfigViews();

  const XrViewConfigurationView& view = this->RenderResources->ConfigViews[0];

  // Use the system's recommended rendering parameters.
  const uint32_t imageRectWidth = view.recommendedImageRectWidth;
  const uint32_t imageRectHeight = view.recommendedImageRectHeight;
  const uint32_t swapchainSampleCount = view.recommendedSwapchainSampleCount;

  // Create swapchains with texture array for color and depth images.
  const uint32_t viewCount = (uint32_t)this->RenderResources->ConfigViews.size();

  // One swapchain per view to make it simple
  // We could also use a texture arraySize != 1 but the rendering
  // will be more complex
  this->RenderResources->ColorSwapchains.resize(viewCount);
  this->RenderResources->DepthSwapchains.resize(viewCount);
  for (uint32_t i = 0; i < viewCount; ++i)
  {
    this->RenderResources->ColorSwapchains[i] = this->CreateSwapchainOpenGL(colorSwapchainFormat,
      imageRectWidth, imageRectHeight, swapchainSampleCount, 0 /*createFlags*/,
      XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT);

    if (this->OptionalExtensions.DepthExtensionSupported)
    {
      this->RenderResources->DepthSwapchains[i] = this->CreateSwapchainOpenGL(depthSwapchainFormat,
        imageRectWidth, imageRectHeight, swapchainSampleCount, 0 /*createFlags*/,
        XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }
  }

  // Preallocate view buffers for xrLocateViews later inside frame loop.
  this->RenderResources->Views.resize(viewCount, { XR_TYPE_VIEW });

  // Preallocate projection layer views and depth if needed
  this->RenderResources->ProjectionLayerViews.resize(viewCount);
  if (this->OptionalExtensions.DepthExtensionSupported)
  {
    this->RenderResources->DepthInfoViews.resize(viewCount);
  }

  return true;
}

//------------------------------------------------------------------------------
vtkOpenXRManager::SwapchainOpenGL_t vtkOpenXRManager::CreateSwapchainOpenGL(int64_t format,
  uint32_t width, uint32_t height, uint32_t sampleCount, XrSwapchainCreateFlags createFlags,
  XrSwapchainUsageFlags usageFlags)
{
  SwapchainOpenGL_t swapchain;
  swapchain.Format = format;
  swapchain.Width = width;
  swapchain.Height = height;

  XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
  swapchainCreateInfo.arraySize = 1;
  swapchainCreateInfo.format = format;
  swapchainCreateInfo.width = width;
  swapchainCreateInfo.height = height;
  swapchainCreateInfo.mipCount = 1;
  swapchainCreateInfo.faceCount = 1;
  swapchainCreateInfo.sampleCount = sampleCount;
  swapchainCreateInfo.createFlags = createFlags;
  swapchainCreateInfo.usageFlags = usageFlags;

  this->XrCheckError(xrCreateSwapchain(this->Session, &swapchainCreateInfo, &swapchain.Swapchain),
    "Failed to create swapchain!");

  uint32_t chainLength;
  this->XrCheckError(xrEnumerateSwapchainImages(swapchain.Swapchain, 0, &chainLength, nullptr),
    "Failed to get swapchain images count");

  swapchain.Images.resize(chainLength, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
  this->XrCheckError(
    xrEnumerateSwapchainImages(swapchain.Swapchain, (uint32_t)swapchain.Images.size(), &chainLength,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchain.Images.data())),
    "Failed to enumerate swapchain images");

  return swapchain;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateConfigViews()
{
  uint32_t viewCount;
  this->XrCheckError(xrEnumerateViewConfigurationViews(
                       this->Instance, this->SystemId, this->ViewType, 0, &viewCount, nullptr),
    "Failed to get view configuration view count!");
  if (viewCount != this->StereoViewCount)
  {
    vtkWarningWithObjectMacro(nullptr, << "StereoViewCount (" << this->StereoViewCount
                                       << ") is different than viewCount (" << viewCount << ")");
  }

  this->RenderResources->ConfigViews.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });

  if (!this->XrCheckError(
        xrEnumerateViewConfigurationViews(this->Instance, this->SystemId, this->ViewType, viewCount,
          &viewCount, this->RenderResources->ConfigViews.data()),
        "Failed to enumerate view configuration views!"))
  {
    return false;
  }

  this->PrintViewConfigViewInfo(this->RenderResources->ConfigViews);

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateActionSet(
  const std::string& actionSetName, const std::string& localizedActionSetName)
{
  vtkDebugWithObjectMacro(
    nullptr, "Create action set " << actionSetName << ": " << localizedActionSetName);

  XrActionSetCreateInfo actionSetInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };

  strcpy(actionSetInfo.actionSetName, actionSetName.c_str());
  strcpy(actionSetInfo.localizedActionSetName, localizedActionSetName.c_str());

  XrActionSet actionSet;
  if (!this->XrCheckError(xrCreateActionSet(this->Instance, &actionSetInfo, &actionSet),
        "Failed to create default actionset"))
  {
    return false;
  }
  this->ActionSets.push_back(actionSet);

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::SelectActiveActionSet(unsigned int index)
{
  if (this->ActionSets.size() == 0)
  {
    vtkErrorWithObjectMacro(nullptr, << "An action set must be created prior to select one.");
    return false;
  }
  if (index >= this->ActionSets.size())
  {
    vtkWarningWithObjectMacro(nullptr,
      << "The selected action set at index : " << index << " does not exist. Pick the first one");
    index = 0;
  }

  this->ActiveActionSet = &this->ActionSets[index];

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::AttachSessionActionSets()
{
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::AttachSessionActionSets, Session");

  XrSessionActionSetsAttachInfo actionSetsAttachInfo = {
    XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO, // .type
    nullptr,                                 // .next
    (uint32_t)this->ActionSets.size(),       // .countActionSets
    this->ActionSets.data()                  // .actionSets
  };
  return this->XrCheckError(xrAttachSessionActionSets(this->Session, &actionSetsAttachInfo),
    "Failed to attach action sets");
}

//------------------------------------------------------------------------------
void vtkOpenXRManager::DestroyActionSets()
{
  for (XrActionSet actionSet : this->ActionSets)
  {
    xrDestroyActionSet(actionSet);
  }

  this->ActionSets.clear();

  // active action set pointed to one of those, so clear it now
  this->ActiveActionSet = nullptr;
}

//------------------------------------------------------------------------------
XrPath vtkOpenXRManager::GetXrPath(const std::string& path)
{
  VTK_CHECK_NULL_XRHANDLE(this->Instance, "vtkOpenXRManager::GetXrPath, Instance");

  XrPath xrPath;
  this->XrCheckWarn(
    xrStringToPath(this->Instance, path.c_str(), &xrPath), "Failed to get path " + path);
  return std::move(xrPath);
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateOneAction(
  Action_t& actionT, const std::string& name, const std::string& localizedName)
{
  if (this->ActiveActionSet == nullptr)
  {
    return false;
  }

  XrActionCreateInfo actionInfo = {
    XR_TYPE_ACTION_CREATE_INFO,            // .type
    nullptr,                               // .next
    "",                                    // .actionName
    actionT.ActionType,                    // .actionType
    (uint32_t)this->SubactionPaths.size(), // .countSubactionPaths
    this->SubactionPaths.data(),           // .subactionPaths
    ""                                     // .localizedActionName
  };
  strcpy(actionInfo.actionName, name.c_str());
  strcpy(actionInfo.localizedActionName, localizedName.c_str());

  if (!this->XrCheckError(xrCreateAction(*this->ActiveActionSet, &actionInfo, &actionT.Action),
        "Failed to create action " + std::string(name)))
  {
    return false;
  }

  // If this is a pose action, we need to create an action space
  // In order to use LocateSpace
  if (actionT.ActionType == XR_ACTION_TYPE_POSE_INPUT)
  {
    // One action space per pointer pose and store it in subaction space
    for (uint32_t hand :
      { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
    {
      if (!this->CreateOneActionSpace(actionT.Action, this->SubactionPaths[hand],
            vtkOpenXRUtilities::GetIdentityPose(), actionT.PoseSpaces[hand]))
      {
        vtkErrorWithObjectMacro(nullptr,
          << "Failed to create pose action space for "
          << (hand == vtkOpenXRManager::ControllerIndex::Left ? "left" : "right") << " hand");
        return false;
      };
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::CreateOneActionSpace(const XrAction& action, const XrPath& subactionPath,
  const XrPosef& poseInActionSpace, XrSpace& space)
{
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::CreateOneActionSpace, Session");

  XrActionSpaceCreateInfo actionSpaceInfo = {
    XR_TYPE_ACTION_SPACE_CREATE_INFO, // .type
    nullptr,                          // .next
    action,                           // .action
    subactionPath,                    // .subactionPath
    poseInActionSpace                 // .poseInActionSpace
  };

  return this->XrCheckError(xrCreateActionSpace(this->Session, &actionSpaceInfo, &space), "");
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::SuggestActions(
  const std::string& profile, std::vector<XrActionSuggestedBinding>& actionSuggestedBindings)
{
  vtkDebugWithObjectMacro(nullptr, "SuggestActions for profile : " << profile);
  VTK_CHECK_NULL_XRHANDLE(this->Instance, "vtkOpenXRManager::SuggestActions, Instance");

  XrPath interactionProfilePath;
  this->XrCheckWarn(xrStringToPath(this->Instance, profile.c_str(), &interactionProfilePath),
    "Failed to get interaction profile path " + profile);

  const XrInteractionProfileSuggestedBinding suggestedBindings = {
    XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING, // .type
    nullptr,                                       // .next
    interactionProfilePath,                        // .interactionProfile
    (uint32_t)actionSuggestedBindings.size(),      // .countSuggestedBindings
    actionSuggestedBindings.data()                 // .suggestedBindings
  };

  this->XrCheckWarn(xrSuggestInteractionProfileBindings(this->Instance, &suggestedBindings),
    "Failed to suggest actions");

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::SyncActions()
{
  if (this->ActiveActionSet == nullptr)
  {
    return false;
  }
  const XrActionSet& actionSet = *this->ActiveActionSet;
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::SyncActions, Session");
  VTK_CHECK_NULL_XRHANDLE(actionSet, "vtkOpenXRManager::SyncActions, ActiveActionSet");

  // Only use the active action set, but we could add all action sets
  // in the following vector
  std::vector<XrActiveActionSet> activeActionSets = { { actionSet, XR_NULL_PATH } };
  XrActionsSyncInfo syncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
  syncInfo.countActiveActionSets = (uint32_t)activeActionSets.size();
  syncInfo.activeActionSets = activeActionSets.data();
  return this->XrCheckError(xrSyncActions(this->Session, &syncInfo), "Failed to sync actions");
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::UpdateActionData(Action_t& action_t, const int hand)
{
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::UpdateActionData, Session");
  VTK_CHECK_NULL_XRHANDLE(
    this->ReferenceSpace, "vtkOpenXRManager::UpdateActionData, ReferenceSpace");

  XrActionStateGetInfo info = {
    XR_TYPE_ACTION_STATE_GET_INFO, // .type
    nullptr,                       // .next
    action_t.Action,               // .action
    this->SubactionPaths[hand],    // .subactionPath
  };

  // we store the state of the action, depending on the selected hand
  switch (action_t.ActionType)
  {
    case XR_ACTION_TYPE_FLOAT_INPUT:
      action_t.States[hand]._float.type = XR_TYPE_ACTION_STATE_FLOAT;
      action_t.States[hand]._float.next = nullptr;
      if (!this->XrCheckError(xrGetActionStateFloat(Session, &info, &action_t.States[hand]._float),
            "Failed to get float value"))
      {
        return false;
      }
      break;
    case XR_ACTION_TYPE_BOOLEAN_INPUT:
      action_t.States[hand]._boolean.type = XR_TYPE_ACTION_STATE_BOOLEAN;
      action_t.States[hand]._boolean.next = nullptr;
      if (!this->XrCheckError(
            xrGetActionStateBoolean(this->Session, &info, &action_t.States[hand]._boolean),
            "Failed to get boolean value"))
      {
        return false;
      }
      break;
    case XR_ACTION_TYPE_VECTOR2F_INPUT:
      action_t.States[hand]._vec2f.type = XR_TYPE_ACTION_STATE_VECTOR2F;
      action_t.States[hand]._vec2f.next = nullptr;
      if (!this->XrCheckError(
            xrGetActionStateVector2f(this->Session, &info, &action_t.States[hand]._vec2f),
            "Failed to get vec2f"))
      {
        return false;
      }
      break;
    case XR_ACTION_TYPE_POSE_INPUT:
      action_t.States[hand]._pose.type = XR_TYPE_ACTION_STATE_POSE;
      action_t.States[hand]._pose.next = nullptr;
      if (!this->XrCheckError(
            xrGetActionStatePose(this->Session, &info, &action_t.States[hand]._pose),
            "Failed to get action state pose"))
      {
        return false;
      }

      if (action_t.States[hand]._pose.isActive)
      {
        action_t.PoseLocations[hand].type = XR_TYPE_SPACE_LOCATION;
        action_t.PoseLocations[hand].next = nullptr;

        if (this->StorePoseVelocities)
        {
          action_t.PoseVelocities[hand].type = XR_TYPE_SPACE_VELOCITY;
          action_t.PoseVelocities[hand].next = nullptr;
          action_t.PoseLocations[hand].next = &action_t.PoseVelocities[hand];
        }

        // Store the position of the hand
        if (!this->XrCheckError(xrLocateSpace(action_t.PoseSpaces[hand], this->ReferenceSpace,
                                  this->PredictedDisplayTime, &action_t.PoseLocations[hand]),
              "Failed to locate hand space"))
        {
          return false;
        }
      }

      break;
    default:
      break;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRManager::ApplyVibration(const Action_t& actionT, const int hand,
  const float amplitude, const float duration, const float frequency)
{
  VTK_CHECK_NULL_XRHANDLE(this->Session, "vtkOpenXRManager::ApplyVibration, Session");

  if (actionT.ActionType != XR_ACTION_TYPE_VIBRATION_OUTPUT)
  {
    vtkErrorWithObjectMacro(
      nullptr, << "vtkOpenXRManager::ApplyVibration must be called for an action of type "
                  "XR_ACTION_TYPE_VIBRATION_OUTPUT, not a "
               << vtkOpenXRUtilities::GetActionTypeAsString(actionT.ActionType));
    return false;
  }

  XrHapticActionInfo actionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
  actionInfo.action = actionT.Action;
  actionInfo.subactionPath = this->SubactionPaths[hand];

  XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
  vibration.amplitude = amplitude;
  vibration.duration = duration;
  vibration.frequency = frequency;

  if (!this->XrCheckError(
        xrApplyHapticFeedback(this->Session, &actionInfo, (XrHapticBaseHeader*)&vibration),
        "Failed to apply haptic feedback"))
  {
    return false;
  }
  return true;
}
