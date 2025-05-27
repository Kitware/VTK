// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_5_0() warnings.
#define VTK_DEPRECATION_LEVEL 0

// Init factories.
#include "vtkLogger.h"
#if VTK_MODULE_ENABLE_VTK_RenderingContextOpenGL2
#include "vtkRenderingContextOpenGL2Module.h"
#endif
#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
#include "vtkRenderingOpenGL2Module.h"
#endif
#if VTK_MODULE_ENABLE_VTK_RenderingUI
#include "vtkRenderingUIModule.h"
#endif
#if VTK_MODULE_ENABLE_VTK_RenderingVolumeOpenGL2
#include "vtkRenderingVolumeOpenGL2Module.h"
#endif
#if VTK_MODULE_ENABLE_VTK_RenderingWebGPU
#include "vtkRenderingWebGPUModule.h"
#include "vtkRenderingWebGPUSerDes.h"
#endif

#include <emscripten.h>
#include <emscripten/bind.h>

#include "vtkABINamespace.h"
#include "vtkObjectManager.h"
#include "vtkRemoteSession.h"
#include "vtkSession.h"
#include "vtkStandaloneSession.h"
#include "vtkVersion.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace emscripten;

namespace
{
/**
 * This function configures the session to use WebAssembly-specific handlers
 * for serialization and deserialization. It removes the default
 * vtkOpenGLPolyDataMapper handler, as it is not used in the WebAssembly build.
 *
 * @param session The vtkSession for which to set up WebAssembly handlers.
 */
void PatchOpenGLDeserializer(vtkSession session)
{
  if (auto* manager = static_cast<vtkObjectManager*>(vtkSessionGetManager(session)))
  {
#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
    // Remove the default vtkOpenGLPolyDataMapper[2D] constructors as they are not used in wasm
    manager->GetDeserializer()->UnRegisterConstructor("vtkOpenGLPolyDataMapper");
    manager->GetDeserializer()->UnRegisterConstructor("vtkOpenGLPolyDataMapper2D");
#else
    (void)manager;
#endif
  }
}

void InitWebGPURegistrars(vtkSession session)
{
#if VTK_MODULE_ENABLE_VTK_RenderingWebGPU
  vtkSessionObjectManagerRegistrarFunc registrars[1] = { RegisterClasses_vtkRenderingWebGPU };
  if (vtkSessionInitializeObjectManagerExtensionHandlers(session, registrars, 1) ==
    vtkSessionResultFailure)
  {
    vtkLogF(ERROR, "Failed to register rendering webGPU classes");
  }
#else
  (void)session;
#endif
}

vtkStandaloneSession* makeStandaloneSession()
{
  auto* standaloneSession = new vtkStandaloneSession();
  InitWebGPURegistrars(standaloneSession->Session);
  PatchOpenGLDeserializer(standaloneSession->Session);
  return standaloneSession;
}

vtkRemoteSession* makeRemoteSession()
{
  auto* remoteSession = new vtkRemoteSession();
  InitWebGPURegistrars(remoteSession->Session);
  PatchOpenGLDeserializer(remoteSession->Session);
  return remoteSession;
}
}

/// Javascript bindings to the webassembly sessions.
EMSCRIPTEN_BINDINGS(vtkWebAssemblyAsyncBindings)
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

  /**
   * Determine if the session provides async wrappers.
   */
  function("isAsync", optional_override([] { return true; }));

  /// Wrappings for the WebAssembly standalone session.
  class_<vtkStandaloneSession>("vtkStandaloneSession")
    .constructor(&makeStandaloneSession, allow_raw_pointers())
    .function("create", &vtkStandaloneSession::Create)
    .function("destroy", &vtkStandaloneSession::Destroy)
    .function("set", &vtkStandaloneSession::Set)
    .function("get", &vtkStandaloneSession::Get)
    .function("invoke", &vtkStandaloneSession::Invoke, async())
    .function("observe", &vtkStandaloneSession::Observe)
    .function("unObserve", &vtkStandaloneSession::UnObserve);

  /// Wrappings for the WebAssembly remoting session.
  class_<vtkRemoteSession>("vtkRemoteSession")
    .constructor(&makeRemoteSession, allow_raw_pointers())
    .function("registerState", &vtkRemoteSession::RegisterState)
    .function("unRegisterState", &vtkRemoteSession::UnRegisterState)
    .function("getState", &vtkRemoteSession::GetState)
    .function("set", &vtkRemoteSession::Set)
    .function("get", &vtkRemoteSession::Get)
    .function("skipProperty", &vtkRemoteSession::SkipProperty)
    .function("unSkipProperty", &vtkRemoteSession::UnSkipProperty)
    .function("registerBlob", &vtkRemoteSession::RegisterBlob)
    .function("unRegisterBlob", &vtkRemoteSession::UnRegisterBlob)
    .function("getBlob", &vtkRemoteSession::GetBlob)
    .function("invoke", &vtkRemoteSession::Invoke, async())
    .function("getAllDependencies", &vtkRemoteSession::GetAllDependencies)
    .function("updateObjectFromState", &vtkRemoteSession::UpdateObjectFromState)
    .function("updateStateFromObject", &vtkRemoteSession::UpdateStateFromObject)
    .function("setSize", &vtkRemoteSession::SetSize)
    .function("render", &vtkRemoteSession::Render, async())
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
