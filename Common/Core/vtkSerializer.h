// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSerializer
 * @brief   Serialize VTK objects to JSON.
 */
#ifndef vtkSerializer_h
#define vtkSerializer_h

#include "vtkObject.h"

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkMarshalContext.h"   // for vtkMarshalContext
#include "vtkSmartPointer.h"     // for vktSmartPointer

// clang-format off
#include "vtk_nlohmannjson.h"        // for json
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

#include <memory>   // for unique_ptr
#include <typeinfo> // for type_info

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONCORE_EXPORT vtkSerializer : public vtkObject
{
public:
  static vtkSerializer* New();
  vtkTypeMacro(vtkSerializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using HandlerType = std::function<nlohmann::json(vtkObjectBase*, vtkSerializer*)>;

  /**
   * Serialize the VTK object
   */
  nlohmann::json SerializeJSON(vtkObjectBase* objectBase);

  ///@{
  /**
   * The handlers are invoked to serialize an object of type `type`.
   *
   * @note
   * If `type` does not have a handler, the serializer will
   * print a stack trace to help you understand the reason for failure.
   */
  void RegisterHandler(const std::type_info& type, HandlerType handler);
  HandlerType GetHandler(const std::type_info& type) const;
  bool UnRegisterHandler(const std::type_info& type);
  ///@}

  ///@{
  /**
   * Get/Set the marshalling context.
   * The vtkSerializer does not track state of any object.
   * However, it leverages the context to prevent re-serialization
   * when there are circular dependencies among VTK objects.
   * The context does much more than just preventing recursive serialization.
   * The serializer records parent-child relationships
   * in the context using it's ScopedParentTracker API.
   */
  vtkSetSmartPointerMacro(Context, vtkMarshalContext);
  vtkGetSmartPointerMacro(Context, vtkMarshalContext);
  ///@}

protected:
  vtkSerializer();
  ~vtkSerializer() override;

  vtkSmartPointer<vtkMarshalContext> Context;

private:
  vtkSerializer(const vtkSerializer&) = delete;
  void operator=(const vtkSerializer&) = delete;
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};
VTK_ABI_NAMESPACE_END
#endif
