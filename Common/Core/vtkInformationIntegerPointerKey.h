// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationIntegerPointerKey
 * @brief   Key for pointer to integer.
 *
 * vtkInformationIntegerPointerKey is used to represent keys for pointer
 * to integer values in vtkInformation.h
 */

#ifndef vtkInformationIntegerPointerKey_h
#define vtkInformationIntegerPointerKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkInformationIntegerPointerKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationIntegerPointerKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationIntegerPointerKey(const char* name, const char* location, int length = -1);
  ~vtkInformationIntegerPointerKey() override;

  ///@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, int* value, int length);
  int* Get(vtkInformation* info);
  void Get(vtkInformation* info, int* value);
  int Length(vtkInformation* info);
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
  // The required length of the vector value (-1 is no restriction).
  int RequiredLength;

  /**
   * Get the address at which the actual value is stored.  This is
   * meant for use from a debugger to add watches and is therefore not
   * a public method.
   */
  int* GetWatchAddress(vtkInformation* info);

private:
  vtkInformationIntegerPointerKey(const vtkInformationIntegerPointerKey&) = delete;
  void operator=(const vtkInformationIntegerPointerKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
