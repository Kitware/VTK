// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <emscripten.h>
#include <emscripten/bind.h>

#include "vtkDataArrayRange.h"
#include "vtkTypeUInt8Array.h"
#include "vtkVersion.h"
#include "vtkWasmSceneManager.h"

namespace
{

#define CHECK_INIT                                                                                 \
  do                                                                                               \
  {                                                                                                \
    if (Manager == nullptr)                                                                        \
    {                                                                                              \
      std::cerr << "Manager is null. Did you call forget to call initialize()?\n";                 \
    }                                                                                              \
  } while (0)

vtkWasmSceneManager* Manager = nullptr;

using namespace emscripten;

thread_local const val Uint8Array = val::global("Uint8Array");
thread_local const val Uint32Array = val::global("Uint32Array");
thread_local const val JSON = val::global("JSON");

//-------------------------------------------------------------------------------
bool initialize()
{
  Manager = vtkWasmSceneManager::New();
  return Manager->Initialize();
}

//-------------------------------------------------------------------------------
void finalize()
{
  CHECK_INIT;
  Manager->UnRegister(nullptr);
}

//-------------------------------------------------------------------------------
bool registerState(const std::string& state)
{
  CHECK_INIT;
  return Manager->RegisterState(state);
}

//-------------------------------------------------------------------------------
bool unRegisterState(vtkTypeUInt32 identifier)
{
  CHECK_INIT;
  return Manager->UnRegisterState(identifier);
}

//-------------------------------------------------------------------------------
val getState(vtkTypeUInt32 identifier)
{
  CHECK_INIT;
  return JSON.call<val>("parse", Manager->GetState(identifier));
}

//-------------------------------------------------------------------------------
bool unRegisterObject(vtkTypeUInt32 identifier)
{
  CHECK_INIT;
  return Manager->UnRegisterObject(identifier);
}

//-------------------------------------------------------------------------------
bool registerBlob(const std::string& hash, val jsArray)
{
  CHECK_INIT;
  if (jsArray.instanceof (val::global("Uint8Array")))
  {
    const vtkIdType l = jsArray["length"].as<vtkIdType>();
    auto blob = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
    blob->SetNumberOfValues(l);
    auto blobRange = vtk::DataArrayValueRange(blob);
    val memoryView{ typed_memory_view(static_cast<std::size_t>(l), blobRange.data()) };
    memoryView.call<void>("set", jsArray);
    return Manager->RegisterBlob(hash, blob);
  }
  else
  {
    std::cerr << "Invalid type! Expects instanceof blob == Uint8Array" << std::endl;
    return false;
  }
}

//-------------------------------------------------------------------------------
bool unRegisterBlob(const std::string& hash)
{
  CHECK_INIT;
  return Manager->UnRegisterBlob(hash);
}

//-------------------------------------------------------------------------------
val getBlob(const std::string& hash)
{
  CHECK_INIT;
  const auto blob = Manager->GetBlob(hash);
  val jsBlob = Uint8Array.new_(typed_memory_view(blob->GetNumberOfValues(), blob->GetPointer(0)));
  return jsBlob;
}

//-------------------------------------------------------------------------------
void pruneUnusedBlobs()
{
  CHECK_INIT;
  Manager->PruneUnusedBlobs();
}

//-------------------------------------------------------------------------------
void clear()
{
  CHECK_INIT;
  Manager->Clear();
}

//-------------------------------------------------------------------------------
val getAllDependencies(vtkTypeUInt32 identifier)
{
  CHECK_INIT;
  const auto ids = Manager->GetAllDependencies(identifier);
  val jsIds = Uint32Array.new_(typed_memory_view(ids.size(), ids.data()));
  return jsIds;
}
//-------------------------------------------------------------------------------
std::size_t getTotalBlobMemoryUsage()
{
  CHECK_INIT;
  return ::Manager->GetTotalBlobMemoryUsage();
}

//-------------------------------------------------------------------------------
std::size_t getTotalVTKDataObjectMemoryUsage()
{
  CHECK_INIT;
  return ::Manager->GetTotalVTKDataObjectMemoryUsage();
}

//-------------------------------------------------------------------------------
void updateObjectsFromStates()
{
  CHECK_INIT;
  Manager->UpdateObjectsFromStates();
}

//-------------------------------------------------------------------------------
void updateStatesFromObjects()
{
  CHECK_INIT;
  Manager->UpdateStatesFromObjects();
}

//-------------------------------------------------------------------------------
bool setSize(vtkTypeUInt32 identifier, int width, int height)
{
  CHECK_INIT;
  return Manager->SetSize(identifier, width, height);
}

//-------------------------------------------------------------------------------
bool render(vtkTypeUInt32 identifier)
{
  CHECK_INIT;
  return Manager->Render(identifier);
}

//-------------------------------------------------------------------------------
bool resetCamera(vtkTypeUInt32 identifier)
{
  CHECK_INIT;
  return Manager->ResetCamera(identifier);
}

//-------------------------------------------------------------------------------
bool startEventLoop(vtkTypeUInt32 identifier)
{
  CHECK_INIT;
  return Manager->StartEventLoop(identifier);
}

//-------------------------------------------------------------------------------
bool stopEventLoop(vtkTypeUInt32 identifier)
{
  CHECK_INIT;
  return Manager->StopEventLoop(identifier);
}

//-------------------------------------------------------------------------------
unsigned long addObserver(vtkTypeUInt32 identifier, std::string eventName, val jsFunc)
{
  CHECK_INIT;
  int fp = val::module_property("addFunction")(jsFunc, std::string("vii")).as<int>();
  auto callback = reinterpret_cast<vtkWasmSceneManager::ObserverCallbackF>(fp);
  return Manager->AddObserver(identifier, eventName, callback);
}

//-------------------------------------------------------------------------------
bool removeObserver(vtkTypeUInt32 identifier, unsigned long tag)
{
  CHECK_INIT;
  return Manager->RemoveObserver(identifier, tag);
}

//-------------------------------------------------------------------------------
std::string getVTKVersion()
{
  return vtkVersion::GetVTKVersion();
}

//-------------------------------------------------------------------------------
std::string getVTKVersionFull()
{
  return vtkVersion::GetVTKVersionFull();
}

} // namespace

EMSCRIPTEN_BINDINGS(vtkWasmSceneManager)
{
  function("initialize", ::initialize);
  function("finalize", ::finalize);

  function("registerState", ::registerState);
  function("unRegisterState", ::unRegisterState);
  function("getState", ::getState);

  function("unRegisterObject", ::unRegisterObject);

  function("registerBlob", ::registerBlob);
  function("unRegisterBlob", ::unRegisterBlob);
  function("getBlob", ::getBlob);
  function("pruneUnusedBlobs", ::pruneUnusedBlobs);

  function("clear", ::clear);

  function("getAllDependencies", ::getAllDependencies);

  function("getTotalBlobMemoryUsage", ::getTotalBlobMemoryUsage);
  function("getTotalVTKDataObjectMemoryUsage", ::getTotalVTKDataObjectMemoryUsage);

  function("updateObjectsFromStates", ::updateObjectsFromStates);
  function("updateStatesFromObjects", ::updateStatesFromObjects);

  function("setSize", ::setSize);
  function("render", ::render);
  function("resetCamera", ::resetCamera);

  function("startEventLoop", ::startEventLoop);
  function("stopEventLoop", ::stopEventLoop);

  function("addObserver", ::addObserver);
  function("removeObserver", ::removeObserver);

  function("getVTKVersion", ::getVTKVersion);
  function("getVTKVersionFull", ::getVTKVersionFull);
}

int main()
{
  return 0;
}
