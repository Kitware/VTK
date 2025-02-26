// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenXRManager
 * @brief   Singleton class that holds a collection of utility functions
 *          and member variables to communicate with the OpenXR runtime
 *
 * vtkOpenXRManager is not a vtkObject, the singleton unique instance gets
 * allocated on the stack the first time vtkOpenXRManager::GetInstance() is
 * called.
 */

#ifndef vtkOpenXRManager_h
#define vtkOpenXRManager_h

#include "vtkRenderingOpenXRModule.h" // needed for exports

#include "vtkNew.h"
#include "vtkOpenXR.h"
#include "vtkOpenXRManagerConnection.h"
#include "vtkOpenXRManagerGraphics.h"
#include "vtkSmartPointer.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkOpenXRRenderWindow;
class vtkOpenXRSceneObserver;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRManager
{
public:
  ///@{
  /**
   * Return the singleton instance.
   */
  static vtkOpenXRManager& GetInstance()
  {
    static vtkOpenXRManager UniqueInstance;
    return UniqueInstance;
  }
  ///@}

  enum OutputLevel
  {
    DebugOutput = 0,
    WarningOutput = 1,
    ErrorOutput = 2
  };

  ///@{
  /**
   * Utility function to check the XrResult, print the result message
   * as a debug, warning or error message if the result failed.
   */
  bool XrCheckOutput(OutputLevel level, const XrResult&, const std::string& message);
  ///@}

  /**
   * Structure representing OpenXR instance version
   */
  struct VTKRENDERINGOPENXR_EXPORT InstanceVersion
  {
    std::uint16_t Major{};
    std::uint16_t Minor{};
    std::uint32_t Patch{};
  };

  /**
   * Utility function to get XrInstance runtime version for given ConnectionStrategy
   * This function creates a XrInstance, which may have a significant runtime overhead.
   */
  static InstanceVersion QueryInstanceVersion(vtkOpenXRManagerConnection* cs);

  ///@{
  /**
   * Utility functions to print information about OpenXR manager internal structures.
   */
  void PrintInstanceProperties();
  void PrintSystemProperties(XrSystemProperties* system_properties);
  void PrintSupportedViewConfigs();
  void PrintViewConfigViewInfo(const std::vector<XrViewConfigurationView>&);
  bool PrintReferenceSpaces();
  ///@}

  ///@{
  /**
   * Internal API. Managed by `vtkOpenXRRenderWindow`.
   *
   * Initialize the OpenXR SDK to render images in a virtual reality device.
   * The HelperWindow of xrWindow must be a vtkWin32OpenGLRenderWindow if the platform is Win32,
   * else a vtkXOpenGLRenderWindow if the platform is X.
   */
  bool Initialize(vtkOpenXRRenderWindow* xrWindow);
  ///@}

  ///@{
  /**
   * Internal API. Managed by `vtkOpenXRRenderWindow`.
   *
   * End the OpenXR session and destroy it and the OpenXR instance.
   */
  void Finalize();
  ///@}

  ///@{
  /**
   * Return as a tuple the OpenXR recommended texture size to be sent to the device.
   */
  std::tuple<uint32_t, uint32_t> GetRecommendedImageRectSize();
  ///@}

  ///@{
  /**
   * Return the recommended swapchain sample count.
   */
  uint32_t GetRecommendedSampleCount();
  ///@}

  /**
   * Return the number of OpenXR views (typically one per physical display / eye)
   */
  uint32_t GetViewCount()
  {
    return static_cast<uint32_t>(this->RenderResources->ConfigViews.size());
  }

  ///@{
  /**
   * Return the OpenXR properties as a string, with format
   * "RuntimeName MAJOR.MINOR.PATCH"
   */
  std::string GetOpenXRPropertiesAsString();
  ///@}

