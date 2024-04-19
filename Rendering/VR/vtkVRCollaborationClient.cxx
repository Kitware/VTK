// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkVRCollaborationClient.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLAvatar.h"
#include "vtkOpenGLRenderer.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkVRModel.h"
#include "vtkVRRenderWindow.h"

#include <sstream>
#include <zmq.h>
VTK_ABI_NAMESPACE_BEGIN
namespace
{
const double RAY_LENGTH = 200.0;    // in meters
const double AVATAR_TIMEOUT = 10.0; // in seconds
const int HEARTBEAT_INTERVAL = 1.0; // in seconds
const int LIVE_COUNT = 3;

// http://colorbrewer2.org/#type=qualitative&scheme=Pastel1&n=9
double AVATAR_COLORS[][3] = {
  { 179 / 255.0, 205 / 255.0, 227 / 255.0 },
  { 204 / 255.0, 235 / 255.0, 197 / 255.0 },
  { 222 / 255.0, 203 / 255.0, 228 / 255.0 },
  { 254 / 255.0, 217 / 255.0, 166 / 255.0 },
  { 255 / 255.0, 255 / 255.0, 204 / 255.0 },
  { 229 / 255.0, 216 / 255.0, 189 / 255.0 },
  { 253 / 255.0, 218 / 255.0, 236 / 255.0 },
  { 242 / 255.0, 242 / 255.0, 242 / 255.0 },
  { 251 / 255.0, 180 / 255.0, 174 / 255.0 },
};

const int NUM_COLORS = sizeof(AVATAR_COLORS) / sizeof(AVATAR_COLORS[0]);

// two local helper functions for libzmq
std::string _zmq_string_recv(void* socket, int flags)
{
  zmq_msg_t msg;
  int rc = zmq_msg_init(&msg);
  assert(rc == 0);
  rc = zmq_msg_recv(&msg, socket, flags);
  assert(rc != -1);
  return std::string(static_cast<const char*>(zmq_msg_data(&msg)), zmq_msg_size(&msg));
}

//  Receives all remaining message parts from socket, does nothing.
void _zmq_sock_clear(void* socket)
{
  int more;
  size_t more_size = sizeof(more);
  zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
  while (more)
  {
    //  Process all parts of the message
    zmq_msg_t part;
    int rc = zmq_msg_init(&part);
    assert(rc == 0);
    /* Block until a message is available to be received from socket */
    rc = zmq_msg_recv(&part, socket, 0);
    assert(rc != -1);

    zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
  }
}

} // end anon namespace

// simple macro to package up arguments to call Log()
#define mvLog(verbosity, x)                                                                        \
  std::ostringstream ss;                                                                           \
  ss << x;                                                                                         \
  this->Log(verbosity, ss.str());

// PIMPL to keep zeromq out of the class interface
class vtkVRCollaborationClientInternal
{
public:
  void* Context;
  void* Requester;
  void* Subscriber;
  zmq_pollitem_t CollabPollItems[2];

  vtkVRCollaborationClientInternal()
    : CollabPollItems{ { nullptr, 0, ZMQ_POLLIN, 0 }, { nullptr, 0, ZMQ_POLLIN, 0 } }
  {
    // create context
    this->Context = zmq_ctx_new();
    assert(this->Context);
    int rc = zmq_ctx_set(this->Context, ZMQ_IO_THREADS, 1);
    assert(rc == 0);
    rc = zmq_ctx_set(this->Context, ZMQ_MAX_SOCKETS, ZMQ_MAX_SOCKETS_DFLT);
    assert(rc == 0);

    this->Requester = zmq_socket(this->Context, ZMQ_DEALER);
    this->Subscriber = zmq_socket(this->Context, ZMQ_SUB);

    this->CollabPollItems[0].socket = this->Requester;
    this->CollabPollItems[1].socket = this->Subscriber;
  }

  ~vtkVRCollaborationClientInternal()
  {
    if (this->Requester)
    {
      zmq_close(this->Requester);
    }
    if (this->Subscriber)
    {
      zmq_close(this->Subscriber);
    }
    int rc;
    do
    {
      rc = zmq_ctx_term(this->Context);
    } while (rc == -1 && errno == EINTR);
  }
};

vtkStandardNewMacro(vtkVRCollaborationClient);

vtkVRCollaborationClient::vtkVRCollaborationClient()
  : Connected(false)
  , DisplayOwnAvatar(false)
  , MoveObserver(-1)
  , Callback(nullptr)
  , YourLastAvatarUpdateTime(0.0)
{
  this->Internal = new vtkVRCollaborationClientInternal();

  this->CollabPort = 5555;
  // Position MineView Zeromq, default when none is specified.
  this->CollabSession = "PMVZ";
  this->RetryCount = 1; // start in retry state.
  this->NeedHeartbeat = 0;
  this->NeedReply = 0;
  this->PublishAvailable = false; // publish socket not sending yet.

  this->EventCommand = vtkCallbackCommand::New();
  this->EventCommand->SetClientData(this);
  this->EventCommand->SetCallback(vtkVRCollaborationClient::EventCallback);

  // setup default scale callback
  this->ScaleCallback = [this]() {
    auto ovrrw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
    return ovrrw ? ovrrw->GetPhysicalScale() : 1.0;
  };
}

