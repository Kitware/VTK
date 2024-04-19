// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRemoteInteractionAdapter.h"
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

enum
{
  WheelEvent = vtkCommand::UserEvent + 3000,
};

// map vtk-js event codes to vtkCommand events , for the ones that I didn't found a clear
// correspondence I used vtkCommand::NoEvent and left them unhandled. Taken from
// https://github.com/Kitware/vtk-js/blob/master/Sources/Rendering/Core/RenderWindowInteractor/index.js

using enum_type = int;
// clang-format off
const std::unordered_map< std::string, enum_type > EVENT_MAP {
  {"StartAnimation"             ,vtkCommand::NoEvent},
  {"Animation"                  ,vtkCommand::NoEvent},
  {"EndAnimation"               ,vtkCommand::NoEvent},
  {"PointerEnter"               ,vtkCommand::EnterEvent},
  {"PointerLeave"               ,vtkCommand::LeaveEvent},
  {"MouseEnter"                 ,vtkCommand::EnterEvent},
  {"MouseLeave"                 ,vtkCommand::LeaveEvent},
  {"StartMouseMove"             ,vtkCommand::NoEvent},
  {"MouseMove"                  ,vtkCommand::MouseMoveEvent},
  {"EndMouseMove"               ,vtkCommand::NoEvent},
  {"LeftButtonPress"            ,vtkCommand::LeftButtonPressEvent},
  {"LeftButtonRelease"          ,vtkCommand::LeftButtonReleaseEvent},
  {"MiddleButtonPress"          ,vtkCommand::MiddleButtonPressEvent},
  {"MiddleButtonRelease"        ,vtkCommand::MiddleButtonReleaseEvent},
  {"RightButtonPress"           ,vtkCommand::RightButtonPressEvent},
  {"RightButtonRelease"         ,vtkCommand::RightButtonReleaseEvent},
  {"KeyPress"                   ,vtkCommand::KeyPressEvent},
  {"KeyDown"                    ,vtkCommand::KeyPressEvent},
  {"KeyUp"                      ,vtkCommand::KeyReleaseEvent},
  {"StartMouseWheel"            ,vtkCommand::NoEvent},
  {"MouseWheel"                 ,WheelEvent},
  {"EndMouseWheel"              ,vtkCommand::NoEvent},
  {"StartPinch"                 ,vtkCommand::StartPinchEvent},
  {"Pinch"                      ,vtkCommand::PinchEvent},
  {"EndPinch"                   ,vtkCommand::EndPinchEvent},
  {"StartPan"                   ,vtkCommand::StartPanEvent},
  {"Pan"                        ,vtkCommand::PanEvent},
  {"EndPan"                     ,vtkCommand::EndPanEvent},
  {"StartRotate"                ,vtkCommand::StartRotateEvent},
  {"Rotate"                     ,vtkCommand::RotateEvent},
  {"EndRotate"                  ,vtkCommand::RenderEvent},
  {"Button3D"                   ,vtkCommand::NoEvent},
  {"Move3D"                     ,vtkCommand::NoEvent},
  {"StartPointerLock"           ,vtkCommand::NoEvent},
  {"EndPointerLock"             ,vtkCommand::NoEvent},
  {"StartInteraction"           ,vtkCommand::NoEvent},
  {"Interaction"                ,vtkCommand::NoEvent},
  {"EndInteraction"             ,vtkCommand::NoEvent},
  {"AnimationFrameRateUpdate"   ,vtkCommand::NoEvent}
};
// clang-format on

vtkSetObjectImplementationMacro(vtkRemoteInteractionAdapter, Interactor, vtkRenderWindowInteractor);
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkRemoteInteractionAdapter);

//----------------------------------------------------------------------------
vtkRemoteInteractionAdapter::vtkRemoteInteractionAdapter() = default;

//----------------------------------------------------------------------------
vtkRemoteInteractionAdapter::~vtkRemoteInteractionAdapter()
{
  this->SetInteractor(nullptr);
}

