// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStandaloneSession.h"
#include "vtkLogger.h"
#include "vtkWebAssemblySessionHelper.h"

#include <cstdint>

VTK_ABI_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------
vtkStandaloneSession::vtkStandaloneSession()
{
  this->Session = NewVTKInterfaceForJavaScript();
  vtkSessionInitializeObjectManager(this->Session);
}

//-------------------------------------------------------------------------------
vtkStandaloneSession::~vtkStandaloneSession()
{
  vtkFreeSession(this->Session);
}

//-------------------------------------------------------------------------------
vtkObjectHandle vtkStandaloneSession::Create(const std::string& className)
{
  return vtkSessionCreateObject(this->Session, className.c_str());
}

//-------------------------------------------------------------------------------
bool vtkStandaloneSession::Destroy(vtkObjectHandle object)
{
  return vtkSessionDestroyObject(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkStandaloneSession::Set(vtkObjectHandle object, emscripten::val properties)
{
  // Ensure the ID is set in the JSON state before updating the object
  properties.set("Id", object);
  vtkSessionJsonImpl propertiesImpl{ properties };
  return vtkSessionUpdateObjectFromState(this->Session, &propertiesImpl) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
emscripten::val vtkStandaloneSession::Get(vtkObjectHandle object)
{
  vtkSessionUpdateStateFromObject(this->Session, object);
  auto propertiesImpl = vtkSessionGetState(this->Session, object);
  auto result = std::move(propertiesImpl->JsonValue);
  delete propertiesImpl;
  return result;
}

//-------------------------------------------------------------------------------
emscripten::val vtkStandaloneSession::Invoke(
  vtkObjectHandle object, const std::string& methodName, emscripten::val args)
{
  if (!args.instanceof (Array))
  {
    vtkLog(
      ERROR, << "Invoke must be called with an objectId: u32, methodName: string, args: Array");
    return emscripten::val::undefined();
  }
  vtkSessionJsonImpl argsJsonImpl{ args };
  auto resultImpl = vtkSessionInvoke(this->Session, object, methodName.c_str(), &argsJsonImpl);
  auto result = std::move(resultImpl->JsonValue);
  delete resultImpl;
  return result;
}

//-------------------------------------------------------------------------------
unsigned long vtkStandaloneSession::Observe(
  vtkObjectHandle object, const std::string& eventName, emscripten::val jsFunction)
{
  auto fp =
    val::module_property("addFunction")(jsFunction, std::string("vip")).as<std::uintptr_t>();
  auto callback = reinterpret_cast<vtkSessionObserverCallbackFunc>(fp);
  return vtkSessionAddObserver(this->Session, object, eventName.c_str(), callback);
}

//-------------------------------------------------------------------------------
bool vtkStandaloneSession::UnObserve(vtkObjectHandle object, unsigned long tag)
{
  return vtkSessionRemoveObserver(this->Session, object, tag) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
bool vtkStandaloneSession::UnObserveAll(vtkObjectHandle object)
{
  return vtkSessionRemoveAllObservers(this->Session, object) == vtkSessionResultSuccess;
}

//-------------------------------------------------------------------------------
void vtkStandaloneSession::UnObserveAllObjects()
{
  vtkSessionRemoveAllObserversFromAllObjects(this->Session);
}

//-------------------------------------------------------------------------------
std::size_t vtkStandaloneSession::GetTotalBlobMemoryUsage()
{
  return vtkSessionGetTotalBlobMemoryUsage(this->Session);
}

//-------------------------------------------------------------------------------
std::size_t vtkStandaloneSession::GetTotalVTKDataObjectMemoryUsage()
{
  return vtkSessionGetTotalVTKDataObjectMemoryUsage(this->Session);
}

//-------------------------------------------------------------------------------
std::string vtkStandaloneSession::PrintObjectToString(vtkObjectHandle object)
{
  char* cstr = vtkSessionPrintObjectToString(this->Session, object);
  if (cstr != nullptr)
  {
    std::string result(cstr);
    free(cstr);
    return result;
  }
  else
  {
    vtkLog(ERROR, << "Failed to print object with ID: " << object);
    return {};
  }
}

//-------------------------------------------------------------------------------
void vtkStandaloneSession::PrintSceneManagerInformation()
{
  return vtkSessionPrintSceneManagerInformation(this->Session);
}

//-------------------------------------------------------------------------------
void vtkStandaloneSession::SetDeserializerLogVerbosity(const std::string& verbosityLevel)
{
  return vtkSessionSetDeserializerLogVerbosity(this->Session, verbosityLevel.c_str());
}

//-------------------------------------------------------------------------------
void vtkStandaloneSession::SetInvokerLogVerbosity(const std::string& verbosityLevel)
{
  return vtkSessionSetInvokerLogVerbosity(this->Session, verbosityLevel.c_str());
}

//-------------------------------------------------------------------------------
void vtkStandaloneSession::SetObjectManagerLogVerbosity(const std::string& verbosityLevel)
{
  return vtkSessionSetObjectManagerLogVerbosity(this->Session, verbosityLevel.c_str());
}

//-------------------------------------------------------------------------------
void vtkStandaloneSession::SetSerializerLogVerbosity(const std::string& verbosityLevel)
{
  return vtkSessionSetSerializerLogVerbosity(this->Session, verbosityLevel.c_str());
};
VTK_ABI_NAMESPACE_END
