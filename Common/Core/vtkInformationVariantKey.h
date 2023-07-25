// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationVariantKey
 * @brief   Key for variant values in vtkInformation.
 *
 * vtkInformationVariantKey is used to represent keys for variant values
 * in vtkInformation.
 */

#ifndef vtkInformationVariantKey_h
#define vtkInformationVariantKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

VTK_ABI_NAMESPACE_BEGIN
class vtkVariant;

class VTKCOMMONCORE_EXPORT vtkInformationVariantKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationVariantKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationVariantKey(const char* name, const char* location);
  ~vtkInformationVariantKey() override;

  /**
   * This method simply returns a new vtkInformationVariantKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationVariantKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationVariantKey(name, location);
  }

  ///@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, const vtkVariant&);
  const vtkVariant& Get(vtkInformation* info);
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

protected:
  /**
   * Get the address at which the actual value is stored.  This is
   * meant for use from a debugger to add watches and is therefore not
   * a public method.
   */
  vtkVariant* GetWatchAddress(vtkInformation* info);

private:
  vtkInformationVariantKey(const vtkInformationVariantKey&) = delete;
  void operator=(const vtkInformationVariantKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
