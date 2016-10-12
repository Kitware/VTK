/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationVariantVectorKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationVariantVectorKey
 * @brief   Key for variant vector values.
 *
 * vtkInformationVariantVectorKey is used to represent keys for variant
 * vector values in vtkInformation.h
*/

#ifndef vtkInformationVariantVectorKey_h
#define vtkInformationVariantVectorKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class vtkVariant;

class VTKCOMMONCORE_EXPORT vtkInformationVariantVectorKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationVariantVectorKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkInformationVariantVectorKey(const char* name, const char* location,
                                 int length=-1);
  ~vtkInformationVariantVectorKey() VTK_OVERRIDE;

  /**
   * This method simply returns a new vtkInformationVariantVectorKey, given a
   * name, a location and a required length. This method is provided for
   * wrappers. Use the constructor directly from C++ instead.
   */
  static vtkInformationVariantVectorKey* MakeKey(const char* name, const char* location,
    int length=-1)
  {
    return new vtkInformationVariantVectorKey(name, location, length);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Append(vtkInformation* info, const vtkVariant& value);
  void Set(vtkInformation* info, const vtkVariant* value, int length);
  const vtkVariant* Get(vtkInformation* info) const;
  const vtkVariant& Get(vtkInformation* info, int idx) const;
  void Get(vtkInformation* info, vtkVariant* value) const;
  int Length(vtkInformation* info) const;
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

protected:
  // The required length of the vector value (-1 is no restriction).
  int RequiredLength;

private:
  vtkInformationVariantVectorKey(const vtkInformationVariantVectorKey&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInformationVariantVectorKey&) VTK_DELETE_FUNCTION;
};

#endif
