// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOverrideAttribute
 * @brief   Attribute for vtkObjectFactory overrides
 *
 * vtkOverrideAttribute represents a key/value pair attribute
 * associated with an override class.
 * Attributes are organized as a linked list and used to select
 * the best override based on user preferences.
 *
 * @par Usage:
 * Use the static CreateAttributeChain() method to create
 * a linked list of attributes. For example:
 * \code
 * vtkOverrideAttribute* vtkMyOverrideClass::CreateOverrideAttributes()
 * {
 *   auto* platformAttribute =
 *     vtkOverrideAttribute::CreateAttributeChain("Platform", "iOS", nullptr);
 *   auto* windowSystemAttribute =
 *     vtkOverrideAttribute::CreateAttributeChain("WindowSystem", "Cocoa", platformAttribute);
 *   auto* renderingAttribute =
 *     vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "OpenGL",
 *      windowSystemAttribute);
 *   return renderingAttribute;
 * }
 * \endcode
 *
 * @par Requirements:
 * Override classes must define a vtkClassName_OVERRIDE_ATTRIBUTES macro:
 * \code
 * #define vtkClassName_OVERRIDE_ATTRIBUTES vtkClassName::CreateOverrideAttributes()
 * \endcode
 *
 * @sa vtkObjectFactory
 */
#ifndef vtkOverrideAttribute_h
#define vtkOverrideAttribute_h
#include "vtkObject.h"

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSmartPointer.h"     // For vtkSmartPointer

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkOverrideAttribute : public vtkObject
{
public:
  static vtkOverrideAttribute* New();
  vtkTypeMacro(vtkOverrideAttribute, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /** Get the name of the attribute.
   */
  vtkGetCharFromStdStringMacro(Name);

  ///@{
  /** Get the value of the attribute.
   */
  vtkGetCharFromStdStringMacro(Value);
  ///@}

  ///@{
  /** Get the next attribute in the linked list.
   */
  vtkGetSmartPointerMacro(Next, vtkOverrideAttribute);
  ///@}

  static vtkOverrideAttribute* CreateAttributeChain(
    const char* name, const char* value, vtkOverrideAttribute* nextInChain);

protected:
  vtkOverrideAttribute();
  ~vtkOverrideAttribute() override;

private:
  vtkOverrideAttribute(const vtkOverrideAttribute&) = delete;
  void operator=(const vtkOverrideAttribute&) = delete;

  std::string Name;
  std::string Value;
  vtkSmartPointer<vtkOverrideAttribute> Next;
};

VTK_ABI_NAMESPACE_END
#endif
