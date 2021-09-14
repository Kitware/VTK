/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenVRRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRRenderWindowInteractor
 * @brief   implements OpenVR specific functions
 * required by vtkRenderWindowInteractor.
 *
 *
 */

#ifndef vtkOpenVRRenderWindowInteractor_h
#define vtkOpenVRRenderWindowInteractor_h

#include "vtkEventData.h" // for ivar
#include "vtkNew.h"       // ivars
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderingOpenVRModule.h" // For export macro
#include <functional>                 // for ivar
#include <map>                        // for ivar
#include <openvr.h>                   // for ivar
#include <string>                     // for ivar
#include <tuple>                      // for ivar

class vtkTransform;
class vtkMatrix4x4;
class vtkOpenVRRenderWindow;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderWindowInteractor : public vtkRenderWindowInteractor3D
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkOpenVRRenderWindowInteractor* New();

  vtkTypeMacro(vtkOpenVRRenderWindowInteractor, vtkRenderWindowInteractor3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Initialize the event handler
   */
  virtual void Initialize();

  ///@{
  /**
   * Methods to set the default exit method for the class. This method is
   * only used if no instance level ExitMethod has been defined.  It is
   * provided as a means to control how an interactor is exited given
   * the various language bindings (Win32, etc.).
   */
  static void SetClassExitMethod(void (*f)(void*), void* arg);
  static void SetClassExitMethodArgDelete(void (*f)(void*));
  ///@}

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();

  ///@{
  /**
   * Set/Get the optional translation to map world coordinates into the
   * 3D physical space (meters, 0,0,0).
   */
  virtual void SetPhysicalTranslation(vtkCamera*, double, double, double);
  virtual double* GetPhysicalTranslation(vtkCamera*);
  virtual void SetPhysicalScale(double);
  virtual double GetPhysicalScale();
  ///@}

  /**
   * Run the event loop and return. This is provided so that you can
   * implement your own event loop but yet use the vtk event handling as
   * well.
   */
  void ProcessEvents() override;

  virtual void DoOneEvent(vtkOpenVRRenderWindow* renWin, vtkRenderer* ren);

  /*
   * Return the pointer index as a device
   */
  vtkEventDataDevice GetPointerDevice();

  /*
   * Convert a device pose to pose matrices
   * \param poseMatrixPhysical Optional output pose matrix in physical frame
   * \param poseMatrixWorld    Optional output pose matrix in world frame
   */
  void ConvertOpenVRPoseToMatrices(const vr::TrackedDevicePose_t& tdPose,
    vtkMatrix4x4* poseMatrixWorld, vtkMatrix4x4* poseMatrixPhysical = nullptr);

  /*
   * Convert a device pose to a world coordinate position and orientation
   * \param pos  Output world position
   * \param wxyz Output world orientation quaternion
   * \param ppos Output physical position
   * \param wdir Output world view direction (-Z)
   */
  void ConvertPoseToWorldCoordinates(const vr::TrackedDevicePose_t& tdPose, double pos[3],
    double wxyz[4], double ppos[3], double wdir[3]);
  void ConvertPoseMatrixToWorldCoordinates(
    const float poseMatrix[3][4], double pos[3], double wxyz[4], double ppos[3], double wdir[3]);

  ///@{
  /**
   * Get the latest touchpad or joystick position for a device
   */
  // void GetTouchPadPosition(vtkEventDataDevice, vtkEventDataDeviceInput, float[3]) override;
  ///@}

  /*
   * Return starting physical to world matrix
   */
  void GetStartingPhysicalToWorldMatrix(vtkMatrix4x4* startingPhysicalToWorldMatrix);

  ///@{
  /**
   * Assign an event or std::function to an event path
   */
  void AddAction(std::string path, vtkCommand::EventIds, bool isAnalog);
  void AddAction(std::string path, bool isAnalog, std::function<void(vtkEventData*)>);
  ///@}
  // add an event action

  ///@{
  /**
   * Set/Get the json file describing action bindings for events
   * See https://github.com/ValveSoftware/openvr/wiki/Action-manifest
   */
  vtkGetMacro(ActionManifestFileName, std::string);
  vtkSetMacro(ActionManifestFileName, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the json file describing action set to use
   */
  vtkGetMacro(ActionSetName, std::string);
  vtkSetMacro(ActionSetName, std::string);
  ///@}

protected:
  vtkOpenVRRenderWindowInteractor();
  ~vtkOpenVRRenderWindowInteractor() override;

  ///@{
  /**
   * Class variables so an exit method can be defined for this class
   * (used to set different exit methods for various language bindings,
   * i.e. java, Win32)
   */
  static void (*ClassExitMethod)(void*);
  static void (*ClassExitMethodArgDelete)(void*);
  static void* ClassExitMethodArg;
  ///@}

  ///@{
  /**
   * Win32-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);
  ///@}

  /**
   * This will start up the event loop and never return. If you
   * call this method it will loop processing events until the
   * application is exited.
   */
  virtual void StartEventLoop();

  /**
   * Handle multitouch events. Multitouch events recognition starts when
   * both controllers the trigger pressed.
   */
  int DeviceInputDownCount[vtkEventDataNumberOfDevices];
  virtual void RecognizeComplexGesture(vtkEventDataDevice3D* edata);

  /**
   * Store physical to world matrix at the start of a multi-touch gesture
   */
  vtkNew<vtkMatrix4x4> StartingPhysicalToWorldMatrix;

  class ActionData
  {
  public:
    vr::VRActionHandle_t ActionHandle;
    vtkCommand::EventIds EventId;
    std::function<void(vtkEventData*)> Function;
    bool UseFunction = false;
    bool IsAnalog = false;
  };

  std::map<std::string, ActionData> ActionMap;

  std::string ActionManifestFileName;
  std::string ActionSetName;

  vr::VRActionSetHandle_t ActionsetVTK = vr::k_ulInvalidActionSetHandle;

  enum TrackerEnum
  {
    LeftHand = 0,
    RightHand,
    Head,
    NumberOfTrackers
  };

  struct TrackerActions
  {
    vr::VRInputValueHandle_t Source = vr::k_ulInvalidInputValueHandle;
    // vr::VRActionHandle_t ActionPose = vr::k_ulInvalidActionHandle;
    // vr::InputPoseActionData_t LastPoseData;
    vr::TrackedDevicePose_t LastPose;
  };
  TrackerActions Trackers[NumberOfTrackers];

  void HandleGripEvents(vtkEventData* ed);

private:
  vtkOpenVRRenderWindowInteractor(const vtkOpenVRRenderWindowInteractor&) = delete;
  void operator=(const vtkOpenVRRenderWindowInteractor&) = delete;
};

#endif
