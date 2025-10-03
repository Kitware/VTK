// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebAssemblySessionHelper_h
#define vtkWebAssemblySessionHelper_h

#include <emscripten/val.h>

#include "vtkABINamespace.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
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

struct CopyJSArrayToVTKDataArray
{
  template <typename ArrayT>
  void operator()(ArrayT* dataArray, const emscripten::val& jsArray)
  {
    using ValueType = vtk::GetAPIType<ArrayT>;
    const auto length = jsArray["length"].as<std::size_t>();
    auto pointer = reinterpret_cast<ValueType*>(dataArray->GetVoidPointer(0));
    auto memoryView = emscripten::val{ typed_memory_view(length, pointer) };
    memoryView.call<void>("set", jsArray);
  }
};
}
VTK_ABI_NAMESPACE_END

#endif
