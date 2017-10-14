/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationRequestKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationRequestKey
 * @brief   Key for pointer to pointer.
 *
 * vtkInformationRequestKey is used to represent keys for pointer
 * to pointer values in vtkInformation.h
*/

#ifndef vtkInformationRequestKey_h
#define vtkInformationRequestKey_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkInformationKey.h"

#include "vtkCommonInformationKeyManager.h" // Manage instances of this type.

class VTKCOMMONCORE_EXPORT vtkInformationRequestKey : public vtkInformationKey
{
public:
  vtkTypeMacro(vtkInformationRequestKey,vtkInformationKey);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkInformationRequestKey(const char* name, const char* location);
  ~vtkInformationRequestKey() override;

  /**
   * This method simply returns a new vtkInformationRequestKey, given a
   * name and a location. This method is provided for wrappers. Use the
   * constructor directly from C++ instead.
   */
  static vtkInformationRequestKey* MakeKey(const char* name, const char* location)
  {
    return new vtkInformationRequestKey(name, location);
  }

  //@{
  /**
   * Get/Set the value associated with this key in the given
   * information object.
   */
  void Set(vtkInformation* info);
  void Remove(vtkInformation* info) override;
  int Has(vtkInformation* info) override;
  //@}

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

private:
  vtkInformationRequestKey(const vtkInformationRequestKey&) = delete;
  void operator=(const vtkInformationRequestKey&) = delete;
};

#endif