//----------------------------------------------------------------------------
void vtkRemoteInteractionAdapter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
// based on QVTKInteractorAdapter::ProcessEvent(QEvent* e, vtkRenderWindowInteractor* iren)
bool vtkRemoteInteractionAdapter::ProcessEvent(vtkRenderWindowInteractor* iren,
  const std::string& event_str, double devicePixelRatio, double devicePixelRatioTolerance)
{

  if (!iren)
  {
    vtkLogF(ERROR, "Null interactor passed");
    return false;
  }
  // the following events only happen if the interactor is enabled
  if (!iren->GetEnabled())
  {
    return false;
  }

  try
  {
    nlohmann::json event = nlohmann::json::parse(event_str);
    const std::string& type = event.at("type");
    vtkLogF(TRACE, "event %s", event.dump(1).c_str());
    const int eventType = EVENT_MAP.at(type);
    switch (eventType)
    {
      case vtkCommand::EnterEvent:
      case vtkCommand::LeaveEvent:
        iren->InvokeEvent(eventType, (void*)&event);
        break;
      case vtkCommand::MouseMoveEvent:
      case vtkCommand::LeftButtonPressEvent:
      case vtkCommand::LeftButtonReleaseEvent:
      case vtkCommand::RightButtonPressEvent:
      case vtkCommand::RightButtonReleaseEvent:
      case vtkCommand::MiddleButtonPressEvent:
      case vtkCommand::MiddleButtonReleaseEvent:
      {
        const int x =
          (event.at("x").get<double>() / event.at("w").get<double>() * devicePixelRatio +
            devicePixelRatioTolerance) *
          iren->GetRenderWindow()->GetSize()[0];
        const int y =
          (event.at("y").get<double>() / event.at("h").get<double>() * devicePixelRatio +
            devicePixelRatioTolerance) *
          iren->GetRenderWindow()->GetSize()[1];

        const int ctrlKeyPressed = event.at("ctrlKey").get<int>();
        const int altKeyPressed = event.at("altKey").get<int>();
        const int shiftKeyPressed = event.at("shiftKey").get<int>();
        iren->SetEventInformation(x, y, ctrlKeyPressed, shiftKeyPressed);
        iren->SetAltKey(altKeyPressed);
        iren->InvokeEvent(eventType, (void*)&event);
        break;
      }
      case vtkCommand::KeyPressEvent:
      case vtkCommand::KeyReleaseEvent:
      {
        const int ctrlKeyPressed = event.at("controlKey").get<int>();
        const int altKeyPressed = event.at("altKey").get<int>();
        const int shiftKeyPressed = event.at("shiftKey").get<int>();
        const char asciiCode = event.at("keyCode").get<int>();
        const std::string& key = event.at("key");
        iren->SetKeyEventInformation(ctrlKeyPressed, shiftKeyPressed, asciiCode, 0, key.c_str());
        iren->SetAltKey(altKeyPressed);
        iren->InvokeEvent(eventType);
        if (eventType == vtkCommand::KeyPressEvent && asciiCode != '\0') // TODO check comparson
        {
          iren->InvokeEvent(vtkCommand::CharEvent, (void*)&event);
        }
        break;
      }
      case WheelEvent:
      {
        const int x =
          (event.at("x").get<double>() / event.at("w").get<double>() * devicePixelRatio +
            devicePixelRatioTolerance) *
          iren->GetRenderWindow()->GetSize()[0];
        const int y =
          (event.at("y").get<double>() / event.at("h").get<double>() * devicePixelRatio +
            devicePixelRatioTolerance) *
          iren->GetRenderWindow()->GetSize()[1];

        const int ctrlKeyPressed = event.at("ctrlKey").get<int>();
        const int altKeyPressed = event.at("altKey").get<int>();
        const int shiftKeyPressed = event.at("shiftKey").get<int>();

        iren->SetEventInformation(x, y, ctrlKeyPressed, shiftKeyPressed);
        iren->SetAltKey(altKeyPressed);

        static double accumulatedDelta = 0;
        const double verticalDelta = event.at("spinY").get<double>();
        accumulatedDelta += verticalDelta;
        const double threshold = 1.0; // in vtk-js the value comes normalized

        // invoke vtk event when accumulated delta passes the threshold
        // Note: in javascript a forward (away from the user MouseWheelEvent is
        // indicated with a negative value in contrast to Qt.
        if (accumulatedDelta <= -threshold && verticalDelta != 0.0)
        {
          iren->InvokeEvent(vtkCommand::MouseWheelForwardEvent, (void*)&event);
          accumulatedDelta = 0;
        }
        else if (accumulatedDelta >= threshold && verticalDelta != 0.0)
        {
          iren->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, (void*)&event);
          accumulatedDelta = 0;
        }

        break;
      }
      case vtkCommand::StartPinchEvent:
      case vtkCommand::EndPinchEvent:
      case vtkCommand::PinchEvent:
      case vtkCommand::StartPanEvent:
      case vtkCommand::EndPanEvent:
      case vtkCommand::PanEvent:
      case vtkCommand::StartRotateEvent:
      case vtkCommand::EndRotateEvent:
      case vtkCommand::RotateEvent:
      {
        // Store event information to restore after gesture is completed
        int eventPosition[2];
        iren->GetEventPosition(eventPosition);
        int lastEventPosition[2];
        iren->GetLastEventPosition(lastEventPosition);

        // get center of positions for event
        int position[2] = { 0, 0 };
        for (const auto& item : event.at("positions"))
        {
          position[0] += item.at("x").get<double>() / event.at("w").get<double>();
          position[1] += item.at("y").get<double>() / event.at("h").get<double>();
        }

        position[0] /= static_cast<int>(event.at("positions").size());
        position[1] /= static_cast<int>(event.at("positions").size());

        iren->SetEventInformation(position[0] * devicePixelRatio * devicePixelRatioTolerance,
          position[1] * devicePixelRatio * devicePixelRatioTolerance);

        if (eventType == vtkCommand::StartPinchEvent || eventType == vtkCommand::EndPinchEvent ||
          eventType == vtkCommand::PinchEvent)
        {
          const double factor = event.at("factor").get<double>();
          iren->SetScale(1.0);
          iren->SetScale(factor);
        }
        else if (eventType == vtkCommand::StartPanEvent || eventType == vtkCommand::EndPanEvent ||
          eventType == vtkCommand::PanEvent)
        {
          double translation[2] = { event.at("translation").at(0).get<double>(),
            event.at("translation").at(1).get<double>() };
          iren->SetTranslation(translation);
        }
        else if (eventType == vtkCommand::StartRotateEvent ||
          eventType == vtkCommand::EndRotateEvent || eventType == vtkCommand::RotateEvent)
        {
          const double rotation = event.at("rotation").get<double>();
          iren->SetRotation(rotation);
        }
        else
        {
          vtkLogF(ERROR, "Unexpected Event Type");
          return false;
        }
        iren->InvokeEvent(eventType, (void*)&event);
      }
      break;

      case vtkCommand::InteractionEvent:
      case vtkCommand::StartInteractionEvent:
      case vtkCommand::EndInteractionEvent:
      case vtkCommand::NoEvent:
        // nothing to do
        break;
      default:
        vtkLogF(WARNING, "Unhandled event: %s", type.c_str());
        break;
    }
    return true;
  }
  catch (std::out_of_range& e)
  {
    vtkLogF(ERROR, "Skipping Event: Unknown event type \n%s", e.what());
    return false;
  }
  catch (nlohmann::json::out_of_range& e)
  {
    vtkLogF(ERROR, "Skipping Event \n%s", e.what());
    return false;
  }
}

//----------------------------------------------------------------------------
bool vtkRemoteInteractionAdapter::ProcessEvent(const std::string& event_str)
{
  return ProcessEvent(
    this->Interactor, event_str, this->DevicePixelRatio, this->DevicePixelRatioTolerance);
}

VTK_ABI_NAMESPACE_END