vtkVRCollaborationClient::~vtkVRCollaborationClient()
{
  this->Disconnect();
  this->EventCommand->Delete();
  delete this->Internal;
}

void vtkVRCollaborationClient::Log(vtkLogger::Verbosity verbosity, std::string const& msg)
{
  if (this->Callback)
  {
    this->Callback(msg, verbosity);
  }
  else
  {
    vtkWarningMacro(<< msg);
  }
}

void vtkVRCollaborationClient::Disconnect()
{
  if (!this->Connected)
  {
    return;
  }

  mvLog(vtkLogger::VERBOSITY_INFO, "Collab server disconnecting. " << std::endl);

  if (this->Internal->Requester != nullptr)
  {
    zmq_close(this->Internal->Requester);
  }
  if (this->Internal->Subscriber != nullptr)
  {
    zmq_close(this->Internal->Subscriber);
  }
  for (auto it : this->AvatarUpdateTime)
  {
    this->Renderer->RemoveActor(this->Avatars[it.first]);
    this->Avatars.erase(it.first);
  }
  this->AvatarUpdateTime.clear();

  if (this->MoveObserver >= 0 && this->RenderWindow && this->RenderWindow->GetInteractor())
  {
    this->RenderWindow->GetInteractor()->RemoveObserver(this->MoveObserver);
    this->MoveObserver = -1;
  }
  this->Connected = false;
  this->CollabID.clear();
}

void vtkVRCollaborationClient::AddArguments(vtksys::CommandLineArguments& arguments)
{
  typedef vtksys::CommandLineArguments argT;

  arguments.AddArgument("--collab-server", argT::EQUAL_ARGUMENT, &(this->CollabHost),
    "(optional) Connect to collaboration server at this hostname");
  arguments.AddArgument("--collab-port", argT::EQUAL_ARGUMENT, &(this->CollabPort),
    "(default:5555) Connect to collaboration server at this port");
  arguments.AddArgument("--collab-session", argT::EQUAL_ARGUMENT, &(this->CollabSession),
    "Connect to a separate collaboration session - each collaborator should use a matching value");
  arguments.AddArgument("--collab-name", argT::EQUAL_ARGUMENT, &(this->CollabName),
    "Name to display over your avatar to other collaborators");
  this->DisplayOwnAvatar = false;
  arguments.AddBooleanArgument("--show-my-avatar", &this->DisplayOwnAvatar,
    "(default false) Show an avatar at my own position.");
}

void vtkVRCollaborationClient::Render()
{
  if (this->Connected)
  {
    // if windowed update avatar position based on camera pos
    if (this->MoveObserver == -1)
    {
      this->UpdateAvatarPoseFromCamera();
    }
    this->HandleCollabMessage();
    this->EraseIdleAvatars();
  }
}

void vtkVRCollaborationClient::UpdateAvatarPoseFromCamera()
{
  // act like a Move3D event for the head
  int idevice = static_cast<int>(vtkEventDataDevice::HeadMountedDisplay);
  double* pos = this->DevicePoses[idevice].Position.data();
  double* orient = this->DevicePoses[idevice].Orientation.data();
  this->HasPoseForDevice[idevice] = true;

  this->Renderer->GetActiveCamera()->GetPosition(pos);
  auto cori = this->Renderer->GetActiveCamera()->GetOrientationWXYZ();

  // currently have a mismatch between wxyz and euler angles. Convert.
  this->TempTransform->Identity();
  this->TempTransform->RotateWXYZ(-cori[0], &cori[1]);
  // angles need to be rotated 90
  this->TempTransform->RotateY(90);
  this->TempTransform->GetOrientation(orient);

  this->SendLatestDevicePoses();
}

void vtkVRCollaborationClient::SendLatestDevicePoses()
{
  // don't send a message if we haven't gotten one during the last
  // heartbeat. View messages, however, are always sent (queued).
  if (this->RetryCount > 0)
  {
    return;
  }

  // throttle avatar pose updates
  double currentTime = vtkTimerLog::GetUniversalTime();
  if (currentTime - this->YourLastAvatarUpdateTime > 0.02)
  {
    // package up the device pose messages
    std::vector<int32_t> devices;
    std::vector<double> poses;
    bool haveHead = false;
    for (int i = 0; i < vtkEventDataNumberOfDevices; ++i)
    {
      if (this->HasPoseForDevice[i])
      {
        if (i == static_cast<int>(vtkEventDataDevice::HeadMountedDisplay))
        {
          haveHead = true;
        }
        devices.push_back(i);
        poses.insert(
          poses.end(), this->DevicePoses[i].Position.begin(), this->DevicePoses[i].Position.end());
        poses.insert(poses.end(), this->DevicePoses[i].Orientation.begin(),
          this->DevicePoses[i].Orientation.end());
      }
      this->HasPoseForDevice[i] = false;
    }

    // if no data ignore
    // Don't send hand messages without head data
    if (devices.size() == 0 || !haveHead)
    {
      return;
    }

    double scale = this->ScaleCallback();

    std::vector<vtkVRCollaborationClient::Argument> args;
    args.resize(3);
    args[0].SetInt32Vector(devices.data(), static_cast<int32_t>(devices.size()));
    args[1].SetDoubleVector(poses.data(), static_cast<int32_t>(poses.size()));
    args[2].SetDouble(scale);
    this->YourLastAvatarUpdateTime = currentTime;
    this->SendAMessage("A", args);
  }
}

