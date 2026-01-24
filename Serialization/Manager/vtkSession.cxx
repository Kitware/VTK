// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSession.h"

#include "vtkCallbackCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkObjectManager.h"
#include "vtkSmartPointer.h"
#include "vtkTypeUInt8Array.h"

#if VTK_MODULE_ENABLE_VTK_RenderingCore
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#endif

#include <map>
#include <set>
#include <string>

#include <iostream>
#include <sstream>

struct vtkSessionImpl
{
  vtkNew<vtkObjectManager> Manager;
  vtkSessionJsonParseFunc ParseJson;
  vtkSessionJsonStringifyFunc StringifyJson;
  std::map<std::string, std::set<std::string>> SkippedClassProperties;
};

struct CallbackBridge
{
  vtkSessionObserverCallbackFunc F;
  vtkTypeUInt32 SenderId;
};

extern "C"
{
  //-------------------------------------------------------------------------------
  vtkSession vtkCreateSession(const vtkSessionDescriptor* descriptor)
  {
    auto* sessionImpl = new vtkSessionImpl{};
    if (descriptor != nullptr)
    {
      sessionImpl->ParseJson = descriptor->ParseJson;
      sessionImpl->StringifyJson = descriptor->StringifyJson;
#if VTK_MODULE_ENABLE_VTK_RenderingCore
      vtkRenderWindowInteractor::InteractorManagesTheEventLoop =
        descriptor->InteractorManagesTheEventLoop;
#endif
    }
    return sessionImpl;
  }

  //-------------------------------------------------------------------------------
  void vtkFreeSession(vtkSession session)
  {
    delete session;
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionInitializeObjectManager(vtkSession session)
  {
    bool result = session->Manager->Initialize();
    return result ? vtkSessionResultSuccess : vtkSessionResultFailure;
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionInitializeObjectManagerExtensionHandlers(
    vtkSession session, const vtkSessionObjectManagerRegistrarFunc* registrars, size_t count)
  {
    bool result = session->Manager->InitializeExtensionModuleHandlers(registrars, count);
    return result ? vtkSessionResultSuccess : vtkSessionResultFailure;
  }

  //-------------------------------------------------------------------------------
  void* vtkSessionGetManager(vtkSession session)
  {
    return session ? session->Manager.GetPointer() : nullptr;
  }

  //-------------------------------------------------------------------------------
  vtkObjectHandle vtkSessionCreateObject(vtkSession session, const char* className)
  {
    vtkObjectHandle object;
    vtkSmartPointer<vtkObjectBase> objectImpl;
    // Construct the object using the class name
    if (auto constructor = session->Manager->GetDeserializer()->GetConstructor(className, {}))
    {
      objectImpl = vtk::TakeSmartPointer(constructor());
      object = session->Manager->RegisterObject(objectImpl);
      // Insert placeholder state, so that deserializer knows about this object.
      auto stateJson = nlohmann::json::object();
      stateJson["ClassName"] = className;
      stateJson["Id"] = object;
      if (!session->Manager->RegisterState(stateJson))
      {
        vtkLog(
          ERROR, << "Failed to register state for newly created object of class: " << className);
        session->Manager->UnRegisterObject(object);
        object = 0;
      }
    }
    else
    {
      object = 0;
      vtkLog(ERROR, << "Constructor not found for class name: " << className);
    }
    return object;
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionDestroyObject(vtkSession session, vtkObjectHandle object)
  {
    bool result = session->Manager->UnRegisterObject(object);
    result &= session->Manager->UnRegisterState(object);
    return result ? vtkSessionResultSuccess : vtkSessionResultFailure;
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionRegisterState(vtkSession session, vtkSessionJson state)
  {
    char* stateJsonCString = session->StringifyJson(state);
    auto stateJson = nlohmann::json::parse(stateJsonCString, nullptr, false);
    free(stateJsonCString);
    if (stateJson.is_discarded())
    {
      vtkLog(ERROR, << "Failed to parse state!");
      return vtkSessionResultFailure;
    }
    if (auto classNameIter = stateJson.find("ClassName"); classNameIter != stateJson.end())
    {
      if (*classNameIter == "vtkOSOpenGLRenderWindow")
      {
        *classNameIter = "vtkRenderWindow";
      }
      if (auto propertiesIter = session->SkippedClassProperties.find(*classNameIter);
          propertiesIter != session->SkippedClassProperties.end())
      {
        for (const auto& propertyName : propertiesIter->second)
        {
          stateJson.erase(propertyName);
        }
      }
    }
    return session->Manager->RegisterState(stateJson) ? vtkSessionResultSuccess
                                                      : vtkSessionResultFailure;
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionUnRegisterState(vtkSession session, vtkObjectHandle object)
  {
    return session->Manager->UnRegisterState(object) ? vtkSessionResultSuccess
                                                     : vtkSessionResultFailure;
  }

  //-------------------------------------------------------------------------------
  vtkSessionJson vtkSessionGetState(vtkSession session, vtkObjectHandle object)
  {
    auto stateJsonString = session->Manager->GetState(object);
    return session->ParseJson(stateJsonString.c_str());
  }

  //-------------------------------------------------------------------------------
  void vtkSessionSkipProperty(vtkSession session, const char* className, const char* propertyName)
  {
    session->SkippedClassProperties[className].insert(propertyName);
  }

  //-------------------------------------------------------------------------------
  void vtkSessionUnSkipProperty(vtkSession session, const char* className, const char* propertyName)
  {
    if (auto iter = session->SkippedClassProperties.find(className);
        iter != session->SkippedClassProperties.end())
    {
      iter->second.erase(propertyName);
    }
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionRegisterBlob(
    vtkSession session, const char* hash, uint8_t* blob, size_t length)
  {
    auto blobArray = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
    // Takes ownership of the blob.
    blobArray->SetArray(blob, static_cast<vtkIdType>(length), /*save=*/0);
    return session->Manager->RegisterBlob(hash, blobArray) ? vtkSessionResultSuccess
                                                           : vtkSessionResultFailure;
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionUnRegisterBlob(vtkSession session, const char* hash)
  {
    return session->Manager->UnRegisterBlob(hash) ? vtkSessionResultSuccess
                                                  : vtkSessionResultFailure;
  }

  //-------------------------------------------------------------------------------
  uint8_t* vtkSessionGetBlob(vtkSession session, const char* hash, size_t* length)
  {
    auto blobArray = session->Manager->GetBlob(hash);
    *length = static_cast<std::size_t>(blobArray->GetNumberOfValues());
    return blobArray->GetPointer(0);
  }

  //-------------------------------------------------------------------------------
  vtkSessionJson vtkSessionInvoke(
    vtkSession session, vtkObjectHandle object, const char* methodName, vtkSessionJson args)
  {
    if (session->Manager->GetObjectAtId(object) == nullptr)
    {
      vtkLog(ERROR, << "Cannot invoke " << methodName << " on object with ID: " << object
                    << " because the object does not exist.");
      return session->ParseJson("{}");
    }
    char* argsJsonCString = session->StringifyJson(args);
    auto argsJsonString = std::string(argsJsonCString);
    free(argsJsonCString);
    auto resultJsonString = session->Manager->Invoke(object, methodName, argsJsonString);
    return session->ParseJson(resultJsonString.c_str());
  }

  //-------------------------------------------------------------------------------
  uint32_t* vtkSessionGetAllDependencies(vtkSession session, vtkObjectHandle object, size_t* length)
  {
    auto idsVtkArray = session->Manager->GetAllDependenciesAsVTKDataArray(object);
    *length = static_cast<size_t>(idsVtkArray->GetNumberOfValues());
    // release the ownership of the array and return the pointer to the caller.
    idsVtkArray->SetArrayFreeFunction(nullptr);
    return idsVtkArray->GetPointer(0);
  }

  //-------------------------------------------------------------------------------
  void vtkSessionFreeGetAllDependenciesResult(vtkSession vtkNotUsed(session), uint32_t* ptr)
  {
    delete[] ptr;
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionUpdateObjectFromState(vtkSession session, vtkSessionJson state)
  {
    char* stateJsonCString = session->StringifyJson(state);
    auto stateJson = nlohmann::json::parse(stateJsonCString, nullptr, false);
    free(stateJsonCString);
    if (stateJson.is_discarded())
    {
      vtkLog(ERROR, << "Failed to parse state!");
      return vtkSessionResultFailure;
    }
    else if (auto idIter = stateJson.find("Id"); idIter != stateJson.end())
    {
      if (auto classNameIter = stateJson.find("ClassName"); classNameIter != stateJson.end())
      {
        if (*classNameIter == "vtkOSOpenGLRenderWindow")
        {
          *classNameIter = "vtkRenderWindow";
        }
      }
      if (auto objectImpl = session->Manager->GetObjectAtId(*idIter))
      {
        const std::string className = objectImpl->GetClassName();
        if (auto propertiesIter = session->SkippedClassProperties.find(className);
            propertiesIter != session->SkippedClassProperties.end())
        {
          for (const auto& propertyName : propertiesIter->second)
          {
            stateJson.erase(propertyName);
          }
        }
      }
    }
    return session->Manager->UpdateObjectFromState(stateJson) ? vtkSessionResultSuccess
                                                              : vtkSessionResultFailure;
  }

  //-------------------------------------------------------------------------------
  void vtkSessionUpdateStateFromObject(vtkSession session, vtkObjectHandle object)
  {
    session->Manager->UpdateStateFromObject(object);
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionSetSize(
    vtkSession session, vtkObjectHandle object, int width, int height)
  {
    auto objectImpl = session->Manager->GetObjectAtId(object);
#if VTK_MODULE_ENABLE_VTK_RenderingCore
    if (auto* renderWindow = vtkRenderWindow::SafeDownCast(objectImpl))
    {
      if (auto* interactor = renderWindow->GetInteractor())
      {
        interactor->UpdateSize(width, height);
        return vtkSessionResultSuccess;
      }
      else
      {
        vtkLog(ERROR, << objectImpl << " does not have an interactor!");
      }
    }
    else if (objectImpl != nullptr)
    {
      vtkLog(ERROR, << "Object " << objectImpl->GetObjectDescription() << " at id=" << object
                    << " is not a vtkRenderWindow!");
    }
    else
    {
      vtkLog(TRACE, << "Object at id=" << object << " is null");
    }
    return vtkSessionResultFailure;
#else
    (void)width;
    (void)height;
    vtkLog(ERROR, << "VTK_RenderingCore module is not enabled. Cannot set size.");
    return vtkSessionResultFailure;
#endif
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionRender(vtkSession session, vtkObjectHandle object)
  {
    auto objectImpl = session->Manager->GetObjectAtId(object);
#if VTK_MODULE_ENABLE_VTK_RenderingCore
    if (auto* renderWindow = vtkRenderWindow::SafeDownCast(objectImpl))
    {
      renderWindow->Render();
      return vtkSessionResultSuccess;
    }
    else if (objectImpl != nullptr)
    {
      vtkLog(ERROR, << "Object " << objectImpl->GetObjectDescription() << " at id=" << object
                    << " is not a vtkRenderWindow!");
    }
    else
    {
      vtkLog(TRACE, << "Object at id=" << object << " is null");
    }
    return vtkSessionResultFailure;
#else
    (void)objectImpl;
    vtkLog(ERROR, << "VTK_RenderingCore module is not enabled. Cannot render.");
    return vtkSessionResultFailure;
#endif
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionResetCamera(vtkSession session, vtkObjectHandle object)
  {
    auto objectImpl = session->Manager->GetObjectAtId(object);
#if VTK_MODULE_ENABLE_VTK_RenderingCore
    if (auto* renderer = vtkRenderer::SafeDownCast(objectImpl))
    {
      renderer->ResetCamera();
      return vtkSessionResultSuccess;
    }
    else if (objectImpl != nullptr)
    {
      vtkLog(ERROR, << "Object " << objectImpl->GetObjectDescription() << " at id=" << object
                    << " is not a vtkRenderer!");
    }
    else
    {
      vtkLog(TRACE, << "Object at id=" << object << " is null");
    }
    return vtkSessionResultFailure;
#else
    (void)objectImpl;
    vtkLog(ERROR, << "VTK_RenderingCore module is not enabled. Cannot reset camera.");
    return vtkSessionResultFailure;
#endif
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionStartEventLoop(vtkSession session, vtkObjectHandle object)
  {
    auto objectImpl = session->Manager->GetObjectAtId(object);
#if VTK_MODULE_ENABLE_VTK_RenderingCore
    if (auto* renderWindow = vtkRenderWindow::SafeDownCast(objectImpl))
    {
      if (auto* interactor = renderWindow->GetInteractor())
      {
        vtkVLog(session->Manager->GetObjectManagerLogVerbosity(),
          << "Started event loop id=" << object
          << ", interactor=" << interactor->GetObjectDescription());
        interactor->Start();
        return vtkSessionResultSuccess;
      }
      else
      {
        vtkLog(ERROR, << objectImpl << " does not have an interactor!");
      }
    }
    else if (objectImpl != nullptr)
    {
      vtkLog(ERROR, << "Object " << objectImpl->GetObjectDescription() << " at id=" << object
                    << "is not a vtkRenderWindow!");
    }
    else
    {
      vtkLog(TRACE, << "Object at id=" << object << " is null");
    }
    return vtkSessionResultFailure;
#else
    (void)objectImpl;
    vtkLog(ERROR, << "VTK_RenderingCore module is not enabled. Cannot start event loop.");
    return vtkSessionResultFailure;
#endif
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionStopEventLoop(vtkSession session, vtkObjectHandle object)
  {
    auto objectImpl = session->Manager->GetObjectAtId(object);
#if VTK_MODULE_ENABLE_VTK_RenderingCore
    if (auto* renderWindow = vtkRenderWindow::SafeDownCast(objectImpl))
    {
      if (auto* interactor = renderWindow->GetInteractor())
      {
        vtkVLog(session->Manager->GetObjectManagerLogVerbosity(),
          << "Stopping event loop id=" << objectImpl
          << ", interactor=" << interactor->GetObjectDescription());
        interactor->TerminateApp();
        return vtkSessionResultSuccess;
      }
      else
      {
        vtkLog(ERROR, << objectImpl << " does not have an interactor!");
      }
    }
    else if (objectImpl != nullptr)
    {
      vtkLog(ERROR, << "Object " << objectImpl->GetObjectDescription() << " at id=" << object
                    << " is not a vtkRenderWindow!");
    }
    else
    {
      vtkLog(TRACE, << "Object at id=" << object << " is null");
    }
    return vtkSessionResultFailure;
#else
    (void)objectImpl;
    vtkLog(ERROR, << "VTK_RenderingCore module is not enabled. Cannot start event loop.");
    return vtkSessionResultFailure;
#endif
  }

  //-------------------------------------------------------------------------------
  unsigned long vtkSessionAddObserver(vtkSession session, vtkObjectHandle object,
    const char* eventName, vtkSessionObserverCallbackFunc callback)
  {
    auto* objectImpl = vtkObject::SafeDownCast(session->Manager->GetObjectAtId(object));
    if (objectImpl == nullptr)
    {
      return 0;
    }
    vtkNew<vtkCallbackCommand> callbackCmd;
    callbackCmd->SetClientData(new CallbackBridge{ callback, object });
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
        bridge->F(bridge->SenderId, vtkCommand::GetStringFromEventId(eid));
      });
    return objectImpl->AddObserver(eventName, callbackCmd);
  }

  //-------------------------------------------------------------------------------
  vtkSessionResult vtkSessionRemoveObserver(
    vtkSession session, vtkObjectHandle object, unsigned long tag)
  {
    auto* objectImpl = vtkObject::SafeDownCast(session->Manager->GetObjectAtId(object));
    if (objectImpl == nullptr)
    {
      return vtkSessionResultFailure;
    }
    objectImpl->RemoveObserver(tag);
    return vtkSessionResultSuccess;
  }

  //-------------------------------------------------------------------------------
  void vtkSessionExport(vtkSession session, const char* fileName)
  {
    session->Manager->Export(fileName);
  }

  //-------------------------------------------------------------------------------
  void vtkSessionImport(vtkSession session, const char* stateFileName, const char* blobFileName)
  {
    session->Manager->Import(stateFileName, blobFileName);
  }

  //-------------------------------------------------------------------------------
  void vtkSessionUpdateObjectsFromStates(vtkSession session)
  {
    session->Manager->UpdateObjectsFromStates();
  }

  //-------------------------------------------------------------------------------
  void vtkSessionUpdateStatesFromObjects(vtkSession session)
  {
    session->Manager->UpdateStatesFromObjects();
  }

  //-------------------------------------------------------------------------------
  void vtkSessionPruneUnusedBlobs(vtkSession session)
  {
    session->Manager->PruneUnusedBlobs();
  }

  //-------------------------------------------------------------------------------
  void vtkSessionPruneUnusedObjects(vtkSession session)
  {
    session->Manager->PruneUnusedObjects();
  }

  //-------------------------------------------------------------------------------
  void vtkSessionPruneUnusedStates(vtkSession session)
  {
    session->Manager->PruneUnusedStates();
  }

  //-------------------------------------------------------------------------------
  void vtkSessionClear(vtkSession session)
  {
    session->Manager->Clear();
  }

  //-------------------------------------------------------------------------------
  size_t vtkSessionGetTotalBlobMemoryUsage(vtkSession session)
  {
    return session->Manager->GetTotalBlobMemoryUsage();
  }

  //-------------------------------------------------------------------------------
  size_t vtkSessionGetTotalVTKDataObjectMemoryUsage(vtkSession session)
  {
    return session->Manager->GetTotalVTKDataObjectMemoryUsage();
  }

  //-------------------------------------------------------------------------------
  char* vtkSessionPrintObjectToString(vtkSession session, vtkObjectHandle object)
  {
    std::ostringstream oss;
    if (auto objectImpl = session->Manager->GetObjectAtId(object))
    {
      objectImpl->Print(oss);
      auto str = oss.str();
      return strdup(str.c_str());
    }
    else
    {
      return strdup("(null)");
    }
  }

  //-------------------------------------------------------------------------------
  void vtkSessionPrintSceneManagerInformation(vtkSession session)
  {
    session->Manager->Print(std::cout);
  }

  //-------------------------------------------------------------------------------
  void vtkSessionSetDeserializerLogVerbosity(vtkSession session, const char* verbosityStr)
  {
    const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityStr);
    session->Manager->GetDeserializer()->SetDeserializerLogVerbosity(verbosity);
  }

  //-------------------------------------------------------------------------------
  void vtkSessionSetInvokerLogVerbosity(vtkSession session, const char* verbosityStr)
  {
    const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityStr);
    session->Manager->GetInvoker()->SetInvokerLogVerbosity(verbosity);
  }

  //-------------------------------------------------------------------------------
  void vtkSessionSetObjectManagerLogVerbosity(vtkSession session, const char* verbosityStr)
  {
    const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityStr);
    session->Manager->SetObjectManagerLogVerbosity(verbosity);
  }

  //-------------------------------------------------------------------------------
  void vtkSessionSetSerializerLogVerbosity(vtkSession session, const char* verbosityStr)
  {
    const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityStr);
    session->Manager->GetSerializer()->SetSerializerLogVerbosity(verbosity);
  }
}
