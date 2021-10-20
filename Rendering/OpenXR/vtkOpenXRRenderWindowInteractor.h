/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRRenderWindowInteractor
 * @brief   implements OpenXR specific functions
 * required by vtkRenderWindowInteractor.
 *
 */

#ifndef vtkOpenXRRenderWindowInteractor_h
#define vtkOpenXRRenderWindowInteractor_h

#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderingOpenXRModule.h" // For export macro

#include "vtkEventData.h"
#include "vtkOpenXRManager.h"

#include <functional> // for std::function
#include <map>        // for std::map

class vtkOpenXRBooleanActionData;
class vtkEventData;
class vtkOpenXRRenderWindow;
typedef vtkOpenXRManager::Action_t Action_t;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRRenderWindowInteractor : public vtkRenderWindowInteractor3D
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkOpenXRRenderWindowInteractor* New();

  vtkTypeMacro(vtkOpenXRRenderWindowInteractor, vtkRenderWindowInteractor3D);

  /**
   * Initialize the event handler
   */
  virtual void Initialize();

  //@{
  /**
   * Methods to set the default exit method for the class. This method is
   * only used if no instance level ExitMethod has been defined.  It is
   * provided as a means to control how an interactor is exited given
   * the various language bindings (Win32, etc.).
   */
  static void SetClassExitMethod(void (*f)(void*), void* arg);
  static void SetClassExitMethodArgDelete(void (*f)(void*));
  //@}

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();

  /**
   * Run the event loop and return. This is provided so that you can
   * implement your own event loop but yet use the vtk event handling as
   * well.
   */
  void ProcessEvents() override;

  virtual void DoOneEvent(vtkOpenXRRenderWindow* renWin, vtkRenderer* ren);

  /**
   * Process OpenXR specific events.
   */
  void ProcessXrEvents();

  /**
   * Update tha action states using the OpenXRManager
   * and handle all actions.
   */
  void PollXrActions(vtkOpenXRRenderWindow* renWin);

  /*
   * Return the pointer index as a device
   */
  vtkEventDataDevice GetPointerDevice();

  /**
   * Return the XrPosef for the action named "handpose"
   * and the hand \p hand
   */
  XrPosef& GetHandPose(const uint32_t hand);

  /*
   * Return starting physical to world matrix
   */
  void GetStartingPhysicalToWorldMatrix(vtkMatrix4x4* startingPhysicalToWorldMatrix);

  //@{
  /**
   * Assign an event or std::function to an event path.
   * Called by the interactor style for specific actions
   */
  void AddAction(const std::string& path, const vtkCommand::EventIds&);
  void AddAction(const std::string& path, const std::function<void(vtkEventData*)>&);
  //@}
  // add an event action

  //@{
  /**
   * Set/Get the json file describing action bindings for events
   * At the opposite of OpenVR, this file is not standard
   * So we chosed to use a json file format that looks like
   * the openVR format.
   * But it could change
   */
  vtkGetMacro(ActionManifestFileName, std::string);
  vtkSetMacro(ActionManifestFileName, std::string);
  //@}

  //@{
  /**
   * Set/Get the json file describing action set to use
   */
  vtkGetMacro(ActionSetName, std::string);
  vtkSetMacro(ActionSetName, std::string);
  //@}

protected:
  vtkOpenXRRenderWindowInteractor();
  ~vtkOpenXRRenderWindowInteractor() override;
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Class variables so an exit method can be defined for this class
   * (used to set different exit methods for various language bindings,
   * i.e. java, Win32)
   */
  static void (*ClassExitMethod)(void*);
  static void (*ClassExitMethodArgDelete)(void*);
  static void* ClassExitMethodArg;
  //@}

  //@{
  /**
   * Win32-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);
  //@}

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

  struct ActionData;

  XrActionType GetActionTypeFromString(const std::string& type);
  bool LoadActions(const std::string& actionFilename);
  bool LoadDefaultBinding(const std::string& bindingFilename);
  ActionData* GetActionDataFromName(const std::string& actionName);

  void HandleGripEvents(vtkEventData* ed);

  void HandleAction(const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed);
  void HandleBooleanAction(const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed);
  void HandlePoseAction(const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed);
  void HandleVector2fAction(const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed);
  void ApplyAction(const ActionData& actionData, vtkEventDataDevice3D* ed);

  /**
   * Store physical to world matrix at the start of a multi-touch gesture
   */
  vtkNew<vtkMatrix4x4> StartingPhysicalToWorldMatrix;

  std::string ActionManifestFileName;
  std::string ActionSetName;

  struct ActionData
  {
    std::string Name;

    vtkEventDataDeviceInput DeviceInput = vtkEventDataDeviceInput::Unknown;

    // This structure is defined in vtkOpenXRManager
    // And hold OpenXR related data
    Action_t ActionStruct{ XR_NULL_HANDLE };

    vtkCommand::EventIds EventId;
    std::function<void(vtkEventData*)> Function;
    bool UseFunction = false;
  };

  using MapAction = std::map<std::string, ActionData*>;
  MapAction MapActionStruct_Name;

private:
  vtkOpenXRRenderWindowInteractor(const vtkOpenXRRenderWindowInteractor&) = delete;
  void operator=(const vtkOpenXRRenderWindowInteractor&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkOpenXRRenderWindowInteractor.h
