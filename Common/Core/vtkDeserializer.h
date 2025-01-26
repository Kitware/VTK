// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDeserializer
 * @brief   Deserialize VTK objects from JSON.
 */
#ifndef vtkDeserializer_h
#define vtkDeserializer_h

#include "vtkObject.h"

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkLogger.h"           // for vtkLogger::Verbosity enum
#include "vtkMarshalContext.h"   // for vtkMarshalContext
#include "vtkSmartPointer.h"     // for vktSmartPointer

// clang-format off
#include "vtk_nlohmannjson.h"            // for json
#include VTK_NLOHMANN_JSON(json_fwd.hpp) // for json
// clang-format on

#include <memory>   // for unique_ptr
#include <typeinfo> // for type_info

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONCORE_EXPORT vtkDeserializer : public vtkObject
{
public:
  static vtkDeserializer* New();
  vtkTypeMacro(vtkDeserializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using HandlerType = std::function<void(const nlohmann::json&, vtkObjectBase*, vtkDeserializer*)>;
  using ConstructorType = std::function<vtkObjectBase*()>;

  /**
   * Constructs an object of type `className`.
   *
   * If a constructor is not found for `className`, the `GetConstructor` walks through each item
   * in `superClassNames` and attempts to construct an instance of that type.
   * This is useful when the VTK build of the serializer side and the deserializer side
   * are on entirely different platforms by taking advantage of the object factory mechanism.
   *
   *
   * Example of usefulness of `superClassNames`:
   *
   * Let's suppose in a Windows VTK application, the `vtkSerializer` serialized an instance of
   * `vtkWin32RenderWindowInteractor` into json which was then transferred over the network
   * to a macOS machine. Over there, seeing that the state refers to the
   * `vtkWin32RenderWindowInteractor` class the `vtkDeserializer` will attempt to find a constructor
   * for win32 class and fail. It then checks if the super class (here `vtkRenderWindowInteractor`)
   * has a constructor and constructs a new instance of that type. Due to the object factory
   * mechanism, the macOS build of VTK constructs a `vtkCocoaRenderWindowInteractor` and it all
   * works as expected!
   */
  vtkObjectBase* ConstructObject(
    const std::string& className, const std::vector<std::string>& superClassNames);

  /**
   * Deserialize a state registered with the context at `identifier` into `objectBase`.
   * This function lets you pass a non-null object into `objectBase` typically obtained
   * from vtkMarshalContext::GetObjectAtId. In that case, the constructor is not invoked.
   * Otherwise, a new object will be constructed and available in `objectBase`.
   *
   * This method returns `true` if the state was successfully deserialized and `false` when an error
   * occurs. This method returns `true` if the state was already deserialized into an object.
   */
  bool DeserializeJSON(const vtkTypeUInt32& identifier, vtkSmartPointer<vtkObjectBase>& objectBase);

  ///@{
  /**
   * The constructors are invoked to construct an instance of `className`.
   *
   * @note
   * If `className` does not have a registered constructor, the deserializer will
   * print a stack trace to help you understand the reason for failure.
   */
  void RegisterConstructor(const std::string& className, ConstructorType constructor);
  ConstructorType GetConstructor(
    const std::string& className, const std::vector<std::string>& superClassNames);
  void UnRegisterConstructor(const std::string& className);
  ///@}

  ///@{
  /**
   * The handlers are invoked to deserialize a json state into a `vtkObjectBase` derived
   * instance of type `type`.
   *
   * @note
   * If a class does not have a handler, this class will
   * print a stack trace to help you understand the reason for failure.
   */
  void RegisterHandler(const std::type_info& type, HandlerType deserializer);
  HandlerType GetHandler(const std::type_info& type) const;
  bool UnRegisterHandler(const std::type_info& type);
  ///@}

  ///@{
  /**
   * Get/Set the marshalling context.
   * The vtkDeserializer does not track state of any object.
   * However, it leverages the context to prevent re-deserialization
   * when there are circular dependencies among the VTK objects.
   * The context does much more than just preventing recursive de-serialization.
   * The deserializer records parent-child relationships
   * in the context using it's ScopedParentTracker API.
   */
  vtkSetSmartPointerMacro(Context, vtkMarshalContext);
  vtkGetSmartPointerMacro(Context, vtkMarshalContext);
  ///@}

  ///@{
  /**
   * Set/Get the log verbosity of messages that are emitted when data is uploaded to GPU memory.
   * The GetDeserializerLogVerbosity looks up system environment for
   * `VTK_DESERIALIZER_LOG_VERBOSITY` that shall be used to set initial logger verbosity. The
   * default value is TRACE.
   *
   * Accepted string values are OFF, ERROR, WARNING, INFO, TRACE, MAX, INVALID or ASCII
   * representation for an integer in the range [-9,9].
   *
   * @note This method internally uses vtkLogger::ConvertToVerbosity(const char*) to parse the
   * value from environment variable.
   */
  void SetDeserializerLogVerbosity(vtkLogger::Verbosity verbosity);
  vtkLogger::Verbosity GetDeserializerLogVerbosity();
  ///@}

protected:
  vtkDeserializer();
  ~vtkDeserializer() override;

  vtkSmartPointer<vtkMarshalContext> Context;
  vtkLogger::Verbosity DeserializerLogVerbosity = vtkLogger::VERBOSITY_INVALID;

private:
  vtkDeserializer(const vtkDeserializer&) = delete;
  void operator=(const vtkDeserializer&) = delete;
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

/**
 * Convenient to get value for a property from the state and apply the value on a vtk object.
 */
#define VTK_DESERIALIZE_VALUE_FROM_STATE(name, type, state, object)                                \
  do                                                                                               \
  {                                                                                                \
    const auto iter = state.find(#name);                                                           \
    if ((iter != state.end()) && !iter->is_null())                                                 \
    {                                                                                              \
      object->Set##name(iter->get<type>());                                                        \
    }                                                                                              \
  } while (0)

/**
 * Convenient to get a vtkObject property from the state and set it on another vtk
 * object. `stateKey` is the name used in state. `propertyName` is the name used by
 * the VTK class Set/Get macros or a `SetSomething()` function. This is a special case.
 */
#define VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE_DIFFERENT_NAMES(                                     \
  stateKey, propertyName, cls, state, object, deserializer)                                        \
  do                                                                                               \
  {                                                                                                \
    const auto iter = state.find(#stateKey);                                                       \
    if ((iter != state.end()) && !iter->is_null())                                                 \
    {                                                                                              \
      const auto* context = deserializer->GetContext();                                            \
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();                                 \
      auto subObject = context->GetObjectAtId(identifier);                                         \
      deserializer->DeserializeJSON(identifier, subObject);                                        \
      if (auto* asVtkType = cls::SafeDownCast(subObject))                                          \
      {                                                                                            \
        object->Set##propertyName(asVtkType);                                                      \
      }                                                                                            \
    }                                                                                              \
  } while (0)

/**
 * Similar to above, when the state and VTK class property have the same name. This is common.
 */
#define VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(name, cls, state, object, deserializer)              \
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE_DIFFERENT_NAMES(                                           \
    name, name, cls, state, object, deserializer)

/**
 * Convenient to get a vector of values for a property from the state and apply the values on a vtk
 * object.
 */
#define VTK_DESERIALIZE_VECTOR_FROM_STATE(name, type, state, object)                               \
  do                                                                                               \
  {                                                                                                \
    const auto iter = state.find(#name);                                                           \
    if ((iter != state.end()) && !iter->is_null())                                                 \
    {                                                                                              \
      using namespace std;                                                                         \
      const auto elements = iter->get<vector<type>>();                                             \
      object->Set##name(elements.data());                                                          \
    }                                                                                              \
  } while (0)

VTK_ABI_NAMESPACE_END
#endif
