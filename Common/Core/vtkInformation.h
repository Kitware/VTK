// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformation
 * @brief   Store vtkAlgorithm input/output information.
 *
 * vtkInformation represents information and/or data for one input or
 * one output of a vtkAlgorithm.  It maps from keys to values of
 * several data types.  Instances of this class are collected in
 * vtkInformationVector instances and passed to
 * vtkAlgorithm::ProcessRequest calls.  The information and
 * data referenced by the instance on a particular input or output
 * define the request made to the vtkAlgorithm instance.
 */

#ifndef vtkInformation_h
#define vtkInformation_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALMANUAL

#include <string> // for std::string compat

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkExecutive;
class vtkInformationDataObjectKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationExecutivePortKey;
class vtkInformationExecutivePortVectorKey;
class vtkInformationIdTypeKey;
class vtkInformationInformationKey;
class vtkInformationInformationVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerPointerKey;
class vtkInformationIntegerVectorKey;
class vtkInformationInternals;
class vtkInformationKey;
class vtkInformationKeyToInformationFriendship;
class vtkInformationKeyVectorKey;
class vtkInformationObjectBaseKey;
class vtkInformationObjectBaseVectorKey;
class vtkInformationRequestKey;
class vtkInformationStringKey;
class vtkInformationStringVectorKey;
class vtkInformationUnsignedLongKey;
class vtkInformationVariantKey;
class vtkInformationVariantVectorKey;
class vtkInformationVector;
class vtkVariant;