  ///@{
  /**
   * Returns a pointer to the view pose that contains the view orientation
   * and position for the specified eye, or nullptr if eye exceeds or equals
   * the number of configured views.  This class retains responsibility for
   * the memory pointed to by the return value.
   */
  const XrPosef* GetViewPose(uint32_t eye)
  {
    if (eye >= this->GetViewCount())
    {
      return nullptr;
    }
    return &(this->RenderResources->Views[eye].pose);
  }
  ///@}

  ///@{
  /**
   * Returns a pointer to the projection field of view for the specified eye,
   * or nullptr if eye exceeds or equals the number of configured views.  This
   * class retains responsibility for the memory pointed to by the return value.
   */
  const XrFovf* GetProjectionFov(uint32_t eye)
  {
    if (eye >= this->GetViewCount())
    {
      return nullptr;
    }
    return &(this->RenderResources->Views[eye].fov);
  }
  ///@}

  ///@{
  /**
   * Return true if the runtime supports the depth extension.
   *
   * This value is defined by `vtkOpenXRManager::Initialize`.
   */
  bool IsDepthExtensionSupported() { return this->OptionalExtensions.DepthExtensionSupported; }
  ///@}

  /**
   * Return true if the runtime supports the scene understanding extension.
   *
   * This value is defined by `vtkOpenXRManager::Initialize`.
   */
  bool IsSceneUnderstandingSupported()
  {
    return this->OptionalExtensions.SceneUnderstandingSupported;
  }

  ///@{
  /**
   * Return true if the current frame should be rendered.
   * This value is updated each time we call WaitAndBeginFrame and
   * EndFrame.
   */
  bool GetShouldRenderCurrentFrame() { return this->ShouldRenderCurrentFrame; }
  ///@}

  ///@{
  /**
   * Start the OpenXR session.
   * If successful, SessionRunning becomes true.
   */
  bool BeginSession();
  ///@}

  ///@{
  /**
   * Return the OpenXR Session.
   */
  const XrSession& GetSession() { return this->Session; }
  ///@}

  ///@{
  /**
   * Return the instance used to communicate with the runtime
   */
  const XrInstance& GetXrRuntimeInstance() { return this->Instance; }
  ///@}

  ///@{
  /**
   * Return true if the OpenXR session is currently running, ie.
   * the call to BeginSession was successful.
   */
  bool IsSessionRunning() { return this->SessionRunning; }
  ///@}

  ///@{
  /**
   * This function is used to start a frame. If the frame should be rendered,
   * then we locate the views to update the view pose and projection for each
   * eye / display
   */
  bool WaitAndBeginFrame();
  ///@}

  ///@{
  /**
   * Prepare the rendering resources for the specified eye and store in \p colorTextureId and
   * in \p depthTextureId (if the depth extension is supported) the texture in which we need
   * to draw pixels. \p win is used to get the right clipping planes for the depth extension.
   * Return true if no error occurred.
   */
  bool PrepareRendering(vtkOpenXRRenderWindow* win, void* colorTextureId, void* depthTextureId);
  ///@}

  ///@{
  /**
   * When the rendering in a swapchain image is done, it must be released with
   * this function.
   */
  void ReleaseSwapchainImage(uint32_t eye);
  ///@}

  ///@{
  /**
   * Submit the composition layers for the predicted display time of the current frame.
   * It must be called at the end of each frame.
   */
  bool EndFrame();
  ///@}

  ///@{
  /**
   * Store in eventData the result of xrPollEvent.
   */
  bool PollEvent(XrEventDataBuffer& eventData);
  ///@}

  ///@{
  /**
   * Get the XrPath from the well-formed string \p path.
   */
  XrPath GetXrPath(const std::string& path);
  ///@}

  const std::array<XrPath, 2>& GetSubactionPaths() { return this->SubactionPaths; }

  ///@{
  /**
   * Creates an action set and add it to the vector of action sets.
   */
  bool CreateActionSet(const std::string& actionSetName, const std::string& localizedActionSetName);
  ///@}

