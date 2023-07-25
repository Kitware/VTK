// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationInformationKey
 * @brief   Key for vtkInformation values.
 *
 * vtkInformationInformationKey is used to represent keys in vtkInformation
 * for other information objects.
 */

#ifndef vtkInformationInformationKey_h
#define vtkInformationInformationKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkInformationInformationKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationInformationKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationInformationKey(const char* name, const char* location);
  ~vtkInformationInformationKey() override;

  /**
   * This method simply returns a new vtkInformationInformationKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationInformationKey* MakeKey(
    const char* name, const char* location)
  {
    return new vtkInformationInformationKey(name, location);
  }

  ///@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, vtkInformation*);
  vtkInformation* Get(vtkInformation* info);
  ///@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) override;

  /**
   * Duplicate (new instance created) the entry associated with this key from
   * one information object to another (new instances of any contained
   * vtkInformation and vtkInformationVector objects are created).
   */
  void DeepCopy(vtkInformation* from, vtkInformation* to) override;

private:
  vtkInformationInformationKey(const vtkInformationInformationKey&) = delete;
  void operator=(const vtkInformationInformationKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