void vtkVRCollaborationClient::SendAMessage(
  std::string const& msgType, std::vector<Argument> const& args)
{
  if (this->CollabID.empty())
  {
    return;
  }

  // send header, our ID, session.
  zmq_send_const(this->Internal->Requester, "PMVZ", 4, ZMQ_SNDMORE);
  zmq_send(this->Internal->Requester, this->CollabID.c_str(), this->CollabID.size(), ZMQ_SNDMORE);
  zmq_send(this->Internal->Requester, this->CollabSession.c_str(), this->CollabSession.size(),
    ZMQ_SNDMORE);
  zmq_send(this->Internal->Requester, msgType.c_str(), msgType.size(), ZMQ_SNDMORE);

  // send the number of arguments
  uint16_t numArgs = static_cast<uint16_t>(args.size());
  zmq_send(this->Internal->Requester, &numArgs, sizeof(numArgs), ZMQ_SNDMORE);

  // now send the arguments
  for (int i = 0; i < numArgs; ++i)
  {
    auto& arg = args[i];

    // send the arg type
    uint16_t type = static_cast<uint16_t>(arg.Type);
    zmq_send(this->Internal->Requester, &type, sizeof(type), ZMQ_SNDMORE);

    // send the arg count (how many in the vector)
    uint16_t count = static_cast<uint16_t>(arg.Count);
    zmq_send(this->Internal->Requester, &count, sizeof(count), ZMQ_SNDMORE);

    // finally send the data
    switch (arg.Type)
    {
      case Double:
      {
        zmq_send(this->Internal->Requester, arg.Data.get(), sizeof(double) * arg.Count,
          (i == numArgs - 1 ? 0 : ZMQ_SNDMORE));
        break;
      }
      case Int32:
      {
        zmq_send(this->Internal->Requester, arg.Data.get(), sizeof(int32_t) * arg.Count,
          (i == numArgs - 1 ? 0 : ZMQ_SNDMORE));
        break;
      }
      case String:
      {
        zmq_send(this->Internal->Requester, arg.Data.get(), arg.Count,
          (i == numArgs - 1 ? 0 : ZMQ_SNDMORE));
        break;
      }
    }
  }
}

bool vtkVRCollaborationClient::Argument::GetString(std::string& result)
{
  if (this->Type != String || !this->Data)
  {
    return false;
  }

  char* cstr = static_cast<char*>(this->Data.get());
  // make sure it is terminated
  cstr[this->Count - 1] = 0;
  result = std::string(cstr);
  return true;
}

void vtkVRCollaborationClient::Argument::SetString(std::string const& in)
{
  this->Type = String;
  this->Count = static_cast<uint16_t>(in.size() + 1);
  char* cdata = static_cast<char*>(malloc(in.size() + 1));
  this->Data = std::shared_ptr<void>(cdata, free);
  std::copy(in.begin(), in.end(), cdata);
  cdata[in.size()] = 0;
}

bool vtkVRCollaborationClient::Argument::GetStringVector(std::vector<std::string>& result)
{
  if (this->Type != String || !this->Data)
  {
    return false;
  }

  char* cstr = static_cast<char*>(this->Data.get());
  // make sure it is terminated
  cstr[this->Count - 1] = 0;

  size_t pos = 0;
  while (pos < this->Count)
  {
    std::string tmp = std::string(cstr + pos);
    result.push_back(tmp);
    pos += tmp.size() + 1;
  }

  return true;
}

void vtkVRCollaborationClient::Argument::SetStringVector(std::vector<std::string> const& in)
{
  this->Type = String;

  size_t byteCount = 0;
  for (size_t i = 0; i < in.size(); i++)
  {
    byteCount += in[i].size();
    byteCount++;
  }
  this->Count = static_cast<uint16_t>(byteCount);

  char* cdata = static_cast<char*>(malloc(byteCount));
  this->Data = std::shared_ptr<void>(cdata, free);

  char* cptr = cdata;
  for (size_t i = 0; i < in.size(); i++)
  {
    std::copy(in[i].begin(), in[i].end(), cptr);
    cptr += in[i].size();
    *cptr = 0;
    cptr++;
  }
}

bool vtkVRCollaborationClient::Argument::GetDoubleVector(std::vector<double>& result)
{
  if (this->Type != Double || !this->Data)
  {
    return false;
  }

  result.resize(this->Count);
  double* dptr = static_cast<double*>(this->Data.get());
  std::copy(dptr, dptr + this->Count, result.begin());
  return true;
}

void vtkVRCollaborationClient::Argument::SetDoubleVector(double const* in, uint16_t size)
{
  this->Type = Double;
  this->Count = size;
  double* cdata = static_cast<double*>(malloc(sizeof(double) * size));
  this->Data = std::shared_ptr<void>(cdata, free);
  std::copy(in, in + size, cdata);
}

void vtkVRCollaborationClient::Argument::SetDouble(double in)
{
  this->Type = Double;
  this->Count = 1;
  double* cdata = static_cast<double*>(malloc(sizeof(double)));
  cdata[0] = in;
  this->Data = std::shared_ptr<void>(cdata, free);
}

bool vtkVRCollaborationClient::Argument::GetDouble(double& result)
{
  if (this->Type != Double || !this->Data || this->Count != 1)
  {
    return false;
  }

  double* dptr = static_cast<double*>(this->Data.get());
  result = *dptr;
  return true;
}

bool vtkVRCollaborationClient::Argument::GetInt32Vector(std::vector<int32_t>& result)
{
  if (this->Type != Int32 || !this->Data)
  {
    return false;
  }

  result.resize(this->Count);
  int32_t* dptr = static_cast<int32_t*>(this->Data.get());
  std::copy(dptr, dptr + this->Count, result.begin());
  return true;
}

void vtkVRCollaborationClient::Argument::SetInt32Vector(int32_t const* in, uint16_t size)
{
  this->Type = Int32;
  this->Count = size;
  int32_t* cdata = static_cast<int32_t*>(malloc(sizeof(int32_t) * size));
  this->Data = std::shared_ptr<void>(cdata, free);
  std::copy(in, in + size, cdata);
}

void vtkVRCollaborationClient::Argument::SetInt32(int32_t in)
{
  this->Type = Int32;
  this->Count = 1;
  int32_t* cdata = static_cast<int32_t*>(malloc(sizeof(int32_t)));
  cdata[0] = in;
  this->Data = std::shared_ptr<void>(cdata, free);
}

bool vtkVRCollaborationClient::Argument::GetInt32(int32_t& result)
{
  if (this->Type != Int32 || !this->Data || this->Count != 1)
  {
    return false;
  }

  int32_t* dptr = static_cast<int32_t*>(this->Data.get());
  result = *dptr;
  return true;
}

std::vector<vtkVRCollaborationClient::Argument> vtkVRCollaborationClient::GetMessageArguments()
{
  std::vector<Argument> result;

  uint16_t numArgs = 0;
  zmq_recv(this->Internal->Subscriber, &numArgs, sizeof(numArgs), 0);

  result.resize(numArgs);

  for (int i = 0; i < numArgs; ++i)
  {
    Argument& arg = result[i];

    // get the arg type
    uint16_t argType = Double;
    zmq_recv(this->Internal->Subscriber, &argType, sizeof(argType), 0);
    arg.Type = static_cast<ArgumentType>(argType);

    // get the arg count
    uint16_t argCount = 0;
    zmq_recv(this->Internal->Subscriber, &argCount, sizeof(argCount), 0);
    arg.Count = argCount;

    switch (arg.Type)
    {
      case Double:
      {
        arg.Data = std::shared_ptr<void>(malloc(sizeof(double) * arg.Count), free);
        auto zresult =
          zmq_recv(this->Internal->Subscriber, arg.Data.get(), sizeof(double) * arg.Count, 0);
        if (zresult != sizeof(double) * arg.Count)
        {
          vtkErrorMacro("failed to get valid argument");
        }
        break;
      }
      case Int32:
      {
        arg.Data = std::shared_ptr<void>(malloc(sizeof(int32_t) * arg.Count), free);
        auto zresult =
          zmq_recv(this->Internal->Subscriber, arg.Data.get(), sizeof(int32_t) * arg.Count, 0);
        if (zresult != sizeof(int32_t) * arg.Count)
        {
          vtkErrorMacro("failed to get valid argument");
        }
        break;
      }
      case String:
      {
        arg.Data = std::shared_ptr<void>(malloc(arg.Count), free);
        auto zresult = zmq_recv(this->Internal->Subscriber, arg.Data.get(), arg.Count, 0);
        if (zresult != arg.Count)
        {
          vtkErrorMacro("failed to get valid argument");
        }
        break;
      }
    }
  }

  return result;
}

void vtkVRCollaborationClient::SendAMessage(std::string const& msgType)
{
  if (this->CollabID.empty())
  {
    return;
  }
  // send header, our ID, session.
  zmq_send_const(this->Internal->Requester, "PMVZ", 4, ZMQ_SNDMORE);
  zmq_send(this->Internal->Requester, this->CollabID.c_str(), this->CollabID.size(), ZMQ_SNDMORE);
  zmq_send(this->Internal->Requester, this->CollabSession.c_str(), this->CollabSession.size(),
    ZMQ_SNDMORE);
  zmq_send(this->Internal->Requester, msgType.c_str(), msgType.size(), 0);
}

void vtkVRCollaborationClient::SendPoseMessage(
  std::string const& msgType, int index, double pos[3], double dir[3])
{
  std::vector<vtkVRCollaborationClient::Argument> args;
  args.resize(3);
  args[0].SetInt32(index);
  args[1].SetDoubleVector(pos, 3);
  args[2].SetDoubleVector(dir, 3);
  this->SendAMessage(msgType, args);
}

void vtkVRCollaborationClient::HandleBroadcastMessage(
  std::string const& otherID, std::string const& type)
{
  if (type == "A")
  {
    std::vector<Argument> args = this->GetMessageArguments();

    std::vector<double> poses;
    std::vector<int32_t> devices;
    double ascale = 1.0;
    if (args.size() != 3 || !args[0].GetInt32Vector(devices) || !args[1].GetDoubleVector(poses) ||
      !args[2].GetDouble(ascale))
    {
      mvLog(vtkLogger::VERBOSITY_ERROR,
        "Incorrect arguments for A (avatar pose) collaboration message" << std::endl);
      return;
    }

    // if this update is from us, we ignore it by default.
    if (otherID != this->CollabID || this->DisplayOwnAvatar)
    {
      double scale = this->ScaleCallback();
      auto avatar = this->GetAvatar(otherID);
      avatar->SetScale(0.3 * scale);

      bool haveLeft = false;
      bool haveRight = false;
      for (size_t i = 0; i < devices.size(); ++i)
      {
        vtkEventDataDevice device = static_cast<vtkEventDataDevice>(devices[i]);

        double* updatePos = poses.data() + i * 7;
        double* updateOrient = poses.data() + i * 7 + 3;

        if (device == vtkEventDataDevice::LeftController)
        {
          avatar->SetLeftHandPosition(updatePos);
          avatar->SetLeftHandOrientation(updateOrient);
          if (!avatar->GetUseLeftHand())
          {
            avatar->UseLeftHandOn();
          }
          haveLeft = true;
        }
        else if (device == vtkEventDataDevice::RightController)
        {
          avatar->SetRightHandPosition(updatePos);
          avatar->SetRightHandOrientation(updateOrient);
          if (!avatar->GetUseRightHand())
          {
            avatar->UseRightHandOn();
          }
          haveRight = true;
        }
        else if (device == vtkEventDataDevice::HeadMountedDisplay)
        {
          avatar->SetHeadPosition(updatePos);
          avatar->SetHeadOrientation(updateOrient);
        }
        this->AvatarUpdateTime[otherID][static_cast<int>(device)] = vtkTimerLog::GetUniversalTime();
      }

      // adjust hand positions based on sending avatar scale
      double adjustment = scale / ascale;
      auto* headPos = avatar->GetHeadPosition();
      if (haveRight)
      {
        auto* handPos = avatar->GetRightHandPosition();
        avatar->SetRightHandPosition(headPos[0] + adjustment * (handPos[0] - headPos[0]),
          headPos[1] + adjustment * (handPos[1] - headPos[1]),
          headPos[2] + adjustment * (handPos[2] - headPos[2]));
      }
      if (haveLeft)
      {
        auto* handPos = avatar->GetLeftHandPosition();
        avatar->SetLeftHandPosition(headPos[0] + adjustment * (handPos[0] - headPos[0]),
          headPos[1] + adjustment * (handPos[1] - headPos[1]),
          headPos[2] + adjustment * (handPos[2] - headPos[2]));
      }
    }

    // Check if we were idle, and re-send join messages.
    if (otherID == this->CollabID && this->AvatarIdle(this->CollabID))
    {
      mvLog(vtkLogger::VERBOSITY_INFO, "Collab " << otherID << " return from idle " << std::endl);

      std::vector<vtkVRCollaborationClient::Argument> args2;
      args2.resize(1);
      args2[0].SetString(this->CollabID);
      this->SendAMessage("J", args2);
    }
  }
  else if (type == "J")
  {
    std::vector<Argument> args = this->GetMessageArguments();

    std::string extraID;
    if (args.size() != 1 || !args[0].GetString(extraID))
    {
      mvLog(vtkLogger::VERBOSITY_ERROR,
        "Incorrect arguments for J (join) collaboration message" << std::endl);
      return;
    }

    // Join message, send our list of views.
    // if we are idle, don't respond to join messages - send a join when
    // we are not idle anymore.
    if (this->AvatarIdle(this->CollabID))
    {
      return;
    }
    mvLog(vtkLogger::VERBOSITY_INFO, "Collab " << otherID << ", Join" << std::endl);
    if (!this->CollabName.empty())
    {
      std::vector<vtkVRCollaborationClient::Argument> args2;
      args2.resize(1);
      args2[0].SetString(this->CollabName);
      this->SendAMessage("N", args2);
    }
  }
  else if (type == "SR" || type == "HR")
  {
    // show/hide a ray
    std::vector<Argument> args = this->GetMessageArguments();
    int32_t device;
    if (args.size() != 1 || !args[0].GetInt32(device))
    {
      mvLog(vtkLogger::VERBOSITY_ERROR,
        "Incorrect arguments for SR/HR (ray) collaboration message" << std::endl);
      return;
    }

    bool show = (type == "SR");
    if (this->Avatars.count(otherID) != 0)
    {
      auto avatar = this->GetAvatar(otherID);
      if (device == static_cast<int>(vtkEventDataDevice::LeftController))
      {
        avatar->SetLeftShowRay(show);
      }
      else if (device == static_cast<int>(vtkEventDataDevice::RightController))
      {
        avatar->SetRightShowRay(show);
      }
      double scale = this->ScaleCallback();
      avatar->SetRayLength(RAY_LENGTH * scale);
    }
  }
  else if (type == "N")
  {
    std::vector<Argument> args = this->GetMessageArguments();
    // Set avatar's name, displayed above head.
    std::string avatarName;
    if (args.size() != 1 || !args[0].GetString(avatarName))
    {
      mvLog(vtkLogger::VERBOSITY_ERROR,
        "Incorrect arguments for N (name) collaboration message" << std::endl);
      return;
    }
    mvLog(vtkLogger::VERBOSITY_INFO, "Collab " << otherID << ", Name " << avatarName << std::endl);
    if (!avatarName.empty() && otherID != this->CollabID)
    {
      this->GetAvatar(otherID)->SetLabel(avatarName.c_str());
    }
  }
}

