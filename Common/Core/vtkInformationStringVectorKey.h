// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationStringVectorKey
 * @brief   Key for String vector values.
 *
 * vtkInformationStringVectorKey is used to represent keys for String
 * vector values in vtkInformation.h
 */

#ifndef vtkInformationStringVectorKey_h
#define vtkInformationStringVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

#include <string> // for std::string compat

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkInformationStringVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationStringVectorKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationStringVectorKey(const char* name, const char* location, int length = -1);
  ~vtkInformationStringVectorKey() override;

  /**
   * This method simply returns a new vtkInformationStringVectorKey, given a
   * name, a location and a required length. This method is provided for
   * wrappers. Use the constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationStringVectorKey* MakeKey(
    const char* name, const char* location, int length = -1)
  {
    return new vtkInformationStringVectorKey(name, location, length);
  }

  ///@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(vtkInformation* info, const char* value);
  void Set(vtkInformation* info, const char* value, int index = 0);
  void Append(vtkInformation* info, const std::string& value);
  void Set(vtkInformation* info, const std::string& value, int idx = 0);
  const char* Get(vtkInformation* info, int idx = 0);
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

private:
  vtkInformationStringVectorKey(const vtkInformationStringVectorKey&) = delete;
  void operator=(const vtkInformationStringVectorKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
