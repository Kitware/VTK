// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationStringKey
 * @brief   Key for string values in vtkInformation.
 *
 * vtkInformationStringKey is used to represent keys for string values
 * in vtkInformation.
 */

#ifndef vtkInformationStringKey_h
#define vtkInformationStringKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

#include <string> // for std::string compat

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkInformationStringKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationStringKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationStringKey(const char* name, const char* location);
  ~vtkInformationStringKey() override;

  /**
   * This method simply returns a new vtkInformationStringKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationStringKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationStringKey(name, location);
  }

  ///@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, const char*);
  void Set(vtkInformation* info, const std::string& str);
  const char* Get(vtkInformation* info);
  ///@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) override;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) override;

private:
  vtkInformationStringKey(const vtkInformationStringKey&) = delete;
  void operator=(const vtkInformationStringKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
