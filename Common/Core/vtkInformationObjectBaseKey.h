/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationObjectBaseKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationObjectBaseKey
 * @brief   Key for vtkObjectBase values.
 *
 * vtkInformationObjectBaseKey is used to represent keys in
 * vtkInformation for values that are vtkObjectBase instances.
*/

#ifndef vtkInformationObjectBaseKey_h
#define vtkInformationObjectBaseKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class vtkObjectBase;

class VTKCOMMONCORE_EXPORT vtkInformationObjectBaseKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationObjectBaseKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkInformationObjectBaseKey(const char* name, const char* location,
                              const char* requiredClass=0);
  ~vtkInformationObjectBaseKey() VTK_OVERRIDE;

  /**
   * This method simply returns a new vtkInformationObjectBaseKey, given a
   * name, location and optionally a required class (a classname to restrict
   * which class types can be set with this key). This method is provided
   * for wrappers. Use the constructor directly from C++ instead.
   */
  static vtkInformationObjectBaseKey* MakeKey(const char* name, const char* location,
                                             const char* requiredClass=0)
  {
    return new vtkInformationObjectBaseKey(name, location, requiredClass);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, vtkObjectBase*);
  vtkObjectBase* Get(vtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) VTK_OVERRIDE;

  /**
   * Report a reference this key has in the given information object.
   */
  void Report(vtkInformation* info, vtkGarbageCollector* collector) VTK_OVERRIDE;

protected:
  // The type required of all objects stored with this key.
  const char* RequiredClass;

  vtkInformationKeySetStringMacro(RequiredClass);

private:
  vtkInformationObjectBaseKey(const vtkInformationObjectBaseKey&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInformationObjectBaseKey&) VTK_DELETE_FUNCTION;
};

#endif
