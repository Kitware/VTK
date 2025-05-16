// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <emscripten.h>
#include <emscripten/bind.h>

#include "vtkABINamespace.h"
#include "vtkRemoteSession.h"
#include "vtkStandaloneSession.h"
#include "vtkVersion.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace emscripten;

/// Javascript bindings to the webassembly sessions.
EMSCRIPTEN_BINDINGS(vtkWebAssemblyInterfaceJavaScript)
{
  /**
   * Get the VTK version string.
   */
  function(
    "getVTKVersion", optional_override([] { return std::string(vtkVersion::GetVTKVersion()); }));
  /**
   * Get the full VTK version string.
   */
  function("getVTKVersionFull",
    optional_override([] { return std::string(vtkVersion::GetVTKVersionFull()); }));

  /// Wrappings for the WebAssembly standalone session.
  class_<vtkStandaloneSession>("vtkStandaloneSession")
    .constructor()
    .function("create", &vtkStandaloneSession::Create)
    .function("destroy", &vtkStandaloneSession::Destroy)
    .function("set", &vtkStandaloneSession::Set)
    .function("get", &vtkStandaloneSession::Get)
    .function("invoke", &vtkStandaloneSession::Invoke)
    .function("observe", &vtkStandaloneSession::Observe)
    .function("unObserve", &vtkStandaloneSession::UnObserve);

  /// Wrappings for the WebAssembly remoting session.
  class_<vtkRemoteSession>("vtkRemoteSession")
    .constructor()
    .function("registerState", &vtkRemoteSession::RegisterState)
    .function("unRegisterState", &vtkRemoteSession::UnRegisterState)
    .function("getState", &vtkRemoteSession::GetState)
    .function("skipProperty", &vtkRemoteSession::SkipProperty)
    .function("unSkipProperty", &vtkRemoteSession::UnSkipProperty)
    .function("registerBlob", &vtkRemoteSession::RegisterBlob)
    .function("unRegisterBlob", &vtkRemoteSession::UnRegisterBlob)
    .function("getBlob", &vtkRemoteSession::GetBlob)
    .function("invoke", &vtkRemoteSession::Invoke)
    .function("getAllDependencies", &vtkRemoteSession::GetAllDependencies)
    .function("updateObjectFromState", &vtkRemoteSession::UpdateObjectFromState)
    .function("updateStateFromObject", &vtkRemoteSession::UpdateStateFromObject)
    .function("setSize", &vtkRemoteSession::SetSize)
    .function("render", &vtkRemoteSession::Render)
    .function("resetCamera", &vtkRemoteSession::ResetCamera)
    .function("startEventLoop", &vtkRemoteSession::StartEventLoop)
    .function("stopEventLoop", &vtkRemoteSession::StopEventLoop)
    .function("bindRenderWindow", &vtkRemoteSession::BindRenderWindow)
    .function("observe", &vtkRemoteSession::Observe)
    .function("unObserve", &vtkRemoteSession::UnObserve)
    .function("export", &vtkRemoteSession::Export)
    .function("import", &vtkRemoteSession::Import)
    .function("updateObjectsFromStates", &vtkRemoteSession::UpdateObjectsFromStates)
    .function("updateStatesFromObjects", &vtkRemoteSession::UpdateStatesFromObjects)
    .function("pruneUnusedBlobs", &vtkRemoteSession::PruneUnusedBlobs)
    .function("clear", &vtkRemoteSession::Clear)
    .function("getTotalBlobMemoryUsage", &vtkRemoteSession::GetTotalBlobMemoryUsage)
    .function(
      "getTotalVTKDataObjectMemoryUsage", &vtkRemoteSession::GetTotalVTKDataObjectMemoryUsage)
    .function("printSceneManagerInformation", &vtkRemoteSession::PrintSceneManagerInformation)
    .function("setDeserializerLogVerbosity", &vtkRemoteSession::SetDeserializerLogVerbosity)
    .function("setInvokerLogVerbosity", &vtkRemoteSession::SetInvokerLogVerbosity)
    .function("setObjectManagerLogVerbosity", &vtkRemoteSession::SetObjectManagerLogVerbosity)
    .function("setSerializerLogVerbosity", &vtkRemoteSession::SetSerializerLogVerbosity);
}

VTK_ABI_NAMESPACE_END
