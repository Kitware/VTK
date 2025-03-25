// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInvoker
 * @brief   Deserialize VTK objects from JSON.
 */
#ifndef vtkInvoker_h
#define vtkInvoker_h

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

class VTKCOMMONCORE_EXPORT vtkInvoker : public vtkObject
{
public:
  static vtkInvoker* New();
  vtkTypeMacro(vtkInvoker, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using HandlerType =
    std::function<nlohmann::json(vtkInvoker*, vtkObjectBase*, const char*, const nlohmann::json&)>;

  nlohmann::json Invoke(
    const vtkTypeUInt32& identifier, const std::string& methodName, const nlohmann::json& args);

  ///@{
  /**
   * The handlers are used to call a named method.
   *
   * @note
   * If a class does not have a handler, this class will
   * print a stack trace to help you understand the reason for failure.
   */
  void RegisterHandler(const std::type_info& type, HandlerType Invoker);
  HandlerType GetHandler(const std::type_info& type) const;
  bool UnRegisterHandler(const std::type_info& type);
  ///@}

  ///@{
  /**
   * Get/Set the marshalling context.
   * The vtkInvoker does not track state of any object.
   * However, it leverages the context to discover objects and invoke methods.
   */
  vtkSetSmartPointerMacro(Context, vtkMarshalContext);
  vtkGetSmartPointerMacro(Context, vtkMarshalContext);
  ///@}

  ///@{
  /**
   * Set/Get the log verbosity of messages that are emitted when data is uploaded to GPU memory.
   * The GetInvokerLogVerbosity looks up system environment for
   * `VTK_Invoker_LOG_VERBOSITY` that shall be used to set initial logger verbosity. The
   * default value is TRACE.
   *
   * Accepted string values are OFF, ERROR, WARNING, INFO, TRACE, MAX, INVALID or ASCII
   * representation for an integer in the range [-9,9].
   *
   * @note This method internally uses vtkLogger::ConvertToVerbosity(const char*) to parse the
   * value from environment variable.
   */
  void SetInvokerLogVerbosity(vtkLogger::Verbosity verbosity);
  vtkLogger::Verbosity GetInvokerLogVerbosity();
  ///@}

protected:
  vtkInvoker();
  ~vtkInvoker() override;

  vtkSmartPointer<vtkMarshalContext> Context;
  vtkLogger::Verbosity InvokerLogVerbosity = vtkLogger::VERBOSITY_INVALID;

private:
  vtkInvoker(const vtkInvoker&) = delete;
  void operator=(const vtkInvoker&) = delete;
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
