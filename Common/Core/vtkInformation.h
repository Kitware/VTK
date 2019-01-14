/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include <string> // for std::string compat

// If being "compiled" by gccxml, pretend VTKCOMMONCORE_EXPORT is nothing
// for this header file. The per-method usage of VTKCOMMONCORE_EXPORT in
// this header file leads to gccxml errors without this workaround.
//
#ifdef __GCCXML__
#undef VTKCOMMONCORE_EXPORT
#define VTKCOMMONCORE_EXPORT
#endif

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

#if defined(_WIN32)
# define VTK_INFORMATION_EXPORT
#else
# define VTK_INFORMATION_EXPORT VTKCOMMONCORE_EXPORT
#endif


class VTK_INFORMATION_EXPORT vtkInformation : public vtkObject
{
public:
  VTKCOMMONCORE_EXPORT static vtkInformation *New();
  vtkTypeMacro(vtkInformation,vtkObject);
  VTKCOMMONCORE_EXPORT void PrintSelf(ostream& os, vtkIndent indent) override;
  VTKCOMMONCORE_EXPORT void PrintKeys(ostream& os, vtkIndent indent);

  /**
   * Modified signature with no arguments that calls Modified
   * on vtkObject superclass.
   */
  VTKCOMMONCORE_EXPORT void Modified() override;

  /**
   * Modified signature that takes an information key as an argument.
   * Sets the new MTime and invokes a modified event with the
   * information key as call data.
   */
  VTKCOMMONCORE_EXPORT void Modified(vtkInformationKey* key);

  /**
   * Clear all information entries.
   */
  VTKCOMMONCORE_EXPORT void Clear();

  /**
   * Return the number of keys in this information object (as would be returned
   * by iterating over the keys).
   */
  VTKCOMMONCORE_EXPORT int GetNumberOfKeys();

  /**
   * Copy all information entries from the given vtkInformation
   * instance.  Any previously existing entries are removed.  If
   * deep==1, a deep copy of the information structure is performed (new
   * instances of any contained vtkInformation and vtkInformationVector
   * objects are created).
   */
  VTKCOMMONCORE_EXPORT void Copy(vtkInformation* from, int deep=0);

  /**
   * Append all information entries from the given vtkInformation
   * instance. If deep==1, a deep copy of the information structure is performed
   * (new instances of any contained vtkInformation and vtkInformationVector
   * objects are created).
   */
  VTKCOMMONCORE_EXPORT void Append(vtkInformation* from, int deep=0);

