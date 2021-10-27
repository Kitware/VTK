/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRRenderWindow
 * @brief   OpenXR rendering window
 *
 *
 * vtkOpenXRRenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow.
 *
 * This class and its similar classes are designed to be drop in
 * replacements for VTK. If you link to this module and turn on
 * the CMake option VTK_OPENXR_OBJECT_FACTORY, the object
 * factory mechanism should replace the core rendering classes such as
 * RenderWindow with OpenXR specialized versions. The goal is for VTK
 * programs to be able to use the OpenXR library with little to no
 * changes.
 *
 * This class handles the bulk of interfacing to OpenXR. It supports one
 * renderer currently. The renderer is assumed to cover the entire window
 * which is what makes sense to VR. Overlay renderers can probably be
 * made to work with this but consider how overlays will appear in a
 * HMD if they do not track the viewpoint etc.
 *
 * OpenXR provides HMD and controller positions in "Physical" coordinate
 * system.
 * Origin: user's eye position at the time of calibration.
 * Axis directions: x = user's right; y = user's up; z = user's back.
 * Unit: meter.
 *
 * Renderer shows actors in World coordinate system. Transformation between
 * Physical and World coordinate systems is defined by PhysicalToWorldMatrix.
 * This matrix determines the user's position and orientation in the rendered
 * scene and scaling (magnification) of rendered actors.
 *
 */

#ifndef vtkOpenXRRenderWindow_h
#define vtkOpenXRRenderWindow_h

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenXRModule.h" // For export macro

#include "vtkEventData.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenXR.h"
#include "vtkVRRay.h"

#include <array>  // array
#include <vector> // vector