vtkSmartPointer<vtkOpenGLAvatar> vtkVRCollaborationClient::GetAvatar(std::string otherID)
{
  // if it's from a new collaborator, add an avatar
  if (this->Avatars.count(otherID) == 0)
  {
    mvLog(vtkLogger::VERBOSITY_INFO, "Adding Avatar " << otherID << std::endl);
    this->Avatars[otherID] = vtkSmartPointer<vtkOpenGLAvatar>::New();
    auto newAvatar = this->Avatars[otherID];
    this->Renderer->AddActor(newAvatar);
    // meters -> ft conversion.
    double scale = this->ScaleCallback();
    newAvatar->SetScale(0.3 * scale);
    newAvatar->SetUpVector(0, 0, 1);
    size_t colorIndex = this->Avatars.size() - 1;
    // base the color on the server's index of avatars.
    try
    {
      colorIndex = std::stoi(otherID);
    }
    catch (...)
    {
    }
    newAvatar->GetProperty()->SetColor(AVATAR_COLORS[(colorIndex) % NUM_COLORS]);
    newAvatar->GetLabelTextProperty()->SetColor(AVATAR_COLORS[(colorIndex) % NUM_COLORS]);
    newAvatar->GetLabelTextProperty()->SetFontSize(16);
    if (otherID == this->CollabID)
    {
      // Display only the hands
      newAvatar->SetShowHandsOnly(true);
      auto ovrrw = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
      if (ovrrw)
      {
        vtkVRModel* cmodel = ovrrw->GetModelForDevice(vtkEventDataDevice::LeftController);
        if (cmodel)
        {
          cmodel->SetVisibility(false);
        }
        cmodel = ovrrw->GetModelForDevice(vtkEventDataDevice::RightController);
        if (cmodel)
        {
          cmodel->SetVisibility(false);
        }
      }
    }
    for (int i = 0; i < vtkEventDataNumberOfDevices; ++i)
    {
      this->AvatarUpdateTime[otherID][i] = 0;
    }
  }
  return this->Avatars[otherID];
}

