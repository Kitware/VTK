/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationInformationKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationInformationKey
 * @brief   Key for vtkInformation values.
 *
 * vtkInformationInformationKey is used to represent keys in vtkInformation
 * for other information objects.
*/

#ifndef vtkInformationInformationKey_h
#define vtkInformationInformationKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTKCOMMONCORE_EXPORT vtkInformationInformationKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationInformationKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkInformationInformationKey(const char* name, const char* location);
  ~vtkInformationInformationKey() VTK_OVERRIDE;

  /**
   * This method simply returns a new vtkInformationInformationKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static vtkInformationInformationKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationInformationKey(name, location);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info, vtkInformation*);
  vtkInformation* Get(vtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) VTK_OVERRIDE;

  /**
   * Duplicate (new instance created) the entry associated with this key from
   * one information object to another (new instances of any contained
   * vtkInformation and vtkInformationVector objects are created).
   */
  void DeepCopy(vtkInformation* from, vtkInformation* to) VTK_OVERRIDE;

private:
  vtkInformationInformationKey(const vtkInformationInformationKey&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInformationInformationKey&) VTK_DELETE_FUNCTION;
};

#endif
