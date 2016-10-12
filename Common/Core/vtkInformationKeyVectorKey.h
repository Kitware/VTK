/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKeyVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKCOMMONCORE_EXPORT vtkInformationKeyVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationKeyVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkInformationKeyVectorKey(const char* name, const char* location);
  ~vtkInformationKeyVectorKey() VTK_OVERRIDE;

  /**
   * This method simply returns a new vtkInformationKeyVectorKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static vtkInformationKeyVectorKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationKeyVectorKey(name, location);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(vtkInformation* info, vtkInformationKey* value);
  void AppendUnique(vtkInformation* info, vtkInformationKey* value);
  void Set(vtkInformation* info, vtkInformationKey*const * value, int length);
  void RemoveItem(vtkInformation* info, vtkInformationKey* value);
  vtkInformationKey** Get(vtkInformation* info);
  vtkInformationKey*  Get(vtkInformation* info, int idx);
  void Get(vtkInformation* info, vtkInformationKey** value);
  int Length(vtkInformation* info);
  //@}

  /**
   * Copy the entry associated with this key from one information
   * object to another.  If there is no entry in the first information
   * object for this key, the value is removed from the second.
   */
  void ShallowCopy(vtkInformation* from, vtkInformation* to) VTK_OVERRIDE;

  /**
   * Print the key's value in an information object to a stream.
   */
  void Print(ostream& os, vtkInformation* info) VTK_OVERRIDE;

private:
  vtkInformationKeyVectorKey(const vtkInformationKeyVectorKey&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInformationKeyVectorKey&) VTK_DELETE_FUNCTION;
};

#endif