  ///@{
  /**
   * Selects the current active action set from the
   * ActionSets vector using its index.
   */
  bool SelectActiveActionSet(unsigned int index);
  ///@}

  ///@{
  /**
   * Attach all action sets in the ActionSets vector to the session.
   */
  bool AttachSessionActionSets();
  ///@}

  ///@{
  /**
   * Iterate over and destroy all action sets that have been created.
   */
  ///@}
  void DestroyActionSets();

  struct Action_t;

  ///@{
  /**
   * Creates one action with name \p name and localizedName \p localizedName
   * and store the action handle inside \p actionT using the selected
   * active action set.
   */
  bool CreateOneAction(
    Action_t& actionT, const std::string& name, const std::string& localizedName);
  ///@}

  ///@{
  /**
   * Suggest actions stored in \p actionSuggestedBindings for the interaction profile \p profile
   */
  bool SuggestActions(
    const std::string& profile, std::vector<XrActionSuggestedBinding>& actionSuggestedBindings);
  ///@}

  ///@{
  /**
   * Update the action states using the active action set. This function should be called
   * before UpdateActionData
   */
  bool SyncActions();
  ///@}

  ///@{
  /**
   * Update the action data and store it in action_t.States for one hand.
   * For pose actions :
   *  - we locate its space in the ReferenceSpace space
   *    using the PredictedDisplayTime
   *  - we can store its pose velocity if StorePoseVelocities is true.
   */
  bool UpdateActionData(Action_t& action_t, int hand);
  ///@}

  /**
   * Apply haptic vibration
   * \p action to emit vibration on \p hand to emit on \p amplitude 0.0 to 1.0.
   * \p duration nanoseconds, default 25ms \p frequency (hz)
   */
  bool ApplyVibration(const Action_t& actionT, int hand, float amplitude = 0.5,
    float duration = 25000000.0, float frequency = XR_FREQUENCY_UNSPECIFIED);

  enum ControllerIndex
  {
    Inactive = -1,
    Left = 0,
    Right = 1,
    Head = 2,
    Generic = 3,
    NumberOfControllers = 4
  };

  struct Action_t
  {
    XrAction Action;
    XrActionType ActionType;

    union
    {
      XrActionStateFloat _float;
      XrActionStateBoolean _boolean;
      XrActionStatePose _pose;
      XrActionStateVector2f _vec2f;
    } States[ControllerIndex::NumberOfControllers];

    XrSpace PoseSpaces[ControllerIndex::NumberOfControllers];
    XrSpaceLocation PoseLocations[ControllerIndex::NumberOfControllers];
    XrSpaceVelocity PoseVelocities[ControllerIndex::NumberOfControllers];
  };

  ///@{
  /**
   * Set/Get the rendering backend strategy.
   */
  void SetGraphicsStrategy(vtkOpenXRManagerGraphics* gs) { this->GraphicsStrategy = gs; }
  vtkOpenXRManagerGraphics* GetGraphicsStrategy() { return this->GraphicsStrategy; }
  ///@}

  ///@{
  /**
   * Set/Get the connection strategy.
   */
  void SetConnectionStrategy(vtkOpenXRManagerConnection* cs) { this->ConnectionStrategy = cs; }
  vtkOpenXRManagerConnection* GetConnectionStrategy() { return this->ConnectionStrategy; }
  ///@}

  VTK_DEPRECATED_IN_9_5_0(
    "Use vtkOpenXRRenderWindow::SetUseDepthExtension instead. This has no effect!")
  void SetUseDepthExtension(bool) {}
  VTK_DEPRECATED_IN_9_5_0(
    "Use vtkOpenXRRenderWindow::GetUseDepthExtension instead. This returns false!")
  bool GetUseDepthExtension() const { return false; }

  /**
   * Return OpenXR System ID associated with the XrSession
   */
  XrSystemId GetSystemID() const { return this->SystemId; }