void vtkVRCollaborationClient::HandleCollabMessage()
{
  double currTime = vtkTimerLog::GetUniversalTime();
  bool receivedMsg = true;
  do
  {
    // timeout is 0, return immediately.
    zmq_poll(&(this->Internal->CollabPollItems)[0], 2, 0);
    if (this->Internal->CollabPollItems[0].revents & ZMQ_POLLIN)
    {
      // reply on the request-reply (dealer) socket - expect ID or error.
      std::string reply = _zmq_string_recv(this->Internal->Requester, ZMQ_DONTWAIT);
      if (reply == "ERROR")
      {
        mvLog(vtkLogger::VERBOSITY_ERROR, "Collab server returned error " << std::endl);
      }
      else if (reply == "pong")
      {
        // update server alive time, below.
      }
      else if (reply.empty())
      {
        // error, do nothing.
        mvLog(vtkLogger::VERBOSITY_ERROR, "Error: empty reply " << std::endl);
      }
      else
      {
        this->CollabID = reply;
        mvLog(vtkLogger::VERBOSITY_INFO, "Received ID " << this->CollabID << std::endl);
        this->RetryCount = 0;
        // ideally send "J" join message here, but pub-sub not ready yet.
      }
    }

    // handle broadcast messages
    //
    // A - avatar position update
    // J - New client joined message
    // N - Client name
    // SR/HR - show or hide a ray
    // V - View change
    // P - New TourStop
    // VL - ViewList
    //
    if (this->Internal->CollabPollItems[1].revents & ZMQ_POLLIN)
    {
      std::string sig = _zmq_string_recv(this->Internal->Subscriber, ZMQ_DONTWAIT);
      if (sig.size())
      {
        // verify the signature
        // we can get bad data, so make sure the first message contains the
        // correct data before requesting other pieces (which could block
        // and hang the app if the data was bad)
        if (sig == this->CollabSession)
        {
          // the first sub-msg contains the session string for the subscription
          //  process other avatar updates
          std::string otherID = _zmq_string_recv(this->Internal->Subscriber, ZMQ_DONTWAIT);
          std::string type = _zmq_string_recv(this->Internal->Subscriber, ZMQ_DONTWAIT);
          if (otherID.empty() || type.empty())
          {
            // error, ignore
            mvLog(vtkLogger::VERBOSITY_ERROR, "empty ID or ID " << otherID << ",  " << type);
            _zmq_sock_clear(this->Internal->Subscriber);
            continue;
          }

          this->HandleBroadcastMessage(otherID, type);
        }
        else
        {
          mvLog(vtkLogger::VERBOSITY_ERROR,
            "Error: mismatched session header with signature of: " << sig);
          _zmq_sock_clear(this->Internal->Subscriber);
        }

        // we got a message on the publish socket, see if this is the first one.
        if (!this->PublishAvailable)
        {
          this->PublishAvailable = true;
          // send join message, to trigger view setup.
          std::vector<vtkVRCollaborationClient::Argument> args;
          args.resize(1);
          args[0].SetString(this->CollabID);
          this->SendAMessage("J", args);
        }
      }
      else
      {
        mvLog(vtkLogger::VERBOSITY_ERROR, "Error: empty session header");
        _zmq_sock_clear(this->Internal->Subscriber);
        continue;
      }
    }

    receivedMsg = (this->Internal->CollabPollItems[0].revents & ZMQ_POLLIN ||
      this->Internal->CollabPollItems[1].revents & ZMQ_POLLIN);
    if (receivedMsg)
    {
      // got a message, reset heartbeat.
      this->NeedHeartbeat = currTime + HEARTBEAT_INTERVAL;
      this->NeedReply = currTime + HEARTBEAT_INTERVAL * LIVE_COUNT;
      this->RetryCount = 0;
    }
    else if (currTime > this->NeedHeartbeat && !this->CollabID.empty())
    {
      // heartbeat only if we have an ID. send ping, expect pong
      if (this->RetryCount == 0)
      {
        this->RetryCount = 1;
      }
      zmq_send_const(this->Internal->Requester, "ping", 4, ZMQ_SNDMORE);
      zmq_send(this->Internal->Requester, this->CollabID.c_str(), this->CollabID.size(), 0);
      this->NeedHeartbeat = currTime + HEARTBEAT_INTERVAL;
    }

    // if heartbeat fails multiple times
    if (currTime > this->NeedReply)
    {
      if (this->RetryCount > LIVE_COUNT)
      {
        this->NeedReply = currTime + HEARTBEAT_INTERVAL * LIVE_COUNT * this->RetryCount;
        mvLog(vtkLogger::VERBOSITY_WARNING, "Collab server disconnected, waiting. " << std::endl);
      }
      else
      {
        mvLog(vtkLogger::VERBOSITY_WARNING,
          "Collab server not responding, retry " << this->RetryCount << std::endl);
        ++this->RetryCount;
        // disconnect and reconnect sockets, clear ID
        this->Initialize(this->Renderer);
      }
    }
  } while (receivedMsg);
}

bool vtkVRCollaborationClient::AvatarIdle(std::string id)
{
  double currTime = vtkTimerLog::GetUniversalTime();
  auto times = this->AvatarUpdateTime[id];

  // if we've never received a head position message, the avatar isn't idle.
  if (times[0] == 0)
  {
    return false;
  }

  double avatarTime = times[0];
  // consider ourselves idle slightly before any collaborators do, avoiding races.
  double timeout = id == this->CollabID ? 0.98 * AVATAR_TIMEOUT : AVATAR_TIMEOUT;
  return (currTime - avatarTime > timeout);
}

void vtkVRCollaborationClient::EraseIdleAvatars()
{
  double currTime = vtkTimerLog::GetUniversalTime();
  for (auto it : this->AvatarUpdateTime)
  {
    if (it.second[0] == 0)
    {
      continue;
    }
    double avatarTime = it.second[0];
    if (currTime - avatarTime > AVATAR_TIMEOUT && it.first != this->CollabID &&
      this->Avatars.count(it.first) != 0)
    {
      mvLog(vtkLogger::VERBOSITY_INFO, "Removing Avatar: " << it.first << std::endl);
      this->Renderer->RemoveActor(this->Avatars[it.first]);
      this->Avatars.erase(it.first);
      this->AvatarUpdateTime.erase(it.first);
      // send join message, to trigger view setup.
      std::vector<vtkVRCollaborationClient::Argument> args;
      args.resize(1);
      args[0].SetString(this->CollabID);
      this->SendAMessage("J", args);
      break;
    }

    if (this->Avatars.count(it.first) == 0)
    {
      continue;
    }

    // see if the hands are idle, or not present at all.
    int device = static_cast<int>(vtkEventDataDevice::LeftController);
    if (currTime - it.second[device] > AVATAR_TIMEOUT)
    {
      auto currAvatar = this->Avatars[it.first];
      if (currAvatar->GetUseLeftHand())
      {
        currAvatar->UseLeftHandOff();
      }
    }
    device = static_cast<int>(vtkEventDataDevice::RightController);
    if (currTime - it.second[device] > AVATAR_TIMEOUT)
    {
      auto currAvatar = this->Avatars[it.first];
      if (currAvatar->GetUseRightHand())
      {
        currAvatar->UseRightHandOff();
      }
    }
  }
}

