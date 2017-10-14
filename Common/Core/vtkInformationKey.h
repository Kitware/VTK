/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationKey
 * @brief   Superclass for vtkInformation keys.
 *
 * vtkInformationKey is the superclass for all keys used to access the
 * map represented by vtkInformation.  The vtkInformation::Set and
 * vtkInformation::Get methods of vtkInformation are accessed by
 * information keys.  A key is a pointer to an instance of a subclass
 * of vtkInformationKey.  The type of the subclass determines the
 * overload of Set/Get that is selected.  This ensures that the type
 * of value stored in a vtkInformation instance corresponding to a
 * given key matches the type expected for that key.
*/

#ifndef vtkInformationKey_h
#define vtkInformationKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObjectBase.h"
#include "vtkObject.h" // Need vtkTypeMacro

class vtkInformation;

class VTKCOMMONCORE_EXPORT vtkInformationKey : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationKey,vtkObjectBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Prevent normal vtkObject reference counting behavior.
   */
  void Register(vtkObjectBase*) override;

  /**
   * Prevent normal vtkObject reference counting behavior.
   */
  void UnRegister(vtkObjectBase*) override;

  /**
   * Get the name of the key.  This is not the type of the key, but
   * the name of the key instance.
   */
  const char* GetName();

  /**
   * Get the location of the key.  This is the name of the class in
   * which the key is defined.
   */
  const char* GetLocation();

  //@{
  /**
   * Key instances are static data that need to be created and
   * destroyed.  The constructor and destructor must be public.  The
   * name of the static instance and the class in which it is defined
   * should be passed to the constructor.  They must be string
   * literals because the strings are not copied.
   */
  vtkInformationKey(const char* name, const char* location);
  ~vtkInformationKey() override;
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to)=0;

  /**
   * Duplicate (new instance created) the entry associated with this key from
   * one information object to another (new instances of any contained
   * vtkInformation and vtkInformationVector objects are created).
   * Default implementation simply calls ShallowCopy().
   */
  virtual void DeepCopy(vtkInformation *from, vtkInformation *to)
    { this->ShallowCopy(from, to); }

  /**
   * Check whether this key appears in the given information object.
   */
  virtual int Has(vtkInformation* info);

  /**
   * Remove this key from the given information object.
   */
  virtual void Remove(vtkInformation* info);

  /**
   * Report a reference this key has in the given information object.
   */
  virtual void Report(vtkInformation* info, vtkGarbageCollector* collector);

  //@{
  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(vtkInformation* info);
  virtual void Print(ostream& os, vtkInformation* info);
  //@}

  /**
   * This function is only relevant when the pertaining key
   * is used in a VTK pipeline. Specific keys that handle
   * pipeline data requests (for example, UPDATE_PIECE_NUMBER)
   * can overwrite this method to notify the pipeline that a
   * a filter should be (re-)executed because what is in
   * the current output is different that what is being requested
   * by the key. For example, DATA_PIECE_NUMBER != UPDATE_PIECE_NUMBER.
   */
  virtual bool NeedToExecute(vtkInformation* vtkNotUsed(pipelineInfo),
                             vtkInformation* vtkNotUsed(dobjInfo)) {return false;}

  /**
   * This function is only relevant when the pertaining key
   * is used in a VTK pipeline. Specific keys that handle
   * pipeline data requests (for example, UPDATE_PIECE_NUMBER)
   * can overwrite this method to store in the data information
   * meta-data about the request that led to the current filter
   * execution. This meta-data can later be used to compare what
   * is being requested to decide whether the filter needs to
   * re-execute. For example, a filter may store the current
   * UPDATE_PIECE_NUMBER in the data object's information as
   * the DATA_PIECE_NUMBER. DATA_PIECE_NUMBER can later be compared
   * to a new UPDATA_PIECE_NUMBER to decide whether a filter should
   * re-execute.
   */
  virtual void StoreMetaData(vtkInformation* vtkNotUsed(request),
                             vtkInformation* vtkNotUsed(pipelineInfo),
                             vtkInformation* vtkNotUsed(dobjInfo)) {}

  /**
   * This function is only relevant when the pertaining key
   * is used in a VTK pipeline. By overwriting this method, a
   * key can decide if/how to copy itself downstream or upstream
   * during a particular pipeline pass. For example, meta-data keys
   * can copy themselves during REQUEST_INFORMATION whereas request
   * keys can copy themselves during REQUEST_UPDATE_EXTENT.
   */
  virtual void CopyDefaultInformation(vtkInformation* vtkNotUsed(request),
                                      vtkInformation* vtkNotUsed(fromInfo),
                                      vtkInformation* vtkNotUsed(toInfo)) {}

protected:
  char* Name;
  char* Location;

#define vtkInformationKeySetStringMacro(name) \
virtual void Set##name (const char* _arg) \
{ \
  if ( this->name == nullptr && _arg == nullptr) { return;} \
  if ( this->name && _arg && (!strcmp(this->name,_arg))) { return;} \
  delete [] this->name; \
  if (_arg) \
  { \
    size_t n = strlen(_arg) + 1; \
    char *cp1 =  new char[n]; \
    const char *cp2 = (_arg); \
    this->name = cp1; \
    do { *cp1++ = *cp2++; } while ( --n ); \
  } \
   else \
   { \
    this->name = nullptr; \
   } \
}

  vtkInformationKeySetStringMacro(Name);
  vtkInformationKeySetStringMacro(Location);

  // Set/Get the value associated with this key instance in the given
  // information object.
  void SetAsObjectBase(vtkInformation* info, vtkObjectBase* value);
  const vtkObjectBase* GetAsObjectBase(vtkInformation* info) const;
  vtkObjectBase* GetAsObjectBase(vtkInformation* info);

  // Report the object associated with this key instance in the given
  // information object to the collector.
  void ReportAsObjectBase(vtkInformation* info,
                          vtkGarbageCollector* collector);

  // Helper for debug leaks support.
  void ConstructClass(const char*);

private:
  vtkInformationKey(const vtkInformationKey&) = delete;
  void operator=(const vtkInformationKey&) = delete;
};

// Macros to define an information key instance in a C++ source file.
// The corresponding method declaration must appear in the class
// definition in the header file.
#define vtkInformationKeyMacro(CLASS, NAME, type)             \
  static vtkInformation##type##Key* CLASS##_##NAME =          \
    new vtkInformation##type##Key(#NAME, #CLASS);             \
  vtkInformation##type##Key* CLASS::NAME()                    \
  {                                                           \
    return CLASS##_##NAME;                                    \
  }
#define vtkInformationKeySubclassMacro(CLASS, NAME, type, super)    \
  static vtkInformation##type##Key* CLASS##_##NAME =          \
    new vtkInformation##type##Key(#NAME, #CLASS);             \
  vtkInformation##super##Key* CLASS::NAME()                    \
  {                                                           \
    return CLASS##_##NAME;                                    \
  }
#define vtkInformationKeyRestrictedMacro(CLASS, NAME, type, required)   \
  static vtkInformation##type##Key* CLASS##_##NAME =                    \
    new vtkInformation##type##Key(#NAME, #CLASS, required);             \
  vtkInformation##type##Key* CLASS::NAME()                              \
  {                                                                     \
    return CLASS##_##NAME;                                              \
  }


#endif
