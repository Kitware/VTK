// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationDoubleKey
 * @brief   Key for double values in vtkInformation.
 *
 * vtkInformationDoubleKey is used to represent keys for double values
 * in vtkInformation.
 */

#ifndef vtkInformationDoubleKey_h
#define vtkInformationDoubleKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkInformationDoubleKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationDoubleKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationDoubleKey(const char* name, const char* location);
  ~vtkInformationDoubleKey() override;

  /**
   * This method simply returns a new vtkInformationDoubleKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationDoubleKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationDoubleKey(name, location);
  }

  ///@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, double);
  double Get(vtkInformation* info);
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
  double* GetWatchAddress(vtkInformation* info);

private:
  vtkInformationDoubleKey(const vtkInformationDoubleKey&) = delete;
  void operator=(const vtkInformationDoubleKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
