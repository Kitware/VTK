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
// .NAME vtkInformation - Store vtkAlgorithm input/output information.
// .SECTION Description
// vtkInformation represents information and/or data for one input or
// one output of a vtkAlgorithm.  It maps from keys to values of
// several data types.  Instances of this class are collected in
// vtkInformationVector instances and passed to
// vtkAlgorithm::ProcessRequest calls.  The information and
// data referenced by the instance on a particular input or output
// define the request made to the vtkAlgorithm instance.

#ifndef __vtkInformation_h
#define __vtkInformation_h

#include "vtkObject.h"

class vtkDataObject;
class vtkExecutive;
class vtkInformationDataObjectKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationExecutivePortKey;
class vtkInformationExecutivePortVectorKey;
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
class vtkInformationStringKey;
class vtkInformationUnsignedLongKey;
class vtkInformationVector;

class VTK_FILTERING_EXPORT vtkInformation : public vtkObject
{
public:
  static vtkInformation *New();
  vtkTypeRevisionMacro(vtkInformation,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Clear all information entries.
  void Clear();

  // Description:
  // Copy all information entries from the given vtkInformation
  // instance.  Any previously existing entries are removed.  If 
  // deep==1, a deep copy of the information structure is performed (new 
  // instances of any contained vtkInformation and vtkInformationVector 
  // objects are created).
  void Copy(vtkInformation* from, int deep=0);

  // Description:
  // Copy the key/value pair associated with the given key in the
  // given information object.  If deep=1, a deep copy of the information
  // structure is performed (new instances of any contained vtkInformation and 
  // vtkInformationVector objects are created).
  void CopyEntry(vtkInformation* from, vtkInformationKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationDataObjectKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationDoubleVectorKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationExecutivePortKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationInformationKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationInformationVectorKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationIntegerKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationIntegerVectorKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationStringKey* key, int deep=0);
  void CopyEntry(vtkInformation* from, vtkInformationUnsignedLongKey* key, int deep=0);

  // Description:
  // Use the given key to lookup a list of other keys in the given
  // information object.  The key/value pairs associated with these
  // other keys will be copied.  If deep==1, a deep copy of the
  // information structure is performed.
  void CopyEntries(vtkInformation* from, vtkInformationKeyVectorKey* key, int deep=0);

  // Description:
  // Get/Set an integer-valued entry.
  void Set(vtkInformationIntegerKey* key, int value);
  int Get(vtkInformationIntegerKey* key);
  void Remove(vtkInformationIntegerKey* key);
  int Has(vtkInformationIntegerKey* key);

  // Description:
  // Get/Set an integer-valued entry.
  void Set(vtkInformationDoubleKey* key, double value);
  double Get(vtkInformationDoubleKey* key);
  void Remove(vtkInformationDoubleKey* key);
  int Has(vtkInformationDoubleKey* key);

  // Description:
  // Get/Set an integer-vector-valued entry.
  void Append(vtkInformationIntegerVectorKey* key, int value);
  void Set(vtkInformationIntegerVectorKey* key, int* value, int length);
  void Set(vtkInformationIntegerVectorKey* key, int value1, 
           int value2, int value3);
  void Set(vtkInformationIntegerVectorKey* key, 
           int value1, int value2, int value3,
           int value4, int value5, int value6);
  int* Get(vtkInformationIntegerVectorKey* key);
  void Get(vtkInformationIntegerVectorKey* key, int* value);
  int Length(vtkInformationIntegerVectorKey* key);
  void Remove(vtkInformationIntegerVectorKey* key);
  int Has(vtkInformationIntegerVectorKey* key);

  // Description:
  // Get/Set an integer-pointer-valued entry.
  void Set(vtkInformationIntegerPointerKey* key, int* value, int length);
  int* Get(vtkInformationIntegerPointerKey* key);
  void Get(vtkInformationIntegerPointerKey* key, int* value);
  int Length(vtkInformationIntegerPointerKey* key);
  void Remove(vtkInformationIntegerPointerKey* key);
  int Has(vtkInformationIntegerPointerKey* key);

  // Description:
  // Get/Set an unsigned-long-valued entry.
  void Set(vtkInformationUnsignedLongKey* key, unsigned long value);
  unsigned long Get(vtkInformationUnsignedLongKey* key);
  void Remove(vtkInformationUnsignedLongKey* key);
  int Has(vtkInformationUnsignedLongKey* key);

  // Description:
  // Get/Set an double-vector-valued entry.
  void Append(vtkInformationDoubleVectorKey* key, double value);
  void Set(vtkInformationDoubleVectorKey* key, double* value, int length);
  void Set(vtkInformationDoubleVectorKey* key, double value1, 
           double value2, double value3);
  void Set(vtkInformationDoubleVectorKey* key, 
           double value1, double value2, double value3,
           double value4, double value5, double value6);
  double* Get(vtkInformationDoubleVectorKey* key);
  void Get(vtkInformationDoubleVectorKey* key, double* value);
  int Length(vtkInformationDoubleVectorKey* key);
  void Remove(vtkInformationDoubleVectorKey* key);
  int Has(vtkInformationDoubleVectorKey* key);

  // Description:
  // Get/Set an InformationKey-vector-valued entry.
  void Append(vtkInformationKeyVectorKey* key, vtkInformationKey* value);
  void Set(vtkInformationKeyVectorKey* key, vtkInformationKey** value, int length);
  void Remove(vtkInformationKeyVectorKey* key, vtkInformationKey* value);
  vtkInformationKey** Get(vtkInformationKeyVectorKey* key);
  void Get(vtkInformationKeyVectorKey* key, vtkInformationKey** value);
  int Length(vtkInformationKeyVectorKey* key);
  void Remove(vtkInformationKeyVectorKey* key);
  int Has(vtkInformationKeyVectorKey* key);

  // Provide extra overloads of this method to avoid requiring user
  // code to include the headers for these key types.  Avoid wrapping
  // them because the original method can be called from the wrappers
  // anyway and this causes a python help string to be too long.
  //BTX
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationDataObjectKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationDoubleKey* value);
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationDoubleVectorKey* value);
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationExecutivePortKey* value);
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationInformationKey* value);
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationInformationVectorKey* value);
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationIntegerKey* value);
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationIntegerVectorKey* value);
  void Append(vtkInformationKeyVectorKey* key, vtkInformationStringKey* value);
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationObjectBaseKey* value);
  void Append(vtkInformationKeyVectorKey* key,
              vtkInformationUnsignedLongKey* value);
  //ETX

  // Description:
  // Get/Set a string-valued entry.
  void Set(vtkInformationStringKey* key, const char*);
  const char* Get(vtkInformationStringKey* key);
  void Remove(vtkInformationStringKey* key);
  int Has(vtkInformationStringKey* key);

  // Description:
  // Get/Set an entry storing another vtkInformation instance.
  void Set(vtkInformationInformationKey* key, vtkInformation*);
  vtkInformation* Get(vtkInformationInformationKey* key);
  void Remove(vtkInformationInformationKey* key);
  int Has(vtkInformationInformationKey* key);

  // Description:
  // Get/Set an entry storing a vtkInformationVector instance.
  void Set(vtkInformationInformationVectorKey* key, vtkInformationVector*);
  vtkInformationVector* Get(vtkInformationInformationVectorKey* key);
  void Remove(vtkInformationInformationVectorKey* key);
  int Has(vtkInformationInformationVectorKey* key);

  // Description:
  // Get/Set an entry storing a vtkObjectBase instance.
  void Set(vtkInformationObjectBaseKey* key, vtkObjectBase*);
  vtkObjectBase* Get(vtkInformationObjectBaseKey* key);
  void Remove(vtkInformationObjectBaseKey* key);
  int Has(vtkInformationObjectBaseKey* key);

  // Description:
  // Get/Set an entry storing a vtkDataObject instance.
  void Set(vtkInformationDataObjectKey* key, vtkDataObject*);
  vtkDataObject* Get(vtkInformationDataObjectKey* key);
  void Remove(vtkInformationDataObjectKey* key);
  int Has(vtkInformationDataObjectKey* key);

  // Description:
  // Get/Set an entry storing a vtkExecutive/port number pair.
  void Set(vtkInformationExecutivePortKey* key, vtkExecutive*, int);
  vtkExecutive* GetExecutive(vtkInformationExecutivePortKey* key);
  int GetPort(vtkInformationExecutivePortKey* key);
  void Remove(vtkInformationExecutivePortKey* key);
  int Has(vtkInformationExecutivePortKey* key);

  // Description:
  // Get/Set an entry storing a vector of vtkExecutive/port number pairs.
  void Append(vtkInformationExecutivePortVectorKey* key,
              vtkExecutive* executive, int port);
  void Remove(vtkInformationExecutivePortVectorKey* key,
              vtkExecutive* executive, int port);
  void Set(vtkInformationExecutivePortVectorKey* key,
           vtkExecutive** executives, int* ports, int length);
  vtkExecutive** GetExecutives(vtkInformationExecutivePortVectorKey* key);
  int* GetPorts(vtkInformationExecutivePortVectorKey* key);
  void Get(vtkInformationExecutivePortVectorKey* key,
           vtkExecutive** executives, int* ports);
  int Length(vtkInformationExecutivePortVectorKey* key);
  void Remove(vtkInformationExecutivePortVectorKey* key);
  int Has(vtkInformationExecutivePortVectorKey* key);

  // Description:
  // Upcast the given key instance.
  static vtkInformationKey* GetKey(vtkInformationDataObjectKey* key);
  static vtkInformationKey* GetKey(vtkInformationDoubleKey* key);
  static vtkInformationKey* GetKey(vtkInformationDoubleVectorKey* key);
  static vtkInformationKey* GetKey(vtkInformationExecutivePortKey* key);
  static vtkInformationKey* GetKey(vtkInformationInformationKey* key);
  static vtkInformationKey* GetKey(vtkInformationInformationVectorKey* key);
  static vtkInformationKey* GetKey(vtkInformationIntegerKey* key);
  static vtkInformationKey* GetKey(vtkInformationIntegerVectorKey* key);
  static vtkInformationKey* GetKey(vtkInformationStringKey* key);
  static vtkInformationKey* GetKey(vtkInformationKey* key);
  static vtkInformationKey* GetKey(vtkInformationUnsignedLongKey* key);

  // Description:
  // Initiate garbage collection when a reference is removed.
  virtual void Register(vtkObjectBase* o);
  virtual void UnRegister(vtkObjectBase* o);

protected:
  vtkInformation();
  ~vtkInformation();

  // Get/Set a map entry directly through the vtkObjectBase instance
  // representing the value.  Used internally to manage the map.
  void SetAsObjectBase(vtkInformationKey* key, vtkObjectBase* value);
  vtkObjectBase* GetAsObjectBase(vtkInformationKey* key);

  // Internal implementation details.
  vtkInformationInternals* Internal;

  // Garbage collection support.
  virtual void ReportReferences(vtkGarbageCollector*);

  // Report the object associated with the given key to the collector.
  void ReportAsObjectBase(vtkInformationKey* key,
                          vtkGarbageCollector* collector);

private:
  //BTX
  friend class vtkInformationKeyToInformationFriendship;
  //ETX
private:
  vtkInformation(const vtkInformation&);  // Not implemented.
  void operator=(const vtkInformation&);  // Not implemented.
};

#endif