class VTKCOMMONCORE_EXPORT VTK_MARSHALMANUAL vtkInformation : public vtkObject
{
public:
  static vtkInformation* New();
  vtkTypeMacro(vtkInformation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  void PrintKeys(ostream& os, vtkIndent indent);

  /**
   * Modified signature with no arguments that calls Modified
   * on vtkObject superclass.
   */
  void Modified() override;

  /**
   * Modified signature that takes an information key as an argument.
   * Sets the new MTime and invokes a modified event with the
   * information key as call data.
   */
  void Modified(vtkInformationKey* key);

  /**
   * Clear all information entries.
   */
  void Clear();

  /**
   * Return the number of keys in this information object (as would be returned
   * by iterating over the keys).
   */
  int GetNumberOfKeys(); // can't const

  /**
   * Copy all information entries from the given vtkInformation
   * instance.  Any previously existing entries are removed.  If
   * deep==1, a deep copy of the information structure is performed (new
   * instances of any contained vtkInformation and vtkInformationVector
   * objects are created).
   */
  void Copy(vtkInformation* from, vtkTypeBool deep = 0);

  /**
   * Append all information entries from the given vtkInformation
   * instance. If deep==1, a deep copy of the information structure is performed
   * (new instances of any contained vtkInformation and vtkInformationVector
   * objects are created).
   */
  void Append(vtkInformation* from, vtkTypeBool deep = 0);

  ///@{
  /**
   * Copy the key/value pair associated with the given key in the
   * given information object.  If deep=1, a deep copy of the information
   * structure is performed (new instances of any contained vtkInformation and
   * vtkInformationVector objects are created).
   */
  void CopyEntry(vtkInformation* from, vtkInformationKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationDataObjectKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationDoubleVectorKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationVariantKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationVariantVectorKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationInformationKey* key, vtkTypeBool deep = 0);
  void CopyEntry(
    vtkInformation* from, vtkInformationInformationVectorKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationIntegerKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationIntegerVectorKey* key, vtkTypeBool deep = 0);
  void CopyEntry(
    vtkInformation* from, vtkInformationObjectBaseVectorKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationRequestKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationStringKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationStringVectorKey* key, vtkTypeBool deep = 0);
  void CopyEntry(vtkInformation* from, vtkInformationUnsignedLongKey* key, vtkTypeBool deep = 0);
  ///@}

  /**
   * Use the given key to lookup a list of other keys in the given
   * information object.  The key/value pairs associated with these
   * other keys will be copied.  If deep==1, a deep copy of the
   * information structure is performed.
   */
  void CopyEntries(vtkInformation* from, vtkInformationKeyVectorKey* key, vtkTypeBool deep = 0);

  /**
   * Check whether the given key appears in this information object.
   */
  int Has(vtkInformationKey* key) VTK_FUTURE_CONST;

  /**
   * Remove the given key and its data from this information object.
   */
  void Remove(vtkInformationKey* key);

  ///@{
  /**
   * Get/Set a request-valued entry.
   */
  void Set(vtkInformationRequestKey* key);
  void Remove(vtkInformationRequestKey* key);
  int Has(vtkInformationRequestKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an integer-valued entry.
   */
  void Set(vtkInformationIntegerKey* key, int value);
  int Get(vtkInformationIntegerKey* key);
  void Remove(vtkInformationIntegerKey* key);
  int Has(vtkInformationIntegerKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set a vtkIdType-valued entry.
   */
  void Set(vtkInformationIdTypeKey* key, vtkIdType value);
  vtkIdType Get(vtkInformationIdTypeKey* key);
  void Remove(vtkInformationIdTypeKey* key);
  int Has(vtkInformationIdTypeKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an double-valued entry.
   */
  void Set(vtkInformationDoubleKey* key, double value);
  double Get(vtkInformationDoubleKey* key);
  void Remove(vtkInformationDoubleKey* key);
  int Has(vtkInformationDoubleKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an variant-valued entry.
   */
  void Set(vtkInformationVariantKey* key, const vtkVariant& value);
  const vtkVariant& Get(vtkInformationVariantKey* key);
  void Remove(vtkInformationVariantKey* key);
  int Has(vtkInformationVariantKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an integer-vector-valued entry.
   */
  void Append(vtkInformationIntegerVectorKey* key, int value);
  void Set(vtkInformationIntegerVectorKey* key, const int* value, int length);
  void Set(vtkInformationIntegerVectorKey* key, int value1, int value2, int value3);
  void Set(vtkInformationIntegerVectorKey* key, int value1, int value2, int value3, int value4,
    int value5, int value6);
  int* Get(vtkInformationIntegerVectorKey* key);
  int Get(vtkInformationIntegerVectorKey* key, int idx);
  void Get(vtkInformationIntegerVectorKey* key, int* value);
  int Length(vtkInformationIntegerVectorKey* key);
  void Remove(vtkInformationIntegerVectorKey* key);
  int Has(vtkInformationIntegerVectorKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set a string-vector-valued entry.
   */
  void Append(vtkInformationStringVectorKey* key, const char* value);
  void Set(vtkInformationStringVectorKey* key, const char* value, int idx = 0);
  void Append(vtkInformationStringVectorKey* key, const std::string& value);
  void Set(vtkInformationStringVectorKey* key, const std::string& value, int idx = 0);
  const char* Get(vtkInformationStringVectorKey* key, int idx = 0);
  int Length(vtkInformationStringVectorKey* key);
  void Remove(vtkInformationStringVectorKey* key);
  int Has(vtkInformationStringVectorKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an integer-pointer-valued entry.
   */
  void Set(vtkInformationIntegerPointerKey* key, int* value, int length);
  int* Get(vtkInformationIntegerPointerKey* key);
  void Get(vtkInformationIntegerPointerKey* key, int* value);
  int Length(vtkInformationIntegerPointerKey* key);
  void Remove(vtkInformationIntegerPointerKey* key);
  int Has(vtkInformationIntegerPointerKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an unsigned-long-valued entry.
   */
  void Set(vtkInformationUnsignedLongKey* key, unsigned long value);
  unsigned long Get(vtkInformationUnsignedLongKey* key);
  void Remove(vtkInformationUnsignedLongKey* key);
  int Has(vtkInformationUnsignedLongKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an double-vector-valued entry.
   */
  void Append(vtkInformationDoubleVectorKey* key, double value);
  void Set(vtkInformationDoubleVectorKey* key, const double* value, int length);
  void Set(vtkInformationDoubleVectorKey* key, double value1, double value2, double value3);
  void Set(vtkInformationDoubleVectorKey* key, double value1, double value2, double value3,
    double value4, double value5, double value6);
  double* Get(vtkInformationDoubleVectorKey* key);
  double Get(vtkInformationDoubleVectorKey* key, int idx);
  void Get(vtkInformationDoubleVectorKey* key, double* value);
  int Length(vtkInformationDoubleVectorKey* key);
  void Remove(vtkInformationDoubleVectorKey* key);
  int Has(vtkInformationDoubleVectorKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an variant-vector-valued entry.
   */
  void Append(vtkInformationVariantVectorKey* key, const vtkVariant& value);
  void Set(vtkInformationVariantVectorKey* key, const vtkVariant* value, int length);
  void Set(vtkInformationVariantVectorKey* key, const vtkVariant& value1, const vtkVariant& value2,
    const vtkVariant& value3);
  void Set(vtkInformationVariantVectorKey* key, const vtkVariant& value1, const vtkVariant& value2,
    const vtkVariant& value3, const vtkVariant& value4, const vtkVariant& value5,
    const vtkVariant& value6);
  const vtkVariant* Get(vtkInformationVariantVectorKey* key);
  const vtkVariant& Get(vtkInformationVariantVectorKey* key, int idx);
  void Get(vtkInformationVariantVectorKey* key, vtkVariant* value);
  int Length(vtkInformationVariantVectorKey* key);
  void Remove(vtkInformationVariantVectorKey* key);
  int Has(vtkInformationVariantVectorKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an InformationKey-vector-valued entry.
   */
  void Append(vtkInformationKeyVectorKey* key, vtkInformationKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationKey* value);
  void Set(vtkInformationKeyVectorKey* key, vtkInformationKey* const* value, int length);
  void Remove(vtkInformationKeyVectorKey* key, vtkInformationKey* value);
  vtkInformationKey** Get(vtkInformationKeyVectorKey* key);
  vtkInformationKey* Get(vtkInformationKeyVectorKey* key, int idx);
  void Get(vtkInformationKeyVectorKey* key, vtkInformationKey** value);
  int Length(vtkInformationKeyVectorKey* key);
  void Remove(vtkInformationKeyVectorKey* key);
  int Has(vtkInformationKeyVectorKey* key) VTK_FUTURE_CONST;
  ///@}

  // Provide extra overloads of this method to avoid requiring user
  // code to include the headers for these key types.  Avoid wrapping
  // them because the original method can be called from the wrappers
  // anyway and this causes a python help string to be too long.

  void Append(vtkInformationKeyVectorKey* key, vtkInformationDataObjectKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationDoubleKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationDoubleVectorKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationInformationKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationInformationVectorKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationIntegerKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationIntegerVectorKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationStringKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationStringVectorKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationObjectBaseKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationUnsignedLongKey* value);

  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationDataObjectKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationDoubleKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationDoubleVectorKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationInformationKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationInformationVectorKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationIntegerKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationIntegerVectorKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationStringKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationStringVectorKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationObjectBaseKey* value);
  void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationUnsignedLongKey* value);

  ///@{
  /**
   * Get/Set a string-valued entry.
   */
  void Set(vtkInformationStringKey* key, const char*);
  void Set(vtkInformationStringKey* key, const std::string&);
  const char* Get(vtkInformationStringKey* key);
  void Remove(vtkInformationStringKey* key);
  int Has(vtkInformationStringKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an entry storing another vtkInformation instance.
   */
  void Set(vtkInformationInformationKey* key, vtkInformation*);
  vtkInformation* Get(vtkInformationInformationKey* key);
  void Remove(vtkInformationInformationKey* key);
  int Has(vtkInformationInformationKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an entry storing a vtkInformationVector instance.
   */
  void Set(vtkInformationInformationVectorKey* key, vtkInformationVector*);
  vtkInformationVector* Get(vtkInformationInformationVectorKey* key);
  void Remove(vtkInformationInformationVectorKey* key);
  int Has(vtkInformationInformationVectorKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an entry storing a vtkObjectBase instance.
   */
  void Set(vtkInformationObjectBaseKey* key, vtkObjectBase*);
  vtkObjectBase* Get(vtkInformationObjectBaseKey* key);
  void Remove(vtkInformationObjectBaseKey* key);
  int Has(vtkInformationObjectBaseKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Manipulate a ObjectBaseVector entry.
   */
  void Append(vtkInformationObjectBaseVectorKey* key, vtkObjectBase* data);
  void Set(vtkInformationObjectBaseVectorKey* key, vtkObjectBase* value, int idx = 0);
  vtkObjectBase* Get(vtkInformationObjectBaseVectorKey* key, int idx = 0);
  int Length(vtkInformationObjectBaseVectorKey* key);
  void Remove(vtkInformationObjectBaseVectorKey* key);
  void Remove(vtkInformationObjectBaseVectorKey* key, vtkObjectBase* objectToRemove);
  void Remove(vtkInformationObjectBaseVectorKey* key, int indexToRemove);
  int Has(vtkInformationObjectBaseVectorKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Get/Set an entry storing a vtkDataObject instance.
   */
  void Set(vtkInformationDataObjectKey* key, vtkDataObject VTK_WRAP_EXTERN*);
  vtkDataObject VTK_WRAP_EXTERN* Get(vtkInformationDataObjectKey* key);
  void Remove(vtkInformationDataObjectKey* key);
  int Has(vtkInformationDataObjectKey* key) VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Upcast the given key instance.
   */
  static vtkInformationKey* GetKey(vtkInformationDataObjectKey* key);
  static vtkInformationKey* GetKey(vtkInformationDoubleKey* key);
  static vtkInformationKey* GetKey(vtkInformationDoubleVectorKey* key);
  static vtkInformationKey* GetKey(vtkInformationInformationKey* key);
  static vtkInformationKey* GetKey(vtkInformationInformationVectorKey* key);
  static vtkInformationKey* GetKey(vtkInformationIntegerKey* key);
  static vtkInformationKey* GetKey(vtkInformationIntegerVectorKey* key);
  static vtkInformationKey* GetKey(vtkInformationRequestKey* key);
  static vtkInformationKey* GetKey(vtkInformationStringKey* key);
  static vtkInformationKey* GetKey(vtkInformationStringVectorKey* key);
  static vtkInformationKey* GetKey(vtkInformationKey* key);
  static vtkInformationKey* GetKey(vtkInformationUnsignedLongKey* key);
  static vtkInformationKey* GetKey(vtkInformationVariantKey* key);
  static vtkInformationKey* GetKey(vtkInformationVariantVectorKey* key);
  ///@}

  ///@{
  /**
   * Initiate garbage collection when a reference is removed.
   */
  bool UsesGarbageCollector() const override { return true; }
  ///@}

  ///@{
  /**
   * Get/Set the Request ivar
   */
  void SetRequest(vtkInformationRequestKey* request);
  vtkInformationRequestKey* GetRequest() VTK_FUTURE_CONST;
  ///@}

protected:
  vtkInformation();
  ~vtkInformation() override;

  // Get/Set a map entry directly through the vtkObjectBase instance
  // representing the value.  Used internally to manage the map.
  void SetAsObjectBase(vtkInformationKey* key, vtkObjectBase* value);
  const vtkObjectBase* GetAsObjectBase(const vtkInformationKey* key) const;
  vtkObjectBase* GetAsObjectBase(vtkInformationKey* key);

  // Internal implementation details.
  vtkInformationInternals* Internal;

  // Garbage collection support.
  void ReportReferences(vtkGarbageCollector*) override;

  // Report the object associated with the given key to the collector.
  void ReportAsObjectBase(vtkInformationKey* key, vtkGarbageCollector* collector);

private:
  friend class vtkInformationKeyToInformationFriendship;
  friend class vtkInformationIterator;

  vtkInformation(const vtkInformation&) = delete;
  void operator=(const vtkInformation&) = delete;
  vtkInformationRequestKey* Request;
};

VTK_ABI_NAMESPACE_END
#endif