  /**
   * Return XrSpace associated with the XrSession
   */
  XrSpace GetReferenceSpace() const { return this->ReferenceSpace; }

  /**
   * Return runtime predicted display time for next frame.
   * This may be needed for some OpenXR API calls that requires time information.
   * This is updated by `WaitAndBeginFrame()`
   */
  XrTime GetPredictedDisplayTime() const { return this->PredictedDisplayTime; }

protected:
  vtkOpenXRManager();
  ~vtkOpenXRManager() = default;

  ///@{
  /**
   * OpenXR Instance creation.
   * This is where we select the extensions using
   * SelectExtensions
   */
  bool CreateInstance(vtkOpenXRRenderWindow* window);
  std::vector<const char*> SelectExtensions(vtkOpenXRRenderWindow* window);
  ///@}

  ///@{
  /**
   * Print the optional extensions which were found and enabled.
   */
  void PrintOptionalExtensions();
  ///@}

  ///@{
  /**
   * OpenXR System creation
   */
  bool CreateSystem();
  ///@}

  ///@{
  /**
   * Enable system properties such as hand tracking,
   * and choose environment blend modes.
   */
  bool CreateSystemProperties();
  ///@}

  ///@{
  /**
   * Create the session and pass the GraphicsBinding to the next pointer
   * of the XrSessionCreateInfo
   * \pre CreateGraphicsBinding must be called
   */
  bool CreateSession();
  ///@}

  ///@{
  /**
   * Swapchaines creation : there is one swapchain per view / display.
   * This function calls CreateConfigViews
   */
  bool CreateSwapchains();
  ///@}

  ///@{
  /**
   * There is one configuration view per view, and it contains the recommended
   * texture resolution in pixels and the recommended swapchain samples
   */
  bool CreateConfigViews();
  ///@}

  ///@{
  /**
   * During the creation of the swapchains, we need to check the runtime available
   * pixels formats, and we pick the first one from the list of our supported color
   * and depth formats returned by vtkOpenXRManagerGraphics::GetSupportedColorFormats
   * and vtkOpenXRManagerGraphics::GetSupportedDepthFormats
   */
  std::tuple<int64_t, int64_t> SelectSwapchainPixelFormats();
  ///@}

  struct Swapchain_t;

  ///@{
  /**
   * Create an XrSwapchain handle used to present rendered image
   * to the user with the given parameters for the XrSwapchainCreateInfo structure
   */
  Swapchain_t CreateSwapchain(int64_t format, uint32_t width, uint32_t height, uint32_t sampleCount,
    XrSwapchainCreateFlags createFlags, XrSwapchainUsageFlags usageFlags);
  ///@}

  ///@{
  /**
   *  Creates the reference space of type ReferenceSpaceType that will be used to locate views
   */
  bool CreateReferenceSpace();
  ///@}

  bool LoadControllerModels();

  ///@{
  /**
   * For pose actions, we must create an action space to locate it
   */
  bool CreateOneActionSpace(const XrAction& action, const XrPath& subactionPath,
    const XrPosef& poseInActionSpace, XrSpace& space);
  ///@}

  ///@{
  /**
   * Creates one subaction path for each hand.
   */
  bool CreateSubactionPaths();
  ///@}

  ///@{
  /**
   * When preparing the rendering for an eye, we must ask the runtime
   * for a texture to draw in it.
   */
  uint32_t WaitAndAcquireSwapchainImage(const XrSwapchain& swapchainHandle);
  ///@}

  // Currently VTK only supports HeadMountedDisplay (HMD)
  constexpr static XrFormFactor FormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  // Pick the view type to be stereo rather than mono or anything else
  constexpr static XrViewConfigurationType ViewType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  // PRIMARY_STEREO view configuration always has 2 views
  constexpr static uint32_t StereoViewCount = 2;

  // Three available types: VIEW, LOCAL and STAGE.  We use LOCAL space which
  // establishes a world-locked origin, rather than VIEW space, which tracks the
  // view origin.
  XrReferenceSpaceType ReferenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;

