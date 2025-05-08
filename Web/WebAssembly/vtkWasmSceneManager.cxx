// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWasmSceneManager.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWebAssemblyRenderWindowInteractor.h"

// Init factories.
#ifdef VTK_MODULE_ENABLE_VTK_RenderingContextOpenGL2
#include "vtkRenderingContextOpenGL2Module.h"
#endif
#ifdef VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
#include "vtkOpenGLPolyDataMapper.h" // needed to remove unused mapper, also includes vtkRenderingOpenGL2Module.h
#include "vtkWebAssemblyOpenGLRenderWindow.h"
#endif
#ifdef VTK_MODULE_ENABLE_VTK_RenderingUI
#include "vtkRenderingUIModule.h"
#endif
#ifdef VTK_MODULE_ENABLE_VTK_RenderingVolumeOpenGL2
#include "vtkRenderingVolumeOpenGL2Module.h"
#endif

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWasmSceneManager);

//-------------------------------------------------------------------------------
vtkWasmSceneManager::vtkWasmSceneManager() = default;

//-------------------------------------------------------------------------------
vtkWasmSceneManager::~vtkWasmSceneManager() = default;

//-------------------------------------------------------------------------------
bool vtkWasmSceneManager::Initialize()
{
  bool result = this->Superclass::Initialize();
#ifdef VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
  // Remove the default vtkOpenGLPolyDataMapper as it is not used with wasm build.
  /// get rid of serialization handler
  this->Serializer->UnRegisterHandler(typeid(vtkOpenGLPolyDataMapper));
  /// get rid of de-serialization handler
  this->Deserializer->UnRegisterHandler(typeid(vtkOpenGLPolyDataMapper));
  /// get rid of constructor
  this->Deserializer->UnRegisterConstructor("vtkOpenGLPolyDataMapper");
#endif
  return result;
}

//-------------------------------------------------------------------------------
void vtkWasmSceneManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObjectManager::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------------
bool vtkWasmSceneManager::SetSize(vtkTypeUInt32 identifier, int width, int height)
{
  auto object = this->GetObjectAtId(identifier);
  if (auto renderWindow = vtkRenderWindow::SafeDownCast(object))
  {
    if (auto iren = renderWindow->GetInteractor())
    {
      iren->UpdateSize(width, height);
      return true;
    }
  }
  return false;
}

//-------------------------------------------------------------------------------
bool vtkWasmSceneManager::Render(vtkTypeUInt32 identifier)
{
  auto object = this->GetObjectAtId(identifier);
  if (auto renderWindow = vtkRenderWindow::SafeDownCast(object))
  {
    renderWindow->Render();
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------------
bool vtkWasmSceneManager::ResetCamera(vtkTypeUInt32 identifier)
{
  auto object = this->GetObjectAtId(identifier);
  if (auto renderer = vtkRenderer::SafeDownCast(object))
  {
    renderer->ResetCamera();
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------------
bool vtkWasmSceneManager::StartEventLoop(vtkTypeUInt32 identifier)
{
  vtkRenderWindowInteractor::InteractorManagesTheEventLoop = false;
  auto object = this->GetObjectAtId(identifier);
  if (auto* renderWindow = vtkRenderWindow::SafeDownCast(object))
  {
    if (auto* interactor =
          vtkWebAssemblyRenderWindowInteractor::SafeDownCast(renderWindow->GetInteractor()))
    {
      if (auto* wasmGLWindow = vtkWebAssemblyOpenGLRenderWindow::SafeDownCast(renderWindow))
      {
        // copy canvas selector from the render window to the interactor.
        interactor->SetCanvasSelector(wasmGLWindow->GetCanvasSelector());
        std::cout << "Started event loop id=" << identifier
                  << ", interactor=" << interactor->GetObjectDescription() << '\n';
        interactor->Start();
        return true;
      }
      else
      {
        std::cerr << "Render window class " << renderWindow->GetClassName()
                  << " is not recognized!\n";
      }
    }
    else
    {
      std::cerr << "Interactor class " << renderWindow->GetClassName() << " is not recognized!\n";
    }
  }
  return false;
}

//-------------------------------------------------------------------------------
bool vtkWasmSceneManager::StopEventLoop(vtkTypeUInt32 identifier)
{
  auto object = this->GetObjectAtId(identifier);
  if (auto renderWindow = vtkRenderWindow::SafeDownCast(object))
  {
    auto interactor = renderWindow->GetInteractor();
    std::cout << "Stopping event loop id=" << identifier
              << ", interactor=" << interactor->GetObjectDescription() << '\n';
    interactor->TerminateApp();
    return true;
  }
  return false;
}

namespace
{
struct CallbackBridge
{
  vtkWasmSceneManager::ObserverCallbackF f;
  vtkTypeUInt32 SenderId;
};
}

//-------------------------------------------------------------------------------
unsigned long vtkWasmSceneManager::AddObserver(
  vtkTypeUInt32 identifier, std::string eventName, ObserverCallbackF callback)
{
  auto object = vtkObject::SafeDownCast(this->GetObjectAtId(identifier));
  if (object == nullptr)
  {
    return 0;
  }
  vtkNew<vtkCallbackCommand> callbackCmd;
  callbackCmd->SetClientData(new CallbackBridge{ callback, identifier });
  callbackCmd->SetClientDataDeleteCallback(
    [](void* clientData)
    {
      auto* bridge = reinterpret_cast<CallbackBridge*>(clientData);
      delete bridge;
    });
  callbackCmd->SetCallback(
    [](vtkObject*, unsigned long eid, void* clientData, void*)
    {
      auto* bridge = reinterpret_cast<CallbackBridge*>(clientData);
      bridge->f(bridge->SenderId, vtkCommand::GetStringFromEventId(eid));
    });
  return object->AddObserver(eventName.c_str(), callbackCmd);
}

//-------------------------------------------------------------------------------
bool vtkWasmSceneManager::RemoveObserver(vtkTypeUInt32 identifier, unsigned long tag)
{

  auto object = vtkObject::SafeDownCast(this->GetObjectAtId(identifier));
  if (object == nullptr)
  {
    return false;
  }
  object->RemoveObserver(tag);
  return true;
}

bool vtkWasmSceneManager::BindRenderWindow(
  vtkTypeUInt32 renderWindowIdentifier, const char* canvasSelector)
{
  if (auto* renderWindow =
        vtkRenderWindow::SafeDownCast(this->GetObjectAtId(renderWindowIdentifier)))
  {
    if (auto* wasmGLWindow = vtkWebAssemblyOpenGLRenderWindow::SafeDownCast(renderWindow))
    {
      wasmGLWindow->SetCanvasSelector(canvasSelector);
      if (auto* interactor =
            vtkWebAssemblyRenderWindowInteractor::SafeDownCast(renderWindow->GetInteractor()))
      {
        interactor->SetCanvasSelector(canvasSelector);
        return true;
      }
      else
      {
        std::cerr << "No interactor found for render window with identifier: "
                  << renderWindowIdentifier << '\n';
        return false;
      }
    }
    else
    {
      std::cerr << "Render window class " << renderWindow->GetClassName()
                << " is not recognized!\n";
      return false;
    }
  }
  else
  {
    std::cerr << "No render window found with identifier: " << renderWindowIdentifier << '\n';
    return false;
  }
}

VTK_ABI_NAMESPACE_END
