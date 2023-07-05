// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationIntegerKey
 * @brief   Key for integer values in vtkInformation.
 *
 * vtkInformationIntegerKey is used to represent keys for integer values
 * in vtkInformation.
 */

#ifndef vtkInformationIntegerKey_h
#define vtkInformationIntegerKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkInformationIntegerKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationIntegerKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationIntegerKey(const char* name, const char* location);
  ~vtkInformationIntegerKey() override;

  /**
   * This method simply returns a new vtkInformationIntegerKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationIntegerKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationIntegerKey(name, location);
  }

  ///@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, int);
  int Get(vtkInformation* info);
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
  int* GetWatchAddress(vtkInformation* info);

private:
  vtkInformationIntegerKey(const vtkInformationIntegerKey&) = delete;
  void operator=(const vtkInformationIntegerKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
