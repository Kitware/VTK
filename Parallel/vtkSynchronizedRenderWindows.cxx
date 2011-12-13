/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSynchronizedRenderWindows.h"

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkWeakPointer.h"

#include <map>

//----------------------------------------------------------------------------
class vtkSynchronizedRenderWindows::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New()
    {
    vtkObserver* obs = new vtkObserver();
    obs->Target = NULL;
    return obs;
    }

  virtual void Execute(vtkObject *, unsigned long eventId, void *)
    {
    if (this->Target)
      {
      switch (eventId)
        {
      case vtkCommand::StartEvent:
        this->Target->HandleStartRender();
        break;

      case vtkCommand::EndEvent:
        this->Target->HandleEndRender();
        break;

      case vtkCommand::AbortCheckEvent:
        this->Target->HandleAbortRender();
        break;
        }
      }
    }

  vtkSynchronizedRenderWindows* Target;
};

//----------------------------------------------------------------------------
namespace
{
  typedef std::map<unsigned int, vtkWeakPointer<vtkSynchronizedRenderWindows> >
    GlobalSynRenderWindowsMapType;
  GlobalSynRenderWindowsMapType GlobalSynRenderWindowsMap;

  void RenderRMI(void *vtkNotUsed(localArg), 
    void *remoteArg, int remoteArgLength,
    int vtkNotUsed(remoteProcessId))
    {
    vtkMultiProcessStream stream;
    stream.SetRawData(reinterpret_cast<unsigned char*>(remoteArg),
      remoteArgLength);
    unsigned int id = 0;
    stream >> id;
    GlobalSynRenderWindowsMapType::iterator iter =
      GlobalSynRenderWindowsMap.find(id);
    if (iter != GlobalSynRenderWindowsMap.end() &&
      iter->second.GetPointer() != NULL &&
      iter->second.GetPointer()->GetRenderWindow() != NULL)
      {
      iter->second.GetPointer()->GetRenderWindow()->Render();
      }
    }
};

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSynchronizedRenderWindows);
//----------------------------------------------------------------------------
vtkSynchronizedRenderWindows::vtkSynchronizedRenderWindows()
{
  this->Observer = vtkSynchronizedRenderWindows::vtkObserver::New();
  this->Observer->Target = this;

  this->RenderWindow = 0;
  this->ParallelController = 0;
  this->Identifier = 0;
  this->ParallelRendering = true;
  this->RenderEventPropagation = true;
  this->RootProcessId = 0;
}

