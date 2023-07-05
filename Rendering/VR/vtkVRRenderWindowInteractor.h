// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRRenderWindowInteractor
 * @brief   Implements VR specific functions required by vtkRenderWindowInteractor.
 */

#ifndef vtkVRRenderWindowInteractor_h
#define vtkVRRenderWindowInteractor_h

#include "vtkEventData.h" // for ivar
#include "vtkNew.h"       // for ivar
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderingVRModule.h" // for export macro

#include <string> // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix4x4;
class vtkVRRenderWindow;

class VTKRENDERINGVR_EXPORT vtkVRRenderWindowInteractor : public vtkRenderWindowInteractor3D
{
public:
  vtkTypeMacro(vtkVRRenderWindowInteractor, vtkRenderWindowInteractor3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the event handler.
   */
  void Initialize() override;

  /**
   * Run the event loop and return. This is provided so that you can
   * implement your own event loop but yet use the VTK event handling as well.
   */
  void ProcessEvents() override;

  /**
   * Implements the event loop.
   */
  virtual void DoOneEvent(vtkVRRenderWindow* renWin, vtkRenderer* ren) = 0;

  ///@{
  /**
   * Methods to set the default exit method for the class. These methods are
   * only used if no instance level ExitMethod has been defined. They are provided
   * as a means to control how an interactor is exited given the various
   * language bindings (Win32, etc.).
   */
  static void SetClassExitMethod(void (*f)(void*), void* arg);
  static void SetClassExitMethodArgDelete(void (*f)(void*));
  ///@}

  /**
   * This method corresponds to the Exit callback, allowing for the style to invoke it.
   */
  void ExitCallback() override;

  ///@{
  /**
   * Set/get the direction of the physical coordinate system -Z axis in world coordinates.
   */
  void SetPhysicalViewDirection(double, double, double) override;
  double* GetPhysicalViewDirection() override;
  ///@}

  ///@{
  /**
   * Set/get the direction of the physical coordinate system +Y axis in world coordinates.
   */
  void SetPhysicalViewUp(double, double, double) override;
  double* GetPhysicalViewUp() override;
  ///@}

  ///@{
  /**
   * Set/get position of the physical coordinate system origin in world coordinates.
   */
  void SetPhysicalTranslation(vtkCamera*, double, double, double) override;
  double* GetPhysicalTranslation(vtkCamera*) override;
  ///@}

  ///@{
  /**
   * Set/get the physical scale (world / physical distance ratio)
   */
  void SetPhysicalScale(double) override;
  double GetPhysicalScale() override;
  ///@}

  /*
   * Return the pointer index as a device.
   */
  vtkEventDataDevice GetPointerDevice();

  /*
   * Convert a device pose to a world coordinate position and orientation.
   * \param pos  Output world position
   * \param wxyz Output world orientation quaternion
   * \param ppos Output physical position
   * \param wdir Output world view direction (-Z)
   */
  void ConvertPoseToWorldCoordinates(vtkMatrix4x4* poseInTrackingCoordinates, double pos[3],
    double wxyz[4], double ppos[3], double wdir[3]);

  /*
   * Return starting physical to world matrix.
   */
  void GetStartingPhysicalToWorldMatrix(vtkMatrix4x4* startingPhysicalToWorldMatrix);

  ///@{
  /**
   * Set/Get the .json filename describing action bindings for events.
   * Based on https://github.com/ValveSoftware/openvr/wiki/Action-manifest
   * Default is empty.
   */
  vtkGetMacro(ActionManifestFileName, std::string);
  vtkSetMacro(ActionManifestFileName, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the path to the directory to search for the ActionManifestFileName
   * Default is empty.
   */
  vtkGetMacro(ActionManifestDirectory, std::string);
  vtkSetMacro(ActionManifestDirectory, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the name of the action set to use from the action manifest
   */
  vtkGetMacro(ActionSetName, std::string);
  vtkSetMacro(ActionSetName, std::string);
  ///@}

protected:
  vtkVRRenderWindowInteractor();
  ~vtkVRRenderWindowInteractor() override;

  ///@{
  /**
   * internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  ///@}

  /**
   * This will start up the event loop and never return. If you call this
   * method it will loop processing events until the application is exited.
   */
  void StartEventLoop() override;

  ///@{
  /**
   * Handle complex gesture events. Complex gesture events recognition starts when
   * both buttons mapped to the ComplexGesture action are pressed.
   *
   * To differentiate the Rotate, Pinch and Pan gestures, the default implementation
   * is based on the following heuristic:
   * - Pinch is a move to/from the center point.
   * - Rotate is a move along the circumference.
   * - Pan is a move of the center point.
   *
   * After computing the distance along each of these axes in meters, the first
   * to break the hard-coded threshold wins.
   *
   * Overriding both HandleComplexGestureEvents() and RecognizeComplexGesture() allows to define
   * a different heuristic.
   */
  virtual void HandleComplexGestureEvents(vtkEventData* ed);
  virtual void RecognizeComplexGesture(vtkEventDataDevice3D* edata);
  ///@}

  ///@{
  /**
   * Class variables so an exit method can be defined for this class (used to set
   * different exit methods for various language bindings, i.e. Java, Win32).
   */
  static void (*ClassExitMethod)(void*);
  static void (*ClassExitMethodArgDelete)(void*);
  static void* ClassExitMethodArg;
  ///@}

  /**
   * Store physical to world matrix at the start of a complex gesture.
   */
  vtkNew<vtkMatrix4x4> StartingPhysicalToWorldMatrix;

  int DeviceInputDownCount[vtkEventDataNumberOfDevices];

  std::string ActionManifestFileName;
  std::string ActionManifestDirectory;

  std::string ActionSetName;

private:
  vtkVRRenderWindowInteractor(const vtkVRRenderWindowInteractor&) = delete;
  void operator=(const vtkVRRenderWindowInteractor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
