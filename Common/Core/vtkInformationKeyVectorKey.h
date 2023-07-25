// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationKeyVectorKey
 * @brief   Key for vector-of-keys values.
 *
 * vtkInformationKeyVectorKey is used to represent keys for
 * vector-of-keys values in vtkInformation.
 */

#ifndef vtkInformationKeyVectorKey_h
#define vtkInformationKeyVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkInformationKeyVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationKeyVectorKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationKeyVectorKey(const char* name, const char* location);
  ~vtkInformationKeyVectorKey() override;

  /**
   * This method simply returns a new vtkInformationKeyVectorKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationKeyVectorKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationKeyVectorKey(name, location);
  }

  ///@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(vtkInformation* info, vtkInformationKey* value);
  void AppendUnique(vtkInformation* info, vtkInformationKey* value);
  void Set(vtkInformation* info, vtkInformationKey* const* value, int length);
  void RemoveItem(vtkInformation* info, vtkInformationKey* value);
  vtkInformationKey** Get(vtkInformation* info);
  vtkInformationKey* Get(vtkInformation* info, int idx);
  void Get(vtkInformation* info, vtkInformationKey** value);
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

private:
  vtkInformationKeyVectorKey(const vtkInformationKeyVectorKey&) = delete;
  void operator=(const vtkInformationKeyVectorKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
