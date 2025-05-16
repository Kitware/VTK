// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebAssemblySessionHelper_h
#define vtkWebAssemblySessionHelper_h

#include <emscripten/val.h>

#include "vtkABINamespace.h"
#include "vtkObjectManager.h"
#include "vtkType.h"

// Initialize object factories.
#if VTK_MODULE_ENABLE_VTK_RenderingContextOpenGL2
#include "vtkRenderingContextOpenGL2Module.h"
#endif
#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
#include "vtkOpenGLPolyDataMapper.h" // needed to remove unused mapper, also includes vtkRenderingOpenGL2Module.h
#endif
#if VTK_MODULE_ENABLE_VTK_RenderingUI
#include "vtkRenderingUIModule.h"
#endif
#if VTK_MODULE_ENABLE_VTK_RenderingVolumeOpenGL2
#include "vtkRenderingVolumeOpenGL2Module.h"
#endif
VTK_ABI_NAMESPACE_BEGIN

// Implement vtkSessionJsonImpl as a wrapper around emscripten::val
struct vtkSessionJsonImpl
{
  emscripten::val JsonValue;
};

namespace
{

using namespace emscripten;

// Initialize JavaScript global objects.
thread_local const val Uint8Array = val::global("Uint8Array");
thread_local const val Uint32Array = val::global("Uint32Array");
thread_local const val JSON = val::global("JSON");
thread_local const val Array = val::global("Array");

// clang-format off
std::map<std::string, std::function<bool(const vtkDataArray*)>> IsJSArraySameTypeAsVtkDataArray = {
  {"Uint8Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_UINT8; }},
  {"Uint8ClampedArray", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_UINT8; }},
  {"Uint16Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_UINT16; }},
  {"Uint32Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_UINT32; }},
  {"Int8Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_INT8; }},
  {"Int16Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_INT16; }},
  {"Float32Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_FLOAT32; }},
  {"Float64Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_FLOAT64; }},
  {"Int32Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_INT32; }},
  {"BigInt64Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_INT64; }},
  {"BigUint64Array", [](const vtkDataArray* dataArray) -> bool { return dataArray->GetDataType() == VTK_TYPE_UINT64; }},
};
// clang-format on
/**
 * Creates a new VTK interface for JavaScript.
 *
 * This function initializes a new VTK session interface with the provided
 * descriptor. It sets up JSON parsing and stringification functions for the
 * session, allowing for seamless communication between C++ and JavaScript.
 *
 * @return A pointer to the newly created vtkSession.
 */
vtkSession NewVTKInterfaceForJavaScript()
{
  vtkSessionDescriptor descriptor;
  descriptor.StringifyJson = +[](vtkSessionJson inputJson) -> char*
  {
    const auto jsonString = JSON.call<val>("stringify", inputJson->JsonValue).as<std::string>();
    const auto length = jsonString.length();
    char* result = new char[length + 1];
    snprintf(result, length + 1, "%s", jsonString.c_str());
    return result;
  };
  descriptor.ParseJson = +[](const char* inputString) -> vtkSessionJson
  {
    vtkSessionJsonImpl* result = new vtkSessionJsonImpl{};
    result->JsonValue = JSON.call<val>("parse", std::string(inputString));
    return result;
  };
  descriptor.InteractorManagesTheEventLoop = false;
  return vtkCreateSession(&descriptor);
}

/**
 * Retrieves the session manager associated with a given session.
 *
 * This function attempts to obtain the session manager for the specified
 * vtkSession. If the session manager exists and can be cast to a
 * vtkObjectManager, it is returned. Otherwise, the function returns nullptr.
 *
 * @param session The vtkSession for which the session manager is to be retrieved.
 * @return A pointer to the vtkObjectManager if successful, or nullptr if no
 *         valid session manager is found.
 */
vtkObjectManager* GetSessionManager(vtkSession session)
{
  if (auto* manager = static_cast<vtkObjectManager*>(vtkSessionGetManager(session)))
  {
    return manager;
  }
  else
  {
    return nullptr;
  }
}

/**
 * Sets up WebAssembly-specific handlers for the given session.
 *
 * This function configures the session to use WebAssembly-specific handlers
 * for serialization and deserialization. It removes the default
 * vtkOpenGLPolyDataMapper handler, as it is not used in the WebAssembly build.
 *
 * @param session The vtkSession for which to set up WebAssembly handlers.
 */
void SetupWASMHandlers(vtkSession session)
{
  if (auto* manager = GetSessionManager(session))
  {
#ifdef VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
    // Remove the default vtkOpenGLPolyDataMapper as it is not used with wasm build.
    /// get rid of serialization handler
    manager->GetSerializer()->UnRegisterHandler(typeid(vtkOpenGLPolyDataMapper));
    /// get rid of de-serialization handler
    manager->GetDeserializer()->UnRegisterHandler(typeid(vtkOpenGLPolyDataMapper));
    /// get rid of constructor
    manager->GetDeserializer()->UnRegisterConstructor("vtkOpenGLPolyDataMapper");
#else
    (void)manager;
#endif
  }
}
}
VTK_ABI_NAMESPACE_END

#endif
