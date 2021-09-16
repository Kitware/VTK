/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVRRenderWindowInteractor
 * @brief   Implements VR specific functions required by vtkRenderWindowInteractor.
 */

#ifndef vtkVRRenderWindowInteractor_h
#define vtkVRRenderWindowInteractor_h

#include "vtkEventData.h" // for ivar
#include "vtkMatrix4x4.h" // for ivar
#include "vtkNew.h"       // ivars
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderingVRModule.h" // For export macro

#include <functional> // for ivar
#include <map>        // for ivar
#include <string>     // for ivar
#include <tuple>      // for ivar

class vtkTransform;
class vtkMatrix4x4;
class vtkVRRenderWindow;

class VTKRENDERINGVR_EXPORT vtkVRRenderWindowInteractor : public vtkRenderWindowInteractor3D
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  vtkTypeMacro(vtkVRRenderWindowInteractor, vtkRenderWindowInteractor3D);
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

  virtual void DoOneEvent(vtkVRRenderWindow* renWin, vtkRenderer* ren) = 0;

  /*
   * Return the pointer index as a device
   */
  vtkEventDataDevice GetPointerDevice();

  /*
   * Convert a device pose to a world coordinate position and orientation
   * \param pos  Output world position
   * \param wxyz Output world orientation quaternion
   * \param ppos Output physical position
   * \param wdir Output world view direction (-Z)
   */
  // void ConvertPoseToWorldCoordinates(const float poseInTrackingCoordinates[3][4], double pos[3],
  void ConvertPoseToWorldCoordinates(vtkMatrix4x4* poseInTrackingCoordinates, double pos[3],
    double wxyz[4], double ppos[3], double wdir[3]);

  /*
   * Return starting physical to world matrix
   */
  void GetStartingPhysicalToWorldMatrix(vtkMatrix4x4* startingPhysicalToWorldMatrix);

  ///@{
  /**
   * Set/Get the json file describing action bindings for events
   * Based on https://github.com/ValveSoftware/openvr/wiki/Action-manifest
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
  vtkVRRenderWindowInteractor();
  ~vtkVRRenderWindowInteractor() = default;

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

  std::string ActionManifestFileName;
  std::string ActionSetName;

  void HandleGripEvents(vtkEventData* ed);

private:
  vtkVRRenderWindowInteractor(const vtkVRRenderWindowInteractor&) = delete;
  void operator=(const vtkVRRenderWindowInteractor&) = delete;
};

#endif