  //@{
  /**
   * Copy the key/value pair associated with the given key in the
   * given information object.  If deep=1, a deep copy of the information
   * structure is performed (new instances of any contained vtkInformation and
   * vtkInformationVector objects are created).
   */
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationDataObjectKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationDoubleVectorKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationVariantKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationVariantVectorKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationInformationKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationInformationVectorKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationIntegerKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationIntegerVectorKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationObjectBaseVectorKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationRequestKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationStringKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationStringVectorKey* key, int deep=0);
  VTKCOMMONCORE_EXPORT void CopyEntry(vtkInformation* from, vtkInformationUnsignedLongKey* key, int deep=0);
  //@}

  /**
   * Use the given key to lookup a list of other keys in the given
   * information object.  The key/value pairs associated with these
   * other keys will be copied.  If deep==1, a deep copy of the
   * information structure is performed.
   */
  VTKCOMMONCORE_EXPORT void CopyEntries(vtkInformation* from, vtkInformationKeyVectorKey* key, int deep=0);

  /**
   * Check whether the given key appears in this information object.
   */
  VTKCOMMONCORE_EXPORT int Has(vtkInformationKey* key);

  /**
   * Remove the given key and its data from this information object.
   */
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationKey* key);

  //@{
  /**
   * Get/Set a request-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationRequestKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationRequestKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationRequestKey* key);
  //@}

  //@{
  /**
   * Get/Set an integer-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationIntegerKey* key, int value);
  VTKCOMMONCORE_EXPORT int Get(vtkInformationIntegerKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationIntegerKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationIntegerKey* key);
  //@}

  //@{
  /**
   * Get/Set a vtkIdType-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationIdTypeKey* key, vtkIdType value);
  VTKCOMMONCORE_EXPORT vtkIdType Get(vtkInformationIdTypeKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationIdTypeKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationIdTypeKey* key);
  //@}

  //@{
  /**
   * Get/Set an double-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationDoubleKey* key, double value);
  VTKCOMMONCORE_EXPORT double Get(vtkInformationDoubleKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationDoubleKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationDoubleKey* key);
  //@}

  //@{
  /**
   * Get/Set an variant-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationVariantKey* key, const vtkVariant& value);
  VTKCOMMONCORE_EXPORT const vtkVariant& Get(vtkInformationVariantKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationVariantKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationVariantKey* key);
  //@}

  //@{
  /**
   * Get/Set an integer-vector-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Append(vtkInformationIntegerVectorKey* key, int value);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationIntegerVectorKey* key, const int* value, int length);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationIntegerVectorKey* key, int value1,
           int value2, int value3);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationIntegerVectorKey* key,
           int value1, int value2, int value3,
           int value4, int value5, int value6);
  VTKCOMMONCORE_EXPORT int* Get(vtkInformationIntegerVectorKey* key);
  VTKCOMMONCORE_EXPORT int  Get(vtkInformationIntegerVectorKey* key, int idx);
  VTKCOMMONCORE_EXPORT void Get(vtkInformationIntegerVectorKey* key, int* value);
  VTKCOMMONCORE_EXPORT int Length(vtkInformationIntegerVectorKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationIntegerVectorKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationIntegerVectorKey* key);
  //@}

  //@{
  /**
   * Get/Set a string-vector-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Append(vtkInformationStringVectorKey* key, const char* value);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationStringVectorKey* key, const char* value, int idx = 0);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationStringVectorKey* key, const std::string &value);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationStringVectorKey* key, const std::string &value, int idx = 0);
  VTKCOMMONCORE_EXPORT const char*  Get(vtkInformationStringVectorKey* key, int idx = 0);
  VTKCOMMONCORE_EXPORT int Length(vtkInformationStringVectorKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationStringVectorKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationStringVectorKey* key);
  //@}

  //@{
  /**
   * Get/Set an integer-pointer-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationIntegerPointerKey* key, int* value, int length);
  VTKCOMMONCORE_EXPORT int* Get(vtkInformationIntegerPointerKey* key);
  VTKCOMMONCORE_EXPORT void Get(vtkInformationIntegerPointerKey* key, int* value);
  VTKCOMMONCORE_EXPORT int Length(vtkInformationIntegerPointerKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationIntegerPointerKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationIntegerPointerKey* key);
  //@}

  //@{
  /**
   * Get/Set an unsigned-long-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationUnsignedLongKey* key, unsigned long value);
  VTKCOMMONCORE_EXPORT unsigned long Get(vtkInformationUnsignedLongKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationUnsignedLongKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationUnsignedLongKey* key);
  //@}

  //@{
  /**
   * Get/Set an double-vector-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Append(vtkInformationDoubleVectorKey* key, double value);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationDoubleVectorKey* key, const double* value, int length);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationDoubleVectorKey* key, double value1,
           double value2, double value3);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationDoubleVectorKey* key,
           double value1, double value2, double value3,
           double value4, double value5, double value6);
  VTKCOMMONCORE_EXPORT double* Get(vtkInformationDoubleVectorKey* key);
  VTKCOMMONCORE_EXPORT double  Get(vtkInformationDoubleVectorKey* key, int idx);
  VTKCOMMONCORE_EXPORT void Get(vtkInformationDoubleVectorKey* key, double* value);
  VTKCOMMONCORE_EXPORT int Length(vtkInformationDoubleVectorKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationDoubleVectorKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationDoubleVectorKey* key);
  //@}

  //@{
  /**
   * Get/Set an variant-vector-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Append(vtkInformationVariantVectorKey* key, const vtkVariant& value);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationVariantVectorKey* key, const vtkVariant* value, int length);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationVariantVectorKey* key, const vtkVariant& value1,
           const vtkVariant& value2, const vtkVariant& value3);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationVariantVectorKey* key,
           const vtkVariant& value1, const vtkVariant& value2, const vtkVariant& value3,
           const vtkVariant& value4, const vtkVariant& value5, const vtkVariant& value6);
  VTKCOMMONCORE_EXPORT const vtkVariant* Get(vtkInformationVariantVectorKey* key);
  VTKCOMMONCORE_EXPORT const vtkVariant& Get(vtkInformationVariantVectorKey* key, int idx);
  VTKCOMMONCORE_EXPORT void Get(vtkInformationVariantVectorKey* key, vtkVariant* value);
  VTKCOMMONCORE_EXPORT int Length(vtkInformationVariantVectorKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationVariantVectorKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationVariantVectorKey* key);
  //@}

  //@{
  /**
   * Get/Set an InformationKey-vector-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key, vtkInformationKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key, vtkInformationKey* value);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationKeyVectorKey* key, vtkInformationKey*const * value, int length);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationKeyVectorKey* key, vtkInformationKey* value);
  VTKCOMMONCORE_EXPORT vtkInformationKey** Get(vtkInformationKeyVectorKey* key);
  VTKCOMMONCORE_EXPORT vtkInformationKey*  Get(vtkInformationKeyVectorKey* key, int idx);
  VTKCOMMONCORE_EXPORT void Get(vtkInformationKeyVectorKey* key, vtkInformationKey** value);
  VTKCOMMONCORE_EXPORT int Length(vtkInformationKeyVectorKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationKeyVectorKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationKeyVectorKey* key);
  //@}

  // Provide extra overloads of this method to avoid requiring user
  // code to include the headers for these key types.  Avoid wrapping
  // them because the original method can be called from the wrappers
  // anyway and this causes a python help string to be too long.

  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationDataObjectKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key, vtkInformationDoubleKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationDoubleVectorKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationInformationKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationInformationVectorKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationIntegerKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationIntegerVectorKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key, vtkInformationStringKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationStringVectorKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationObjectBaseKey* value);
  VTKCOMMONCORE_EXPORT void Append(vtkInformationKeyVectorKey* key,
              vtkInformationUnsignedLongKey* value);

  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationDataObjectKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationDoubleKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationDoubleVectorKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationInformationKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationInformationVectorKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationIntegerKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationIntegerVectorKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationStringKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationStringVectorKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationObjectBaseKey* value);
  VTKCOMMONCORE_EXPORT void AppendUnique(vtkInformationKeyVectorKey* key,
                    vtkInformationUnsignedLongKey* value);

  //@{
  /**
   * Get/Set a string-valued entry.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationStringKey* key, const char*);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationStringKey* key, const std::string&);
  VTKCOMMONCORE_EXPORT const char* Get(vtkInformationStringKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationStringKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationStringKey* key);
  //@}

  //@{
  /**
   * Get/Set an entry storing another vtkInformation instance.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationInformationKey* key, vtkInformation*);
  VTKCOMMONCORE_EXPORT vtkInformation* Get(vtkInformationInformationKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationInformationKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationInformationKey* key);
  //@}

  //@{
  /**
   * Get/Set an entry storing a vtkInformationVector instance.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationInformationVectorKey* key, vtkInformationVector*);
  VTKCOMMONCORE_EXPORT vtkInformationVector* Get(vtkInformationInformationVectorKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationInformationVectorKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationInformationVectorKey* key);
  //@}

  //@{
  /**
   * Get/Set an entry storing a vtkObjectBase instance.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationObjectBaseKey* key, vtkObjectBase*);
  VTKCOMMONCORE_EXPORT vtkObjectBase* Get(vtkInformationObjectBaseKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationObjectBaseKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationObjectBaseKey* key);
  //@}

  //@{
  /**
   * Manipulate a ObjectBaseVector entry.
   */
  VTKCOMMONCORE_EXPORT void Append(vtkInformationObjectBaseVectorKey* key,
                                   vtkObjectBase *data);
  VTKCOMMONCORE_EXPORT void Set(vtkInformationObjectBaseVectorKey *key,
                                vtkObjectBase* value, int idx = 0);
  VTKCOMMONCORE_EXPORT vtkObjectBase* Get(vtkInformationObjectBaseVectorKey *key,
                                          int idx = 0);
  VTKCOMMONCORE_EXPORT int Length(vtkInformationObjectBaseVectorKey *key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationObjectBaseVectorKey *key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationObjectBaseVectorKey *key,
                                   vtkObjectBase *objectToRemove);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationObjectBaseVectorKey *key,
                                   int indexToRemove);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationObjectBaseVectorKey *key);
  //@}

  //@{
  /**
   * Get/Set an entry storing a vtkDataObject instance.
   */
  VTKCOMMONCORE_EXPORT void Set(vtkInformationDataObjectKey* key,
    vtkDataObject VTK_WRAP_EXTERN *);
  VTKCOMMONCORE_EXPORT vtkDataObject VTK_WRAP_EXTERN* Get(vtkInformationDataObjectKey* key);
  VTKCOMMONCORE_EXPORT void Remove(vtkInformationDataObjectKey* key);
  VTKCOMMONCORE_EXPORT int Has(vtkInformationDataObjectKey* key);
  //@}

  //@{
  /**
   * Upcast the given key instance.
   */
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationDataObjectKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationDoubleKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationDoubleVectorKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationInformationKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationInformationVectorKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationIntegerKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationIntegerVectorKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationRequestKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationStringKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationStringVectorKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationUnsignedLongKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationVariantKey* key);
  VTKCOMMONCORE_EXPORT static vtkInformationKey* GetKey(vtkInformationVariantVectorKey* key);
  //@}

  //@{
  /**
   * Initiate garbage collection when a reference is removed.
   */
  VTKCOMMONCORE_EXPORT void Register(vtkObjectBase* o) override;
  VTKCOMMONCORE_EXPORT void UnRegister(vtkObjectBase* o) override;
  //@}

  //@{
  /**
   * Get/Set the Request ivar
   */
  VTKCOMMONCORE_EXPORT void SetRequest(vtkInformationRequestKey* request);
  VTKCOMMONCORE_EXPORT vtkInformationRequestKey* GetRequest();
  //@}

