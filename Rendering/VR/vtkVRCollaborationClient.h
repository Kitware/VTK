/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This class provides collaboration support for VR using avatars
// It relies on ZeroMQ to communicate with a collaboration server
// to exchance avatar names and poses and potentially other messages.

#ifndef vtkVRCollaborationClient_h
#define vtkVRCollaborationClient_h

#include "vtkEventData.h" // for ivars
#include "vtkLogger.h"    // for Verbosity enum
#include "vtkObject.h"
#include "vtkRenderingVRModule.h"          // For export macro
#include "vtkSmartPointer.h"               // method sig
#include "vtksys/CommandLineArguments.hxx" // for method sig
#include <array>                           // for ivar
#include <functional>                      // for ivars
#include <map>                             // for ivars
#include <memory>                          // for ivars, shared_ptr
#include <string>                          // for ivars
#include <vector>                          // for ivars

class vtkCallbackCommand;
class vtkOpenGLAvatar;
class vtkOpenGLRenderer;
class vtkOpenGLRenderWindow;
class vtkTransform;
class vtkVRCollaborationClientInternal;

class VTKRENDERINGVR_EXPORT vtkVRCollaborationClient : public vtkObject
{
public:
  static vtkVRCollaborationClient* New();
  vtkTypeMacro(vtkVRCollaborationClient, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkVRCollaborationClient(const vtkVRCollaborationClient&) = delete;
  vtkVRCollaborationClient& operator=(const vtkVRCollaborationClient&) = delete;

  // when sending messages we have to marshal arguments so we have a simple
  // class to encapsulate an argument. The method to send a message takes a
  // std::vector of arguments and there is a method to return a std::vector
  // of arguments when receiving a message
  enum ArgumentType
  {
    Double = 0,
    Int32,
    String
  };

  class VTKRENDERINGVR_EXPORT Argument
  {
  public:
    bool GetString(std::string& result);
    void SetString(std::string const& in);
    bool GetStringVector(std::vector<std::string>& result);
    void SetStringVector(std::vector<std::string> const& in);

    bool GetDoubleVector(std::vector<double>& result);
    void SetDoubleVector(double const* in, uint16_t size);
    void SetDouble(double val);
    bool GetDouble(double& result);

    bool GetInt32Vector(std::vector<int32_t>& result);
    void SetInt32Vector(int32_t const* in, uint16_t size);
    void SetInt32(int32_t val);
    bool GetInt32(int32_t& result);

    ArgumentType Type;
    uint16_t Count = 0;
    std::shared_ptr<void> Data;
  };

  ///@{
  /**
   * method signatures to send messages with Arguments and extract them out of messages
   */
  void SendAMessage(std::string const& msgType);
  void SendAMessage(std::string const& msgType, std::vector<Argument> const& args);
  std::vector<Argument> GetMessageArguments();
  void SendPoseMessage(std::string const& msgType, int index, double pos[3], double dir[3]);
  ///@}

  // call during the render loop to handle collaboration messages
  virtual void Render();

  // required call, true on success, pass the renderer you want the avatars added to
  virtual bool Initialize(vtkOpenGLRenderer*);

  // close the connection
  void Disconnect();

  // set the values for the collaboration connection
  // Can be done through Set* methods or by passing in
  // the command line arguments via AddArguments
  virtual void AddArguments(vtksys::CommandLineArguments& arguments);
  void SetCollabHost(std::string const& val) { this->CollabHost = val; }
  void SetCollabSession(std::string const& val) { this->CollabSession = val; }
  void SetCollabName(std::string const& val) { this->CollabName = val; }
  void SetCollabPort(int val) { this->CollabPort = val; }

  // to receive log/warning/error output
  void SetLogCallback(
    std::function<void(std::string const& data, vtkLogger::Verbosity verbosity)> cb)
  {
    this->Callback = cb;
  }

  // to override the default method of getting avatar scales
  void SetScaleCallback(std::function<double()> cb) { this->ScaleCallback = cb; }

  // return the renderer being used by this instance (assigned during Initialize())
  vtkOpenGLRenderer* GetRenderer() { return this->Renderer; }

  // is this instance connected to a collaboration server?
  bool GetConnected() { return this->Connected; }

protected:
  vtkVRCollaborationClient();
  ~vtkVRCollaborationClient() override;

  void Log(vtkLogger::Verbosity verbosity, std::string const& msg);

  // provided values
  std::string CollabID;
  std::string CollabHost;
  std::string CollabSession;
  std::string CollabName;
  int CollabPort;

  std::function<void(std::string const& data, vtkLogger::Verbosity)> Callback;
  std::function<double()> ScaleCallback;

  bool DisplayOwnAvatar;
  bool PublishAvailable;
  void HandleCollabMessage();
  bool AvatarIdle(std::string id);
  void EraseIdleAvatars();
  double NeedHeartbeat;
  double NeedReply;
  int RetryCount;

  bool Connected;

  // get existing avatar, or create new one, if needed, and return it.
  vtkSmartPointer<vtkOpenGLAvatar> GetAvatar(std::string id);

  void SendLatestDevicePoses();
  void UpdateAvatarPoseFromCamera();

  virtual void HandleBroadcastMessage(std::string const& otherID, std::string const& type);

  vtkCallbackCommand* EventCommand;
  static void EventCallback(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);
  long MoveObserver;

  vtkOpenGLRenderer* Renderer;
  vtkOpenGLRenderWindow* RenderWindow;

  vtkNew<vtkTransform> TempTransform;

  // used to throttle outgoing pose messages
  double YourLastAvatarUpdateTime;
  bool HasPoseForDevice[vtkEventDataNumberOfDevices];
  struct Pose
  {
    std::array<double, 3> Position;
    std::array<double, 4> Orientation;
  };
  Pose DevicePoses[vtkEventDataNumberOfDevices];

  // dynamic set of avatars, keyed on IDs sent with updates.
  std::map<std::string, vtkSmartPointer<vtkOpenGLAvatar>> Avatars;
  std::map<std::string, double[vtkEventDataNumberOfDevices]> AvatarUpdateTime;

  // PIMPL to keep zeromq out of the interface for this class
  vtkVRCollaborationClientInternal* Internal;
};

#endif
