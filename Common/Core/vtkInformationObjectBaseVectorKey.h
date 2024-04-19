// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationObjectBaseVectorKey
 * @brief   Key for vtkObjectBase vector values.
 *
 * vtkInformationObjectBaseVectorKey is used to represent keys for double
 * vector values in vtkInformation.h. NOTE the interface in this key differs
 * from that in other similar keys because of our internal use of smart
 * pointers.
 */

#ifndef vtkInformationObjectBaseVectorKey_h
#define vtkInformationObjectBaseVectorKey_h

#include "vtkCommonCoreModule.h"            // For export macro
#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.
#include "vtkInformationKey.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationObjectBaseVectorValue;

class VTKCOMMONCORE_EXPORT vtkInformationObjectBaseVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationObjectBaseVectorKey, vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@{
  /**
   * The name of the static instance and the class in which
   * it is defined(location) should be passed to the constructor.
   * Providing "requiredClass" name one can ensure that only
   * objects of type "requiredClass" are stored in vectors
   * associated with the instance of this key type created.
   * These should be string literals as they are not copied.
   */
  vtkInformationObjectBaseVectorKey(
    const char* name, const char* location, const char* requiredClass = nullptr);
  //
  ~vtkInformationObjectBaseVectorKey() override;
  ///@}

  /**
   * This method simply returns a new vtkInformationObjectBaseVectorKey, given a
   * name, location and optionally a required class (a classname to restrict
   * which class types can be set with this key). This method is provided
   * for wrappers. Use the constructor directly from C++ instead.
   */
  static VTK_NEWINSTANCE vtkInformationObjectBaseVectorKey* MakeKey(
    const char* name, const char* location, const char* requiredClass = nullptr)
  {
    return new vtkInformationObjectBaseVectorKey(name, location, requiredClass);
  }

  /**
   * Clear the vector.
   */
  void Clear(vtkInformation* info);

  /**
   * Resize (extend) the vector to hold size objects. Any new elements
   * created will be null initialized.
   */
  void Resize(vtkInformation* info, int size);

  /**
   * Get the vector's length.
   */
  int Size(vtkInformation* info);
  int Length(vtkInformation* info) { return this->Size(info); }

  /**
   * Put the value on the back of the vector, with ref counting.
   */
  void Append(vtkInformation* info, vtkObjectBase* value);

  /**
   * Set element i of the vector to value. Resizes the vector
   * if needed.
   */
  void Set(vtkInformation* info, vtkObjectBase* value, int i);

  ///@{
  /**
   * Remove all instances of val from the list. If using the indexed overload,
   * the object at the specified position is removed.
   */
  void Remove(vtkInformation* info, vtkObjectBase* val);
  void Remove(vtkInformation* info, int idx);
  using Superclass::Remove; // Don't hide base class methods
  ///@}

  /**
   * Copy n values from the range in source defined by [from  from+n-1]
   * into the range in this vector defined by [to to+n-1]. Resizes
   * the vector if needed.
   */
  void SetRange(vtkInformation* info, vtkObjectBase** source, int from, int to, int n);

  /**
   * Copy n values from the range in this vector defined by [from  from+n-1]
   * into the range in the destination vector defined by [to to+n-1]. Up
   * to you to make sure the destination is big enough.
   */
  void GetRange(vtkInformation* info, vtkObjectBase** dest, int from, int to, int n);

  /**
   * Get the vtkObjectBase at a specific location in the vector.
   */
  vtkObjectBase* Get(vtkInformation* info, int idx);

  // _escription:
  // Get a pointer to the first vtkObjectBase in the vector. We are
  // uysing a vector of smart pointers so this is not easy to
  // implement.
  // vtkObjectBase **Get(vtkInformation* info);

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* source, vtkInformation* dest) override;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) override;

protected:
  // The type required of all objects stored with this key.
  const char* RequiredClass;

private:
  /**
   * Used to create the underlying vector that will be associated
   * with this key.
   */
  void CreateObjectBase();
  /**
   * Check insures that if RequiredClass is set then the
   * type of aValue matches. return true if the two match.
   */
  bool ValidateDerivedType(vtkInformation* info, vtkObjectBase* aValue);
  /**
   * Get the vector associated with this key, if there is
   * none then associate a new vector with this key and return
   * that.
   */
  vtkInformationObjectBaseVectorValue* GetObjectBaseVector(vtkInformation* info);

  //
  vtkInformationObjectBaseVectorKey(const vtkInformationObjectBaseVectorKey&) = delete;
  void operator=(const vtkInformationObjectBaseVectorKey&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
