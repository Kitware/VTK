// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRemoteSession.h"

#include "vtkLogger.h"
#include "vtkWebAssemblySessionHelper.h"
#include <vtk_nlohmannjson.h>

VTK_ABI_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------
vtkRemoteSession::vtkRemoteSession()
{
  this->Session = NewVTKInterfaceForJavaScript();
  vtkSessionInitializeObjectManager(this->Session);
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
bool vtkRemoteSession::UnRegisterState(vtkTypeUInt32 object)
{
  return vtkSessionUnRegisterState(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::Set(vtkTypeUInt32 object, emscripten::val properties)
{
  // Ensure the ID is set in the JSON state before updating the object
  properties.set("Id", object);
  vtkSessionJsonImpl propertiesImpl{ properties };
  return vtkSessionUpdateObjectFromState(this->Session, &propertiesImpl) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
emscripten::val vtkRemoteSession::Get(vtkTypeUInt32 object)
{

  vtkSessionUpdateStateFromObject(this->Session, object);
  auto propertiesImpl = vtkSessionGetState(this->Session, object);
  auto result = std::move(propertiesImpl->JsonValue);
  delete propertiesImpl;
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
  vtkTypeUInt32 object, const std::string& methodName, emscripten::val args)
{
  if (!args.instanceof (Array))
  {
    vtkLog(
      ERROR, << "Invoke must be called with an objectId: u32, methodName: string, args: Array");
    return emscripten::val::undefined();
  }
  if (auto* manager = static_cast<vtkObjectManager*>(vtkSessionGetManager(this->Session)))
  {
    if (auto* dataArray = vtkDataArray::SafeDownCast(manager->GetObjectAtId(object)))
    {
      if (methodName == "SetArray")
      {
        if (args["length"].as<std::size_t>() == 1)
        {
          auto jsArray = args[0];
          for (const auto& it : IsJSArraySameTypeAsVtkDataArray)
          {
            const auto& typeName = it.first;
            if (jsArray.instanceof (val::global(typeName.c_str())) && it.second(dataArray))
            {
              const auto length = jsArray["length"].as<std::size_t>();
              dataArray->SetNumberOfValues(length);
              // Copy the data from the JS array to the VTK data array
              using DispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
              if (!DispatchT::Execute(dataArray, CopyJSArrayToVTKDataArray{}, jsArray))
              {
                // Fallback to the default implementation if the DispatchT fails
                CopyJSArrayToVTKDataArray{}(dataArray, jsArray);
              }
              return emscripten::val::undefined();
            }
          }
          vtkLog(ERROR,
            "Unsupported argument constructed by "
              << jsArray["constructor"].call<std::string>("toString") << " for "
              << dataArray->GetClassName() << "::SetArray"
              << " method.");
          return emscripten::val::undefined();
        }
        else
        {
          vtkLog(ERROR, << "vtkDataArray::SetArray expects a list of a single TypedArray");
          return emscripten::val::undefined();
        }
      }
    }
  }
  else
  {
    vtkLog(ERROR, "Invalid session: " << this->Session);
    return emscripten::val::undefined();
  }
  vtkSessionJsonImpl argsJsonImpl{ args };
  auto resultImpl = vtkSessionInvoke(this->Session, object, methodName.c_str(), &argsJsonImpl);
  auto result = std::move(resultImpl->JsonValue);
  delete resultImpl;
  return result;
}

//-------------------------------------------------------------------------------
emscripten::val vtkRemoteSession::GetAllDependencies(vtkTypeUInt32 object)
{
  std::size_t length;
  auto* ids = vtkSessionGetAllDependencies(this->Session, object, &length);
  val jsArray = Uint32Array.new_(typed_memory_view(length, ids));
  vtkSessionFreeGetAllDependenciesResult(this->Session, ids);
  return jsArray;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::UpdateObjectFromState(emscripten::val state)
{
  vtkSessionJsonImpl stateImpl{ state };
  return vtkSessionUpdateObjectFromState(this->Session, &stateImpl) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
void vtkRemoteSession::UpdateStateFromObject(vtkTypeUInt32 object)
{
  return vtkSessionUpdateStateFromObject(this->Session, object);
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::SetSize(vtkTypeUInt32 object, int width, int height)
{
  return vtkSessionSetSize(this->Session, object, width, height) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::Render(vtkTypeUInt32 object)
{
  return vtkSessionRender(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::ResetCamera(vtkTypeUInt32 object)
{
  return vtkSessionResetCamera(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::StartEventLoop(vtkTypeUInt32 object)
{
  return vtkSessionStartEventLoop(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::StopEventLoop(vtkTypeUInt32 object)
{
  return vtkSessionStopEventLoop(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::BindRenderWindow(vtkTypeUInt32 object, const std::string canvasSelector)
{
  if (auto* manager = static_cast<vtkObjectManager*>(vtkSessionGetManager(this->Session)))
  {
    if (auto renderWindowObject = manager->GetObjectAtId(object))
    {
      (void)renderWindowObject;
      // Update the canvas selector in the render window.
      manager->UpdateObjectFromState({ { "Id", object }, { "CanvasSelector", canvasSelector } });
      // Get the interactor associated with the render window
      // and set the canvas selector on it.
      const auto& renderWindowState = manager->GetDeserializer()->GetContext()->GetState(object);
      if (auto interactorStateIter = renderWindowState.find("Interactor");
          interactorStateIter != renderWindowState.end())
      {
        if (auto interactorIdIter = interactorStateIter->find("Id");
            interactorIdIter != interactorStateIter->end())
        {
          // Update the interactor state with the canvas selector
          const auto interactorId = interactorIdIter->get<vtkTypeUInt32>();
          manager->UpdateObjectFromState(
            { { "Id", interactorId }, { "CanvasSelector", canvasSelector } });
          return true;
        }
      }
      vtkLog(ERROR, "Failed to get interactor for render window: " << object);
      return false;
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
  vtkTypeUInt32 object, const std::string& eventName, emscripten::val jsFunction)
{
  int fp = val::module_property("addFunction")(jsFunction, std::string("vii")).as<int>();
  auto callback = reinterpret_cast<vtkSessionObserverCallbackFunc>(fp);
  return vtkSessionAddObserver(this->Session, object, eventName.c_str(), callback);
}

//-------------------------------------------------------------------------------
bool vtkRemoteSession::UnObserve(vtkTypeUInt32 object, unsigned long tag)
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