  // Communication with the runtime happens through this instance
  XrInstance Instance;

  // A system is defined by an id and is used to create a session
  XrSystemId SystemId;

  XrSession Session;
  XrSessionState SessionState;
  XrSpace ReferenceSpace;

  // At the end of a frame, we must select en environment blend mode
  // to tell the runtime how we want to blend the image with the user's
  // view of the physical world. For example, in VR, we will generally
  // choose XR_ENVIRONMENT_BLEND_MODE_OPAQUE while AR will generally
  // choose XR_ENVIRONMENT_BLEND_MODE_ADDITIVE or XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND
  XrEnvironmentBlendMode EnvironmentBlendMode;

  // Non optional extension
  bool RenderingBackendExtensionSupported = false;

  ///@{
  /**
   * Structure to hold optional extensions
   * loaded with SelectExtensions
   */
  struct
  {
    bool DepthExtensionSupported{ false };
    bool ControllerModelExtensionSupported{ false };
    bool UnboundedRefSpaceSupported{ false };
    bool SpatialAnchorSupported{ false };
    bool HandInteractionSupported{ false };
    bool HandTrackingSupported{ false };
    bool RemotingSupported{ false };
    bool SceneUnderstandingSupported{ false };
    bool SceneMarkerSupported{ false };
  } OptionalExtensions;
  ///@}

  /**
   * Swapchain structure storing information common to all rendering backend.
   * Backend specific images are stored in vtkOpenXRManagerGraphics implementations.
   */
  struct Swapchain_t
  {
    XrSwapchain Swapchain;
    int64_t Format{ 0 };
    uint32_t Width{ 0 };
    uint32_t Height{ 0 };
  };

  ///@{
  /**
   * This struct stores all needed information to render the images
   * and send it to the user
   * We can't make a vector of struct because OpenXR SDK needs
   * an array of XrXXX for xrEnumerate functions
   */
  struct RenderResources_t
  {
    XrViewState ViewState{ XR_TYPE_VIEW_STATE };
    // Each physical Display/Eye is described by a view
    std::vector<XrView> Views;
    // One configuration view per view : this store
    std::vector<XrViewConfigurationView> ConfigViews;

    std::vector<Swapchain_t> ColorSwapchains;
    std::vector<Swapchain_t> DepthSwapchains;

    std::vector<XrCompositionLayerProjectionView> ProjectionLayerViews;
    std::vector<XrCompositionLayerDepthInfoKHR> DepthInfoViews;
  };
  std::unique_ptr<RenderResources_t> RenderResources;
  ///@}

  // There is one subaction path for each hand.
  std::array<XrPath, 2> SubactionPaths;

  std::vector<XrActionSet> ActionSets;
  XrActionSet* ActiveActionSet = nullptr;

  /**
   * Store the frame predicted display time in WaitAndBeginFrame
   * To get the action data at this time and to submit it in EndFrame
   */
  XrTime PredictedDisplayTime;

  bool SessionRunning = false;
  // Following each WaitAndBeginFrame operation, the OpenXR runtime may indicate
  // whether the current frame should be rendered using the `XrFrameState.shouldRender`
  // property. We store this information to optimize rendering and prevent unnecessary
  // render calls. For further details, refer to:
  // https://registry.khronos.org/OpenXR/specs/1.0/man/html/XrFrameState.html
  bool ShouldRenderCurrentFrame = false;
  // If true, the function UpdateActionData will store
  // pose velocities for pose actions
  bool StorePoseVelocities = false;

  vtkSmartPointer<vtkOpenXRManagerGraphics> GraphicsStrategy;

  vtkSmartPointer<vtkOpenXRManagerConnection> ConnectionStrategy;

private:
  vtkOpenXRManager(const vtkOpenXRManager&) = delete;
  void operator=(const vtkOpenXRManager&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkOpenXRManager.h