protected:
  VTKCOMMONCORE_EXPORT vtkInformation();
  VTKCOMMONCORE_EXPORT ~vtkInformation() override;

  // Get/Set a map entry directly through the vtkObjectBase instance
  // representing the value.  Used internally to manage the map.
  VTKCOMMONCORE_EXPORT void SetAsObjectBase(
    vtkInformationKey* key, vtkObjectBase* value);
  VTKCOMMONCORE_EXPORT const vtkObjectBase* GetAsObjectBase(
    const vtkInformationKey* key) const;
  VTKCOMMONCORE_EXPORT vtkObjectBase* GetAsObjectBase(vtkInformationKey* key);

  // Internal implementation details.
  vtkInformationInternals* Internal;

  // Garbage collection support.
  VTKCOMMONCORE_EXPORT void ReportReferences(vtkGarbageCollector*) override;

  // Report the object associated with the given key to the collector.
  VTKCOMMONCORE_EXPORT void ReportAsObjectBase(vtkInformationKey* key,
                          vtkGarbageCollector* collector);

private:

  friend class vtkInformationKeyToInformationFriendship;
  friend class vtkInformationIterator;

private:
  VTKCOMMONCORE_EXPORT vtkInformation(const vtkInformation&) = delete;
  VTKCOMMONCORE_EXPORT void operator=(const vtkInformation&) = delete;
  vtkInformationRequestKey *Request;
};

#endif
// VTK-HeaderTest-Exclude: vtkInformation.h