//----------------------------------------------------------------------------
vtkSynchronizedRenderWindows::~vtkSynchronizedRenderWindows()
{
  this->SetIdentifier(0);

  this->Observer->Target = NULL;

  this->SetRenderWindow(0);
  this->SetParallelController(0);
  this->Observer->Delete();
  this->Observer = NULL;
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::SetIdentifier(unsigned int id)
{
  if (this->Identifier == id)
    {
    return;
    }

  if (this->Identifier != 0)
    {
    GlobalSynRenderWindowsMap.erase(this->Identifier);
    this->Identifier = 0;
    }

  GlobalSynRenderWindowsMapType::iterator iter =
    GlobalSynRenderWindowsMap.find(id);
  if (iter != GlobalSynRenderWindowsMap.end())
    {
    vtkErrorMacro("Identifier already in use: " << id);
    return;
    }

  this->Identifier = id;
  if (id > 0)
    {
    GlobalSynRenderWindowsMap[id] = this;
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::SetRenderWindow(vtkRenderWindow* renWin)
{
  if (this->RenderWindow != renWin)
    {
    if (this->RenderWindow)
      {
      this->RenderWindow->RemoveObserver(this->Observer);
      }
    vtkSetObjectBodyMacro(RenderWindow, vtkRenderWindow, renWin);
    if (this->RenderWindow)
      {
      this->RenderWindow->AddObserver(vtkCommand::StartEvent, this->Observer);
      this->RenderWindow->AddObserver(vtkCommand::EndEvent, this->Observer);
      // this->RenderWindow->AddObserver(vtkCommand::AbortCheckEvent, this->Observer);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::SetParallelController(
  vtkMultiProcessController* controller)
{
  if (this->ParallelController == controller)
    {
    return;
    }
 
  vtkSetObjectBodyMacro(
    ParallelController, vtkMultiProcessController, controller);

  if (controller)
    {
    // no harm in adding this mutliple times.
    controller->AddRMI(::RenderRMI, NULL, SYNC_RENDER_TAG);
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::AbortRender()
{
  if (this->ParallelRendering &&
      this->ParallelController &&
      this->ParallelController->GetLocalProcessId() == this->RootProcessId)
    {
    //TODO: trigger abort render message.
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::HandleStartRender()
{
  if (!this->RenderWindow || !this->ParallelRendering ||
    !this->ParallelController || 
    (!this->Identifier && this->RenderEventPropagation))
    {
    return;
    }

  if (this->ParallelController->GetLocalProcessId() == this->RootProcessId)
    {
    this->MasterStartRender();
    }
  else
    {
    this->SlaveStartRender();
    }
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::MasterStartRender()
{
  if (this->RenderEventPropagation)
    {
    vtkMultiProcessStream stream;
    stream << this->Identifier;

    std::vector<unsigned char> data;
    stream.GetRawData(data);

    this->ParallelController->TriggerRMIOnAllChildren(
      &data[0], static_cast<int>(data.size()), SYNC_RENDER_TAG);
    }

  RenderWindowInfo windowInfo;
  windowInfo.CopyFrom(this->RenderWindow);

  vtkMultiProcessStream stream;
  windowInfo.Save(stream);
  this->ParallelController->Broadcast(stream, this->RootProcessId);
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::SlaveStartRender()
{
  vtkMultiProcessStream stream;
  this->ParallelController->Broadcast(stream, this->RootProcessId);

  RenderWindowInfo windowInfo;
  windowInfo.Restore(stream);
  windowInfo.CopyTo(this->RenderWindow);
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Identifier: " << this->Identifier << endl;
  os << indent << "ParallelRendering: " << this->ParallelRendering << endl;
  os << indent << "RootProcessId: " << this->RootProcessId << endl;
  os << indent << "RenderEventPropagation: " << this->RenderEventPropagation
     << endl;

  os << indent << "RenderWindow: ";
  if(this->RenderWindow==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->RenderWindow << endl;
    }
  if(this->ParallelController==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->ParallelController << endl;
    }
}

//----------------------------------------------------------------------------
// ********* INFO OBJECT METHODS ***************************
//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::RenderWindowInfo::Save(vtkMultiProcessStream& stream)
{
  stream << 1208
    << this->WindowSize[0] << this->WindowSize[1]
    << this->TileScale[0] << this->TileScale[1]
    << this->TileViewport[0]
    << this->TileViewport[1]
    << this->TileViewport[2]
    << this->TileViewport[3]
    << this->DesiredUpdateRate;
}

//----------------------------------------------------------------------------
bool vtkSynchronizedRenderWindows::RenderWindowInfo::Restore(vtkMultiProcessStream& stream)
{
  int tag;
  stream >> tag;
  if (tag != 1208)
    {
    return false;
    }

  stream >> this->WindowSize[0] >> this->WindowSize[1]
    >> this->TileScale[0] >> this->TileScale[1]
    >> this->TileViewport[0]
    >> this->TileViewport[1]
    >> this->TileViewport[2]
    >> this->TileViewport[3]
    >> this->DesiredUpdateRate;
  return true;
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::RenderWindowInfo::CopyFrom(
  vtkRenderWindow *win)
{
  this->WindowSize[0] = win->GetActualSize()[0];
  this->WindowSize[1] = win->GetActualSize()[1];
  this->DesiredUpdateRate = win->GetDesiredUpdateRate();
  win->GetTileScale(this->TileScale);
  win->GetTileViewport(this->TileViewport);
}

//----------------------------------------------------------------------------
void vtkSynchronizedRenderWindows::RenderWindowInfo::CopyTo(
  vtkRenderWindow *win)
{
  win->SetSize(this->WindowSize[0], this->WindowSize[1]);
  win->SetTileScale(this->TileScale);
  win->SetTileViewport(this->TileViewport);
  win->SetDesiredUpdateRate(this->DesiredUpdateRate);
}