class vtkMatrix4x4;
class vtkVRModel;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRRenderWindow : public vtkOpenGLRenderWindow
{
public:
  enum
  {
    PhysicalToWorldMatrixModified = vtkCommand::UserEvent + 200
  };
  static vtkOpenXRRenderWindow* New();
  vtkTypeMacro(vtkOpenXRRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create an interactor to control renderers in this window.
   */
  vtkRenderWindowInteractor* MakeRenderWindowInteractor() override;

  /**
   * Add a renderer to the list of renderers.
   */
  void AddRenderer(vtkRenderer*) override;

  /**
   * Begin the rendering process.
   */
  void Start() override;

  /**
   * Free up any graphics resources associated with this window
   * a value of nullptr means the context may already be destroyed
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * Update the system, if needed, due to stereo rendering. For some stereo
   * methods, subclasses might need to switch some hardware settings here.
   */
  void StereoUpdate() override;

  /**
   * Intermediate method performs operations required between the rendering
   * of the left and right eye.
   */
  void StereoMidpoint() override;

  /**
   * Handles work required once both views have been rendered when using
   * stereo rendering.
   */
  void StereoRenderComplete() override;

  /**
   * Overridden to not release resources that would interfere with an external
   * application's rendering. Avoiding round trip.
   */
  void Render() override;
  //@}

  //@{
  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  void Initialize() override;

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize() override;

  /**
   * Make this windows OpenGL context the current context.
   */
  void MakeCurrent() override;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override { return "OpenXR System"; }

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  vtkTypeBool IsDirect() override { return 1; }

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * Maybe should return 1 always?
   */
  vtkTypeBool GetEventPending() override { return 0; }

  // implement required virtual functions
  void* GetGenericDisplayId() override { return (void*)this->HelperWindow->GetGenericDisplayId(); }
  void* GetGenericWindowId() override { return (void*)this->HelperWindow->GetGenericWindowId(); }
  void* GetGenericParentId() override { return (void*)nullptr; }
  void* GetGenericContext() override { return (void*)this->HelperWindow->GetGenericContext(); }
  void* GetGenericDrawable() override { return (void*)this->HelperWindow->GetGenericDrawable(); }

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL() override { return 1; }

  //@{
  /**
   * Set/Get the helper render window to use for the openGL context
   */
  vtkGetObjectMacro(HelperWindow, vtkOpenGLRenderWindow);
  void SetHelperWindow(vtkOpenGLRenderWindow* val);
  //@}

  // Get the state object used to keep track of
  // OpenGL state
  vtkOpenGLState* GetState() override;

  //@{
  /**
   * Get the frame buffers used for rendering
   */
  GLuint GetLeftResolveBufferId() { return this->FramebufferDescs[0].ResolveFramebufferId; }
  GLuint GetRightResolveBufferId() { return this->FramebufferDescs[1].ResolveFramebufferId; }
  void GetRenderBufferSize(int& width, int& height)
  {
    width = this->Size[0];
    height = this->Size[1];
  }
  //@}

  //@{
  /**
   * Set/get physical coordinate system in world coordinate system.
   *
   * View direction is the -Z axis of the physical coordinate system
   * in world coordinate system.
   * \sa SetPhysicalViewUp, \sa SetPhysicalTranslation,
   * \sa SetPhysicalScale, \sa SetPhysicalToWorldMatrix
   */
  virtual void SetPhysicalViewDirection(double, double, double);
  virtual void SetPhysicalViewDirection(double[3]);
  vtkGetVector3Macro(PhysicalViewDirection, double);
  //@}

  //@{
  /**
   * Set/get physical coordinate system in world coordinate system.
   *
   * View up is the +Y axis of the physical coordinate system
   * in world coordinate system.
   * \sa SetPhysicalViewDirection, \sa SetPhysicalTranslation,
   * \sa SetPhysicalScale, \sa SetPhysicalToWorldMatrix
   */
  virtual void SetPhysicalViewUp(double, double, double);
  virtual void SetPhysicalViewUp(double[3]);
  vtkGetVector3Macro(PhysicalViewUp, double);
  //@}

  //@{
  /**
   * Set/get physical coordinate system in world coordinate system.
   *
   * Position of the physical coordinate system origin
   * in world coordinates.
   * \sa SetPhysicalViewDirection, \sa SetPhysicalViewUp,
   * \sa SetPhysicalScale, \sa SetPhysicalToWorldMatrix
   */
  virtual void SetPhysicalTranslation(double, double, double);
  virtual void SetPhysicalTranslation(double[3]);
  vtkGetVector3Macro(PhysicalTranslation, double);
  //@}

  //@{
  /**
   * Set/get physical coordinate system in world coordinate system.
   *
   * Ratio of distance in world coordinate and physical and system
   * (PhysicalScale = distance_World / distance_Physical).
   * Example: if world coordinate system is in mm then
   * PhysicalScale = 1000.0 makes objects appear in real size.
   * PhysicalScale = 100.0 makes objects appear 10x larger than real size.
   */
  virtual void SetPhysicalScale(double);
  vtkGetMacro(PhysicalScale, double);
  //@}

  //@{
  /**
   * Set physical to world transform matrix. Members calculated and set from the matrix:
   * \sa PhysicalViewDirection, \sa PhysicalViewUp, \sa PhysicalTranslation, \sa PhysicalScale
   * The x axis scale is used for \sa PhysicalScale
   */
  void SetPhysicalToWorldMatrix(vtkMatrix4x4* matrix);
  //@}

  //@{
  /**
   * Get physical to world transform matrix. Members used to calculate the matrix:
   * \sa PhysicalViewDirection, \sa PhysicalViewUp, \sa PhysicalTranslation, \sa PhysicalScale
   */
  void GetPhysicalToWorldMatrix(vtkMatrix4x4* matrix);
  //@}

  //@{
  /*
   * Convert a device pose to pose matrices
   * \param poseMatrixPhysical Optional output pose matrix in physical space
   * \param poseMatrixWorld    Optional output pose matrix in world space
   */
  void ConvertOpenXRPoseToMatrices(const XrPosef& xrPose, vtkMatrix4x4* poseMatrixWorld,
    vtkMatrix4x4* poseMatrixPhysical = nullptr);
  //@}

  //@{
  /*
   * Convert a device pose to a world coordinate position and orientation
   * \param pos  Output world position
   * \param wxyz Output world orientation quaternion
   * \param ppos Output physical position
   * \param wdir Output world view direction (-Z)
   */
  void ConvertOpenXRPoseToWorldCoordinates(
    const XrPosef& xrPose, double pos[3], double wxyz[4], double ppos[3], double wdir[3]);
  //@}

  //@{
  /*
   * Get the index corresponding to this EventDataDevice
   */
  const int GetTrackedDeviceIndexForDevice(vtkEventDataDevice);
  //@}

  //@{
  /*
   * Get the OpenXRModel corresponding to the device index.
   */
  vtkVRModel* GetTrackedDeviceModel(const int idx);
  //@}

  //@{
  /**
   * True if the window has been initialized successfully.
   */
  vtkGetMacro(Initialized, bool);
  //@}

  //@{
  /**
   * Set the active state (active: true / inactive: false) of the specified hand.
   */
  void SetModelActiveState(const int hand, bool state) { this->ModelsActiveState[hand] = state; }
  //@}

protected:
  vtkOpenXRRenderWindow();
  ~vtkOpenXRRenderWindow() override;

  void CreateAWindow() override {}
  void DestroyWindow() override {}

  // Create one framebuffer per view
  void CreateFramebuffers(uint32_t viewCount);

  struct FramebufferDesc;

  bool BindTextureToFramebuffer(FramebufferDesc& framebufferDesc);
  void RenderFramebuffer(FramebufferDesc& framebufferDesc);

  void RenderOneEye(const uint32_t eye);

  void RenderModels();

  vtkOpenGLRenderWindow* HelperWindow;

  bool Initialized = false;

  struct FramebufferDesc
  {
    GLuint ResolveFramebufferId;
    GLuint ResolveColorTextureId;
    GLuint ResolveDepthTextureId;
  };

  // TOBE generic
  // One per view (typically one per eye)
  std::vector<FramebufferDesc> FramebufferDescs;

  // TO BE generic
  /// -Z axis of the Physical to World matrix
  double PhysicalViewDirection[3];
  /// Y axis of the Physical to World matrix
  double PhysicalViewUp[3];
  /// Inverse of the translation component of the Physical to World matrix, in mm
  double PhysicalTranslation[3];
  /// Scale of the Physical to World matrix
  double PhysicalScale;

  // Controller models
  std::array<vtkSmartPointer<vtkVRModel>, 2> Models;

  // Store if a model is active or not here as openxr do not have a concept
  // of active/inactive controller
  std::array<bool, 2> ModelsActiveState = { true, true };

private:
  vtkOpenXRRenderWindow(const vtkOpenXRRenderWindow&) = delete;
  void operator=(const vtkOpenXRRenderWindow&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkOpenXRRenderWindow.h
