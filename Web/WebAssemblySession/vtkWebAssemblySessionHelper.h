// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebAssemblySessionHelper_h
#define vtkWebAssemblySessionHelper_h

#include <emscripten/val.h>

#include "vtkABINamespace.h"
#include "vtkObjectManager.h"
#include "vtkSession.h"

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

}
VTK_ABI_NAMESPACE_END

#endif
