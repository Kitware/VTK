// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRemoteSession.h"

#include "vtkLogger.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWebAssemblySessionHelper.h"

#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
#include "vtkWebAssemblyOpenGLRenderWindow.h"
#endif
#if VTK_MODULE_ENABLE_VTK_RenderingUI
#include "vtkWebAssemblyRenderWindowInteractor.h"
#endif

VTK_ABI_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------
vtkRemoteSession::vtkRemoteSession()
{
  this->Session = NewVTKInterfaceForJavaScript();
  vtkSessionInitializeObjectManager(this->Session);
  SetupWASMHandlers(this->Session);
}

//-------------------------------------------------------------------------------
vtkRemoteSession::~vtkRemoteSession()
{
  vtkFreeSession(this->Session);
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::RegisterState(emscripten::val state)
{
  vtkSessionJsonImpl stateImpl{ state };
  return vtkSessionRegisterState(this->Session, &stateImpl) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::UnRegisterState(vtkObjectHandle object)
{
  return vtkSessionUnRegisterState(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
emscripten::val vtkRemoteSession::GetState(vtkObjectHandle object)
{
  auto resultImpl = vtkSessionGetState(this->Session, object);
  auto result = std::move(resultImpl->JsonValue);
  delete resultImpl;
  return result;
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::SkipProperty(const std::string& className, const std::string& propertyName)
{
  return vtkSessionSkipProperty(this->Session, className.c_str(), propertyName.c_str());
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::UnSkipProperty(const std::string& className, const std::string& propertyName)
{
  return vtkSessionUnSkipProperty(this->Session, className.c_str(), propertyName.c_str());
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::RegisterBlob(const std::string& hash, emscripten::val jsArray)
{
  if (jsArray.instanceof (val::global("Uint8Array")))
  {
    const auto length = jsArray["length"].as<std::size_t>();
    std::uint8_t* blob = new std::uint8_t[length];
    val memoryView{ typed_memory_view(length, blob) };
    memoryView.call<void>("set", jsArray);
    return vtkSessionRegisterBlob(this->Session, hash.c_str(), blob, length) ==
      vtkSessionResultSuccess;
  }
  else
  {
    vtkLog(ERROR, << "Invalid type! Expects instanceof blob == Uint8Array");
    return false;
  }
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::UnRegisterBlob(const std::string& hash)
{
  return vtkSessionUnRegisterBlob(this->Session, hash.c_str());
}

//-------------------------------------------------------------------------------
emscripten::val vtkRemoteSession::GetBlob(const std::string& hash)
{
  std::size_t length;
  auto* data = vtkSessionGetBlob(this->Session, hash.c_str(), &length);
  val jsArray = Uint8Array.new_(typed_memory_view(length, data));
  return jsArray;
}

//-------------------------------------------------------------------------------
emscripten::val vtkRemoteSession::Invoke(
  vtkObjectHandle object, const std::string& methodName, emscripten::val args)
{
  vtkSessionJsonImpl argsJsonImpl{ args };
  auto resultImpl = vtkSessionInvoke(this->Session, object, methodName.c_str(), &argsJsonImpl);
  auto result = std::move(resultImpl->JsonValue);
  delete resultImpl;
  return result;
}

//-------------------------------------------------------------------------------
emscripten::val vtkRemoteSession::GetAllDependencies(vtkObjectHandle object)
{
  std::size_t length;
  auto* ids = vtkSessionGetAllDependencies(this->Session, object, &length);
  val jsArray = Uint32Array.new_(typed_memory_view(length, ids));
  vtkSessionFreeGetAllDependenciesResult(this->Session, ids);
  return jsArray;
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::UpdateObjectFromState(emscripten::val state)
{
  vtkSessionJsonImpl stateImpl{ state };
  return vtkSessionUpdateObjectFromState(this->Session, &stateImpl);
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::UpdateStateFromObject(vtkObjectHandle object)
{
  return vtkSessionUpdateStateFromObject(this->Session, object);
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::SetSize(vtkObjectHandle object, int width, int height)
{
  return vtkSessionSetSize(this->Session, object, width, height) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::Render(vtkObjectHandle object)
{
  return vtkSessionRender(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::ResetCamera(vtkObjectHandle object)
{
  return vtkSessionResetCamera(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::StartEventLoop(vtkObjectHandle object)
{
  return vtkSessionStartEventLoop(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::StopEventLoop(vtkObjectHandle object)
{
  return vtkSessionStopEventLoop(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::BindRenderWindow(vtkObjectHandle object, const std::string canvasSelector)
{
  if (auto* manager = GetSessionManager(this->Session))
  {
    if (auto* renderWindow = vtkRenderWindow::SafeDownCast(manager->GetObjectAtId(object)))
    {
      if (auto* wasmGLWindow = vtkWebAssemblyOpenGLRenderWindow::SafeDownCast(renderWindow))
      {
        wasmGLWindow->SetCanvasSelector(canvasSelector.c_str());
        if (auto* interactor =
              vtkWebAssemblyRenderWindowInteractor::SafeDownCast(renderWindow->GetInteractor()))
        {
          interactor->SetCanvasSelector(canvasSelector.c_str());
          return true;
        }
        else
        {
          vtkLog(ERROR, "No interactor attached to " << wasmGLWindow);
          return false;
        }
      }
      else
      {
        vtkLog(
          ERROR, "Render window class " << renderWindow->GetClassName() << " is not recognized!");
        return false;
      }
    }
    else
    {
      vtkLog(ERROR, "No render window found with identifier: " << object);
      return false;
    }
  }
  vtkLog(ERROR, "Invalid session: " << this->Session);
  return false;
}

//-------------------------------------------------------------------------------
unsigned long vtkRemoteSession::Observe(
  vtkObjectHandle object, const std::string& eventName, emscripten::val jsFunction)
{
  int fp = val::module_property("addFunction")(jsFunction, std::string("vii")).as<int>();
  auto callback = reinterpret_cast<vtkSessionObserverCallbackFunc>(fp);
  return vtkSessionAddObserver(this->Session, object, eventName.c_str(), callback);
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::UnObserve(vtkObjectHandle object, unsigned long tag)
{
  return vtkSessionRemoveObserver(this->Session, object, tag) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::Export(const std::string& fileName)
{
  return vtkSessionExport(this->Session, fileName.c_str());
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::Import(const std::string& stateFileName, const std::string& blobFileName)
{
  return vtkSessionImport(this->Session, stateFileName.c_str(), blobFileName.c_str());
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::UpdateObjectsFromStates()
{
  return vtkSessionUpdateObjectsFromStates(this->Session);
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::UpdateStatesFromObjects()
{
  return vtkSessionUpdateStatesFromObjects(this->Session);
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::PruneUnusedBlobs()
{
  return vtkSessionPruneUnusedBlobs(this->Session);
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::PruneUnusedObjects()
{
  return vtkSessionPruneUnusedObjects(this->Session);
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::PruneUnusedStates()
{
  return vtkSessionPruneUnusedStates(this->Session);
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::Clear()
{
  return vtkSessionClear(this->Session);
}

//-------------------------------------------------------------------------------
std::size_t vtkRemoteSession::GetTotalBlobMemoryUsage()
{
  return vtkSessionGetTotalBlobMemoryUsage(this->Session);
}

//-------------------------------------------------------------------------------
std::size_t vtkRemoteSession::GetTotalVTKDataObjectMemoryUsage()
{
  return vtkSessionGetTotalVTKDataObjectMemoryUsage(this->Session);
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::PrintSceneManagerInformation()
{
  return vtkSessionPrintSceneManagerInformation(this->Session);
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::SetDeserializerLogVerbosity(const std::string& verbosityLevel)
{
  return vtkSessionSetDeserializerLogVerbosity(this->Session, verbosityLevel.c_str());
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::SetInvokerLogVerbosity(const std::string& verbosityLevel)
{
  return vtkSessionSetInvokerLogVerbosity(this->Session, verbosityLevel.c_str());
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::SetObjectManagerLogVerbosity(const std::string& verbosityLevel)
{
  return vtkSessionSetObjectManagerLogVerbosity(this->Session, verbosityLevel.c_str());
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::SetSerializerLogVerbosity(const std::string& verbosityLevel)
{
  return vtkSessionSetSerializerLogVerbosity(this->Session, verbosityLevel.c_str());
};

VTK_ABI_NAMESPACE_END