void vtkVRCollaborationClient::EventCallback(
  vtkObject*, unsigned long eventID, void* clientdata, void* calldata)
{
  vtkVRCollaborationClient* self = static_cast<vtkVRCollaborationClient*>(clientdata);

  if (eventID == vtkCommand::Move3DEvent)
  {
    vtkEventData* edata = static_cast<vtkEventData*>(calldata);
    vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
    if (!edd)
    {
      return;
    }

    auto device = edd->GetDevice();
    int idevice = static_cast<int>(device);
    if (device == vtkEventDataDevice::LeftController ||
      device == vtkEventDataDevice::RightController ||
      device == vtkEventDataDevice::HeadMountedDisplay)
    {
      double* pos = self->DevicePoses[idevice].Position.data();
      double* orient = self->DevicePoses[idevice].Orientation.data();
      edd->GetWorldPosition(pos);
      // empirically, the Oculus sometimes gives nonsense positions
      if (fabs(pos[0]) > 1e07)
      {
        return;
      }
      double wxyz[4] = { 0 };
      edd->GetWorldOrientation(wxyz);

      // currently have a mismatch between wxyz and euler angles. Convert.
      self->TempTransform->Identity();
      self->TempTransform->RotateWXYZ(wxyz[0], &wxyz[1]);
      // angles need to be rotated 90
      self->TempTransform->RotateY(90);
      self->TempTransform->GetOrientation(orient);

      // hands are also too far forward in x.
      if (device != vtkEventDataDevice::HeadMountedDisplay)
      {
        double adjust[3] = { -0.15, 0, 0 };
        self->TempTransform->TransformPoint(adjust, adjust);
        pos[0] += adjust[0];
        pos[1] += adjust[1];
        pos[2] += adjust[2];
      }
      self->HasPoseForDevice[idevice] = true;
      self->SendLatestDevicePoses();
    }
    return;
  }
}

// disconnect if needed, then connect to server.
// Retry count is set externally.
bool vtkVRCollaborationClient::Initialize(vtkOpenGLRenderer* ren)
{
  if (!ren)
  {
    return false;
  }

  this->Renderer = ren;
  this->RenderWindow = static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow());

  if (this->CollabHost.empty())
  {
    return false;
  }

  if (this->RetryCount == 1)
  {
    mvLog(vtkLogger::VERBOSITY_INFO, "Connecting to collaboration server..." << std::endl);
  }
  std::stringstream ss;
  ss << "tcp://" << this->CollabHost << ":" << this->CollabPort;
  std::string requesterEndpoint = ss.str();
  ss.str(std::string());
  ss << "tcp://" << this->CollabHost << ":" << (this->CollabPort + 1);
  std::string subscriberEndpoint = ss.str();
  if (this->Internal->Requester != nullptr)
  {
    zmq_close(this->Internal->Requester);
  }
  if (this->Internal->Subscriber != nullptr)
  {
    zmq_close(this->Internal->Subscriber);
  }
  this->Connected = false;
  this->Internal->Requester = zmq_socket(this->Internal->Context, ZMQ_DEALER);
  this->Internal->Subscriber = zmq_socket(this->Internal->Context, ZMQ_SUB);

  zmq_connect(this->Internal->Requester, requesterEndpoint.c_str());
  zmq_connect(this->Internal->Subscriber, subscriberEndpoint.c_str());
  // Subscribe to messages for our session, subscription required by zmq
  // We won't receive messages from other sessions.
  zmq_setsockopt(this->Internal->Subscriber, ZMQ_SUBSCRIBE, this->CollabSession.c_str(),
    this->CollabSession.size());
  // once we close, we want the socket to close immediately, and drop messages.
  int linger = 0;
  zmq_setsockopt(this->Internal->Requester, ZMQ_LINGER, &linger, sizeof(linger));
  this->Internal->CollabPollItems[0].socket = this->Internal->Requester;
  this->Internal->CollabPollItems[1].socket = this->Internal->Subscriber;
  this->Connected = true;

  this->CollabID.clear();
  double currTime = vtkTimerLog::GetUniversalTime();
  this->NeedHeartbeat = currTime + HEARTBEAT_INTERVAL;
  this->NeedReply = currTime + HEARTBEAT_INTERVAL * LIVE_COUNT * this->RetryCount;
  this->PublishAvailable = false;
  zmq_send_const(this->Internal->Requester, "HelloPMVZ", 9, 0);
  // async reply, so get ID in HandleCollabMessage()

  // add observer based on VR versus windowed
  if (this->RenderWindow->IsA("vtkVRRenderWindow"))
  {
    if (this->MoveObserver == -1)
    {
      this->MoveObserver = this->RenderWindow->GetInteractor()->AddObserver(
        vtkCommand::Move3DEvent, this->EventCommand, 1.0);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkVRCollaborationClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
